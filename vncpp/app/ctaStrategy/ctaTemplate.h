#include <stdint.h>
#include <memory>
#include <functional>
#include <list>
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
	const std::string className;

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
    Strategy(const std::string& instanceName, const std::string& aClassName, 
	CtaEngine& e, const boost::property_tree::ptree& aParameters)
    : name(instanceName), className(aClassName) m_ctaEngine(e), parameters(aParameters)
    {}

	public:
	virtual bool init() =0;
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
    OrderID short(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_SHORT, price, volume, stop);
    }
    //buy and close
    OrderID cover(double price, int volume, bool stop = false)
    {
        return sendOrder(CTAORDER_COVER, price, volume, stop);
    }

    OrderID sendOrder(int orderType, double price, int volume, bool stop = false)
    {
        if(stop)
            return m_ctaEngine.sendStopOrder(vtSymbol, orderType, price, volume, shared_from_this());
        else
            return m_ctaEngine.sendOrder(vtSymbol, orderType, price, volume, shared_from_this());
            
    }

    void cancelOrder(const OrderID& vtOrderID)
    {
        if(isStopOrderID(vtOrderID))
            m_ctaEngine.cancelStopOrder(vtOrderID);
        else
            m_ctaEngine.cancelOrder(vtOrderID);
    }

    void cancelAll()
    {
        m_ctaEngine.cancelAll(name);
    }


    void insertTick(TickPtr tick)
    {
        m_ctaEngine.insertData(tickDbName, vtSymbol, tick);
    }
    void insertBar(BarPtr bar)
    {
        m_ctaEngine.insertData(BarDbName, vtSymbol, bar);
    }
    void loadTick(int days)
    {
        m_ctaEngine.loadTick(tickDbName, vtSymbol, days);
    }
    void loadBar(int days)
    {
        m_ctaEngine.loadBar(barDbName, vtSymbol, days);
    }

    void saveSyncData()
    {
        m_ctaEngine.saveSyncData(shared_from_this());
    }

    double getPriceTick()
    {
        m_ctaEngine.getPriceTick(vtSymbol);
    }
};

typedef Strategy CtaTemplate;

//the strategy implementation has to implement and export those function for CtaEngine
typedef Strategy* (*FUNCTION_createStrategyInstance)(const char* aInstanceName, const char* aStrategyClassName, CtaEngine* engine);
typedef void (*FUNCTION_destroyStrategyInstance)(Strategy* aStrategyInstance);

typedef void (*FUNCTION_strategyDeleter)(Strategy* aStrategyInstance, void* aHost);

//for the built-in strategy created by C++ keyword new
inline 
void staticLibraryDestroyStrategyInstance(Strategy* p, void* aHost = NULL)
{
	delete p;
}

//for the strategy created by FUNCTION_createStrategyInstance from DLL
void dynamicLibraryDestroyStrategyInstance(Strategy* p, void* aHost);

