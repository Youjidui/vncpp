#include "ctaTemplate.h"
#include "ctaEngine.h"


#if defined(_MSC_VER)
    //  Microsoft 
    #define API_EXPORT __declspec(dllexport)
    #define API_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define CLASS_EXPORT __attribute__((visibility("default")))
    #define API_EXPORT
    #define API_IMPORT
#else
    //  do nothing and hope for the best?
    #define API_EXPORT
    #define API_IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif


const char* paramList[] =
{
    "name",
    "className",
    "author",
    "vtSymbol",
    "kkLength",
    "kkDev"
};

const char* varList[] =
{
    "inited",
    "trading",
    "pos",
    "kkUp",
    "kkDown"
};

const char* syncList[] =
{
    "pos",
    "intraTradeHigh",
    "intraTradeLow"
};


class StrategyKingKeltner : public Strategy
{
    std::shared_ptr<BarGenerator> bg;
    std::shared_ptr<ArrayManager> am;

    int kkLength;
    double kkDev;
    double trailingPrcnt;
    int initDays;
    int fixedSize;

    int kkUp;
    int kkDown;
    double intraTradeHigh;
    double intraTradeLow;

    //OcoOrder, One Cancel Another
    OrderID buyOrderID;
    OrderID shortOrderID;
    std::vector<OrderID> orderList;

public:
    static const std::string className; 
	virtual const std::string getClassName() { return className; }

public:
    StrategyKingKeltner(const std::string& instanceName, CtaEngine& e)
    : Strategy(instanceName, e)
    {
		LOG_DEBUG << __FUNCTION__;
        kkLength = 11;
        kkDev    = 1.6;
        trailingPrcnt = 0.8;
        initDays = 10;
        fixedSize = 1;

        kkUp = 0;
        kkDown = 0;
        intraTradeHigh = 0;
        intraTradeLow = 0;
    }

public:
	virtual bool init()
	{
		LOG_DEBUG << __FUNCTION__;
        bg = std::make_shared<BarGenerator>();
        am = std::make_shared<ArrayManager>();
		return true;
    }
	virtual void onInit()
    {
		LOG_DEBUG << __FUNCTION__;
        //auto initData = loadBar(initDays);
        //for(auto i : initData)
        //{
        //    onBar(i);
        //}
		loadBar(initDays, std::bind(&StrategyKingKeltner::onBar, this, std::placeholders::_1));
    }
	virtual bool start()
    {
		LOG_DEBUG << __FUNCTION__;
		return true;
	}
	virtual void onStart()
    {
		LOG_DEBUG << __FUNCTION__;
	}
	virtual void stop()
    {
		LOG_DEBUG << __FUNCTION__;
	}
	virtual void onStop()
    {
		LOG_DEBUG << __FUNCTION__;
	}

    virtual void onTick(TickPtr t)
    {
		LOG_DEBUG << __FUNCTION__;
        bg->updateTick(t);
    }
    virtual void onBar(BarPtr b)      //K-line
    {
		LOG_DEBUG << __FUNCTION__;
        bg->updateBar(b);
    }

    void onFiveBar(BarPtr b)
    {
		LOG_DEBUG << __FUNCTION__;
        for(auto i : orderList)
        {
            cancelOrder(i);
        }
        orderList.clear();

        am->updateBar(b);

        am->keltner(kkLength, kkDev);

        if(position == 0)
        {
            intraTradeHigh = b->high;
            intraTradeLow = b->low;
            sendOcoOrder(kkUp, kkDown, fixedSize);
        }
        else if(position > 0)
        {
            intraTradeHigh = std::max(intraTradeHigh, b->high);
            intraTradeLow = b->low;
            auto id = sell(intraTradeHigh * (1 - trailingPrcnt/100), position, true);
            orderList.push_back(id);
        }
        else if(position < 0)
        {
            intraTradeHigh = b->high;
            intraTradeLow = std::min(intraTradeLow, b->low);
            auto id = cover(intraTradeLow * (1 + trailingPrcnt/100), -position, true);
            orderList.push_back(id);
        }

        saveSyncData();

        //putEvent();
    }

    virtual void onOrder(OrderPtr)
    {
		LOG_DEBUG << __FUNCTION__;
	}

    virtual void onTrade(TradePtr t)
    {
		LOG_DEBUG << __FUNCTION__;
        if(position != 0)
        {
            if(position > 0)
            {
                cancelOrder(shortOrderID);
            }
            else if(position < 0)
            {
                cancelOrder(buyOrderID);
            }

            //{
            //    //auto i = std::find(orderList.cbegin(), orderList.cend(), shortOrderID);
            //    //if(i != orderList.cend())
            //    //    orderList.erase(i);
            //    orderList.erase(std::remove(orderList.begin(), orderList.end(), shortOrderID), orderList.end());
            //}
            orderList.erase(std::remove_if(orderList.begin(), orderList.end(), 
            [this](const OrderID& id)
            {
                return (id == shortOrderID) || (id == buyOrderID);
            }
            ), orderList.end());
        }

        //putEvent();
    }

    void sendOcoOrder(double buyPrice, double shortPrice, int volume)
    {
		LOG_DEBUG << __FUNCTION__;
        auto buyOrderID = buy(buyPrice, volume, true);
        auto shortOrderID = Short(shortPrice, volume, true);
        orderList.push_back(buyOrderID);
        orderList.push_back(shortOrderID);
    }

    virtual void onStopOrder(StopOrderPtr)
    {
		LOG_DEBUG << __FUNCTION__;
	}
};


extern "C"
{

Strategy* API_EXPORT createStrategyInstance(const char* aInstanceName, const char* aStrategyClassName, 
CtaEngine* engine)
{
	LOG_DEBUG << __FUNCTION__;
    if(engine == NULL)
        return NULL;

    if(StrategyKingKeltner::className == aStrategyClassName)
    {
        auto s = new StrategyKingKeltner(aInstanceName, *engine);
        return s;
    }
    return NULL;
}

void API_EXPORT destroyStrategyInstance(Strategy* aStrategyInstance)
{
	LOG_DEBUG << __FUNCTION__;
    delete aStrategyInstance;
}

}



const std::string StrategyKingKeltner::className = "StrategyKingKeltner";

