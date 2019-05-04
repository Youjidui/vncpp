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
    Strategy(const std::string& instanceName, const std::string& aClassName, CtaEngine& e)
    : name(instanceName), className(aClassName) m_ctaEngine(e)
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