typedef std::shared_ptr<Strategy, FUNCTION_strategyDeleter> StrategyPtr;




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
     const std::string& aModuleName, CtaEngine& e)
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
				r = (Strategy*)createfn(aInstanceName, aStrategyClassName.c_str(), &engine);
			if(r)
			{
				//to free the module
				m_dllHandleDict.insert(std::make_pair(r->className, handle));
			}
		}
		return r;
	}

	void freeStrategy(Strategy* s)
	{
		if(s)
		{
			auto it = m_dllHandleDict.find(s->className);
			if(it != m_dllHandleDict.end())
			{
				FUNCTION_destroyStrategyInstance destryFn = (FUNCTION_destroyStrategyInstance)
				#ifdef __linux__
				dlsym
				#elif defined(__WINDOWS)
				GetProcAddress 
				#endif
				(handle, "destroyStrategyInstance");
				if(destroyFn)
					destroyFn(s);

				#ifdef __linux__
				dlclose(it->second);
				#elif defined(__WINDOWS)
				FreeLibrary(it->second);
				#endif
			}

		}
	}

	public:
	StrategyPtr load(const std::string& aInstanceName, const std::string& aStrategyClassName, 
    const std::string& aModuleName, CtaEngine& engine)
	{
		auto p = loadStrategy(aInstanceName, aStrategyName, aModuleName, engine);
		if(p)
			return StrategyPtr(p, &dynamicLibraryDestroyStrategyInstance);
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
		h->freeStrategy(s);
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
	TargetPosTemplate(const std::string& instanceName, CtaEngine& e
	, const boost::property_tree::ptree& aParameters) 
	: CtaTemplate(instanceName, "TargetPosTemplate", e, aParameters)
	{}

	void setTargetPosition(int targetPos)
	{
		m_targetPosition = targetPos;
		trade();
	}

	void trade()
	{
		if(!m_orderList.empty())
			cancelAll();
		else
		{
			int posChange = m_targetPosition - position;
			if(posChange == 0)
				return;

			double longPrice = 0;
			double shortPrice = 0;

			if(m_lastTick)
			{
				if(posChange > 0)
				{
					longPrice = m_lastTick.askPrice[0] + m_tickAdd;
					if(m_lastTick.upperLimit)
						longPrice = std::min(longPrice, m_lastTick.upperLimit);
				}
				else
				{
					shortPrice = m_lastTick.bidPrice[0] - m_tickAdd;
					if(m_lastTick.lowerLimit)
						shortPrice = std::max(shortPrice, m_lastTick.lowerLimit);
				}
			}
			else if(m_lastBar)
			{
				if(posChange > 0)
					longPrice = m_lastBar.close + m_tickAdd;
				else
					shortPrice = m_lastBar.close - m_tickAdd;
			}
			else
			{
				//do nothing
			}
		

			if(getEngineType() == ENGINETYPE_BACKTESTING)
			{
				if(posChange > 0)
				{
					auto id = buy(longPrice, posChange);
					m_orderList.insert(id);
				}
				else
				{
					auto id = short(shortPrice, -posChange);
					m_orderList.insert(id);
				}
			}
			else
			{
				if(posChange > 0)
				{
					if(position < 0)
					{
						auto posCover = std::min(posChange, -position);
						auto id = cover(longPrice, posCover);
						m_orderList.insert(id);
					}
					else
					{
						auto posBuy = posChange;
						auto id = buy(longPrice, posBuy);
						m_orderList.insert(id);
					}
				}
				else
				{
					if(position > 0)
					{
						auto posSell = std::min(-posChange, position);
						auto id = sell(shortPrice, posSell);
						m_orderList.insert(id);
					}
					else
					{
						auto posShort = posChange;
						auto id = buy(shortPrice, posShort);
						m_orderList.insert(id);
					}
				}
			}
		}
	}

    virtual void onTick(TickPtr tick)
	{
		m_lastTick = tick;
		if(m_running)
			trade();
	}
    virtual void onOrder(OrderPtr o)
	{
		if(o->status == STATUS_ALLTRADED || O->status == STATUS_CANCELLED)
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
	
}


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

class Bar
{
	public:
	time_t datetime;

	public:
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
			volumeChange = tick.ttv - m_lastTick->ttv;
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
			m_minuteBar->volume += bar->volume;
		}
		
		struct tm* lt = localtime(&(m_xminuteBar->datetime));
		if((lt->tm_min + 1) % m_xminute == 0)
		{
			m_xminuteBar->datetime = m_xminuteBar->datetime / 60 * 60;

			std::shared_ptr<Bar> bar;
			std::swap(bar, m_xminuteBar);
			onXminuteBar(bar);
		}
	}

	void generate()
	{
		std::shared_ptr<Bar> bar;
		std::swap(bar, this->bar);
		onBar(bar)
	}
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
		TA_Initialize();
	}
	~ArrayManager()
	{
		TA_Shutdown();
	}


	public:
	void sma(n)
	{
		//return TA_SMA();
	}
};


