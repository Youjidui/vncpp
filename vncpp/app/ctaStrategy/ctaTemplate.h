#pragma once
#include <stdint.h>
#include <memory>
#include <functional>
#include <set>
#include <map>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "logging.h"
//#include "eventEngine.h"
#ifdef __linux__
#include <dlfcn.h>
#elif defined(__WINDOWS)
#include <Windows.h>
#endif
#include "ctaBase.h"

class CtaEngine;

typedef boost::variant<bool, int, double, std::string> ParameterValue;

class Strategy : public std::enable_shared_from_this<Strategy>
{
	public:
	const std::string name;		//the instance name (with different symbol)
	virtual const std::string getClassName() = 0;

    CtaEngine& m_ctaEngine;

	//strategy parameters
	//std::string m_settingfilePath;
	boost::property_tree::ptree parameters;

    public:
    //from configuration setting
    //store in boost::property_tree::ptree parameters
	const std::string vtSymbol;
    const std::string productClass;
    const std::string currency;

    const std::string tickDbName;
    const std::string barDbName;

    public:
    int position;

    public:
    Strategy(const std::string& instanceName, CtaEngine& e)
    : name(instanceName)
	, m_ctaEngine(e)
    {}

	public:
	void setParameter(boost::property_tree::ptree&& aParameters)
	{
		parameters = aParameters;
		onSetParameters();
	}

	virtual bool init() = 0;
	virtual void onInit() =0;
	virtual bool start() =0;
	virtual void onStart() =0;
	virtual void stop() =0;
	virtual void onStop() =0;

    virtual void onTick(TickPtr) =0;
    virtual void onOrder(OrderPtr) =0;
    virtual void onTrade(TradePtr) =0;
    virtual void onBar(BarPtr) =0;      //K-line
    virtual void onStopOrder(StopOrderPtr) =0;

    //buy and open
    OrderID buy(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_BUY, price, volume, stop);
    }
    //sell and close
    OrderID sell(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_SELL, price, volume, stop);
    }
    //sell short and open
    OrderID Short(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_SHORT, price, volume, stop);
    }
    //buy and close
    OrderID cover(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_COVER, price, volume, stop);
    }

    OrderID sendOrder(int orderType, double price, int volume, bool stop = false);

    void cancelOrder(const OrderID& vtOrderID);

    void cancelAll();


    void insertTick(TickPtr tick);
    void insertBar(BarPtr bar);
	std::vector<TickPtr> loadTick(int days);
	std::vector<BarPtr> loadBar(int days);

    void saveSyncData();

    double getPriceTick();

	int getEngineType();

	protected:
	virtual void onSetParameters() {}
};

typedef Strategy CtaTemplate;

//the strategy implementation has to implement and export those function for CtaEngine
typedef Strategy* (*FUNCTION_createStrategyInstance)(const char* aInstanceName, const char* aStrategyClassName, CtaEngine* engine);
typedef void (*FUNCTION_destroyStrategyInstance)(Strategy* aStrategyInstance);

struct StrategyDeleter
{
	typedef void (*FUNCTION_strategyDeleter)(Strategy* aStrategyInstance, void* aHost);
	FUNCTION_strategyDeleter df;
	void* pr;

	StrategyDeleter(FUNCTION_strategyDeleter d, void* p)
		: df(d), pr(p)
	{}

	void operator()(Strategy* s)
	{
		df(s, pr);
	}
};

//for the built-in strategy created by C++ keyword new
inline 
void staticLibraryDestroyStrategyInstance(Strategy* p, void* aHost = NULL)
{
	delete p;
}

//for the strategy created by FUNCTION_createStrategyInstance from DLL
void dynamicLibraryDestroyStrategyInstance(Strategy* p, void* aHost);

typedef std::shared_ptr<Strategy> StrategyPtr;

template<class T>
StrategyPtr makeStrategyPtr(const std::string& instanceName, CtaEngine& e, void* host)
{
	if(host) return std::shared_ptr<Strategy>(new T(instanceName, e), StrategyDeleter(&dynamicLibraryDestroyStrategyInstance, host));
	else return std::shared_ptr<Strategy>(new T(instanceName, e), StrategyDeleter(&staticLibraryDestroyStrategyInstance, host));
}



class StrategyLoaderForDynamicLibrary
{
	protected:
	typedef std::string StrategyClassName;
	typedef std::string ModuleName;
	typedef void* ModuleHandle;
	std::map<StrategyClassName, ModuleHandle> m_dllHandleDict;

	friend void dynamicLibraryDestroyStrategyInstance(Strategy* p, void* aHost);

	protected:
	//there may be more than a strategy in a DLL, using the strategy name to identity each other
	Strategy* loadStrategy(const std::string& aInstanceName, const std::string& aStrategyClassName,
     const std::string& aModuleName, CtaEngine& engine)	
	{
		Strategy* r = nullptr;
		#ifdef __linux__
		void* handle = dlopen(aModuleName.c_str(), RTLD_NOW);
		#elif defined(__WINDOWNS)
		void* handle = LoadLibrary(aModuleName.c_str());
		#endif

		if(handle)
		{
			FUNCTION_createStrategyInstance createfn = (FUNCTION_createStrategyInstance)
			#ifdef __linux__
			dlsym
			#elif defined(__WINDOWS)
			GetProcAddress 
			#endif
			(handle, "createStrategyInstance");
			if(createfn)
				r = (Strategy*)createfn(aInstanceName.c_str(), aStrategyClassName.c_str(), &engine);
			if(r)
			{
				//to free the module
				m_dllHandleDict.insert(std::make_pair(r->getClassName(), handle));
			}
		}
		return r;
	}

	void freeStrategy(Strategy* s)
	{
		if(s)
		{
			auto it = m_dllHandleDict.find(s->getClassName());
			if(it != m_dllHandleDict.end())
			{
				void* handle = it->second;
				auto destroyFn = (FUNCTION_destroyStrategyInstance)
				#ifdef __linux__
				dlsym
				#elif defined(__WINDOWS)
				GetProcAddress 
				#endif
				(handle, "destroyStrategyInstance");
				if(destroyFn)
					destroyFn(s);

				#ifdef __linux__
				dlclose(handle);
				#elif defined(__WINDOWS)
				FreeLibrary(handle);
				#endif
			}

		}
	}

public:
	StrategyPtr load(const std::string& aInstanceName, const std::string& aStrategyClassName, 
    const std::string& aModuleName, CtaEngine& engine)
	{
		auto p = loadStrategy(aInstanceName, aStrategyClassName, aModuleName, engine);
		if(p)
			return StrategyPtr(p, StrategyDeleter(&dynamicLibraryDestroyStrategyInstance, this));
		else
			throw std::runtime_error("null pointer. load strategy failed");
	}
};

inline
void dynamicLibraryDestroyStrategyInstance(Strategy* p, void* aHost)
{
	auto h = (StrategyLoaderForDynamicLibrary*)aHost;
	if(h)
	{
		h->freeStrategy(p);
	}
}




class TargetPosTemplate : public CtaTemplate
{
	/*
    className = 'TargetPosTemplate'
    author = u'量衍投资'
    允许直接通过修改目标持仓来实现交易的策略模板
    
    开发策略时，无需再调用buy/sell/cover/short这些具体的委托指令，
    只需在策略逻辑运行完成后调用setTargetPos设置目标持仓，底层算法
    会自动完成相关交易，适合不擅长管理交易挂撤单细节的用户。    
    
    使用该模板开发策略时，请在以下回调方法中先调用母类的方法：
    onTick
    onBar
    onOrder
    
    假设策略名为TestStrategy，请在onTick回调中加上：
    super(TestStrategy, self).onTick(tick)
    
    其他方法类同。
	*/
	bool m_running;
	std::set<OrderID> m_orderList;
	int m_targetPosition;
	double m_tickAdd;		//price offset
	TickPtr m_lastTick;
	BarPtr m_lastBar;		//K-line item

	public:
	TargetPosTemplate(const std::string& instanceName, CtaEngine& e)
	: CtaTemplate(instanceName, e)
	{}

	virtual const std::string getClassName()
	{
		return "TargetPosTemplate";
	}

	void setTargetPosition(int targetPos)
	{
		m_targetPosition = targetPos;
		trade();
	}

	void trade();
    virtual void onTick(TickPtr tick)
	{
		m_lastTick = tick;
		if(m_running)
			trade();
	}
    virtual void onOrder(OrderPtr o)
	{
		if(o->status == STATUS_ALLTRADED || o->status == STATUS_CANCELLED)
		{
			auto i = m_orderList.find(o->vtOrderID);
			if(i != m_orderList.cend())
			{
				m_orderList.erase(i);
			}
		}
	}
    virtual void onTrade(TradePtr)
	{}
    virtual void onBar(BarPtr bar)
	{
		m_lastBar = bar;
	}
    virtual void onStopOrder(StopOrderPtr)
	{}
	
};


#include <time.h>
//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>


inline
bool isInMinuteRange(time_t t1, time_t t2, int aMinute)
{
	//struct tm a, b;
	auto diff = difftime(t1, t2);
	return (diff / 60 < aMinute);
}




class BarGenerator
{
	/*
    K线合成器，支持：
    1. 基于Tick合成1分钟K线
    2. 基于1分钟K线合成X分钟K线（X可以是2、3、5、10、15、30	）
	*/
public:
	BarPtr bar;
	int m_xminute;
	BarPtr m_xminuteBar;
	TickPtr m_lastTick;

	public:
	virtual void onBar(BarPtr bar)
	{}
	virtual void onXMinuteBar(BarPtr bar)
	{}

public:
	void updateTick(TickPtr tick)
	{
		bool newMinute = false;

		if(!bar)
		{
			bar = std::make_shared<Bar>();
			newMinute = true;
		}
		else if(!isInMinuteRange(bar->datetime, tick->datetime, 1))
		{
			bar->datetime = (bar->datetime / 60) * 60;
			onBar(bar);

			bar = std::make_shared<Bar>();
			newMinute = true;
		}

		if(newMinute)
		{
			bar->vtSymbol = tick->vtSymbol;
			bar->symbol = tick->symbol;
			bar->exchange = tick->exchange;
			bar->open = tick->lastPrice;
			bar->high = tick->lastPrice;
			bar->low = tick->lastPrice;
		}
		else
		{
			bar->high = std::max(bar->high, tick->lastPrice);
			bar->low = std::min(bar->low, tick->lastPrice);
		}

		bar->close = tick->lastPrice;
		bar->datetime = tick->datetime;
		bar->openInterest = tick->openInterest;

		if(m_lastTick)
		{
			auto volumeChange = tick->totalVolume - m_lastTick->totalVolume;
			bar->volume += std::max(volumeChange, 0);
		}
		m_lastTick = tick;
	}

	void updateBar(BarPtr bar)
	{
		if(!m_xminuteBar)
		{
			m_xminuteBar = std::make_shared<Bar>();
			m_xminuteBar->vtSymbol = bar->vtSymbol;
			m_xminuteBar->symbol = bar->symbol;
			m_xminuteBar->exchange = bar->exchange;

			m_xminuteBar->open = bar->open;
			m_xminuteBar->high = bar->high;
			m_xminuteBar->low = bar->low;

			m_xminuteBar->datetime = bar->datetime;
		}
		else
		{
			m_xminuteBar->close = bar->close;
			m_xminuteBar->openInterest = bar->openInterest;
			m_xminuteBar->volume += bar->volume;
		}
		
		struct tm* lt = localtime(&(m_xminuteBar->datetime));
		if((lt->tm_min + 1) % m_xminute == 0)
		{
			m_xminuteBar->datetime = m_xminuteBar->datetime / 60 * 60;

			BarPtr bar;
			std::swap(bar, m_xminuteBar);
			onXminuteBar(bar);
		}
	}

	void generate()
	{
		std::shared_ptr<Bar> bar;
		std::swap(bar, this->bar);
		onBar(bar);
	}

	void onXminuteBar(BarPtr)
	{}
};


class ArrayManager
{
	/*
    K线序列管理工具，负责：
    1. K线时间序列的维护
    2. 常用技术指标的计算
	*/
public:
	int count;
	int size;

public:
	ArrayManager()
	{
		//TA_Initialize();
	}
	~ArrayManager()
	{
		//TA_Shutdown();
	}


	public:
	void sma(int n)
	{
		//return TA_SMA();
	}

	public:
	void updateBar(BarPtr b)
	{
	}

	void keltner(int kkLength, double kkDev)
	{
	}
};




/*
 *
 * strategy configuration file format
 *
 * it's a JSON file with following format
 *
 * 	{
 * 		"strategies" : [
 * 		{
 * 			"instanceName" : 	"DemoStrategy 1",
 * 			"className" : 		"DemoStrategy",
 * 			"filePath" : 		"./startegy/libdemostrategy.so"
 * 			"parameters" : 
 * 			{
 * 				"parameter1" : 	true,
 * 				"parameter2" : 	1,
 * 				"parameter3" : 	0.1
 * 			}
 * 		},
 * 		{
 * 			"instanceName" : 	"KingKeltner",
 * 			"className" : 		"StrategyKingKeltner",
 * 			"filePath" : 		"./strategy/libstrategykingkeltner.so"
 * 			"parameters" : 
 * 			{
 * 				"parameter1" : "demo"
 * 			}
 * 		}
 * 		]
 * 	}
 *
 * */


struct CtaStrategyConfigurationManager
{
public:
	typedef std::string StrategyInstanceName;
	
	std::shared_ptr<boost::property_tree::ptree> parameters;
	StrategyLoaderForDynamicLibrary m_loader;

public:
	void loadConfiguration(const std::string& configFilePath)
	{
		std::ifstream f(configFilePath);
		if(f.is_open())
		{
			parameters = std::make_shared<boost::property_tree::ptree>();
			boost::property_tree::json_parser::read_json(f, *parameters);
		}
	}

	void loadStrategies(CtaEngine& engine, std::map<StrategyInstanceName, StrategyPtr>& strategyDict)
	{
		auto& pt = parameters->get_child("strategies");
		for(auto i : pt)
		{
			std::string instanceName;
			instanceName = i.second.get("instanceName", instanceName);
			StrategyPtr p;
		
			auto it = strategyDict.find(instanceName);
			if(it == strategyDict.end())
			{
				std::string className;
				className = i.second.get("className", className);
				std::string filePath;
				filePath = i.second.get("filePath", filePath);

				p = m_loader.load(instanceName, className, filePath, engine);
				strategyDict.insert(std::make_pair(instanceName, p));
			}
			else
			{
				p = it->second;
			}
			auto pp = i.second.get_child("parameters");
			p->setParameter(std::move(pp));
		}
	}

};



