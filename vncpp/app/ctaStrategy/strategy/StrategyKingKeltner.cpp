#include "ctaTemplate.h"
#include "ctaEngine.h"


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
    int intraTradeHigh;
    int intraTradeLow;

    std::vector<OrderID> buyOrderIDList;
    std::vector<OrderID> shortOrderIDList;
    std::vector<OrderPtr> orderList;

public:
    static const std::string className = "StrategyKingKeltner";
	virtual const std::string getClassName() { return className; }

public:
    StrategyKingKeltner(const std::string& instanceName, CtaEngine& e)
    : Strategy(instanceName, e)
    {
        kkLength = 11;
        kkDev    = 1.6;
        trailingPrcnt = 0.8;
        initDays = 10;
        fixedSize = 1;

        kkUp = 0;
        kkDow = 0;
        intraTradeHigh = 0;
        intraTradeLow = 0;
    }

public:
	virtual bool init(const boost::property_tree::ptree* aParameters)
    {
        parameters = aParameters;

        bg = std::make_shared<BarGenerator>();
        am = std::make_shared<ArrayManager>();
    }
	virtual void onInit()
    {
        auto initData = loadBar(initDays);
        for(auto i : initData)
        {
            onBar(i);
        }
    }
	virtual bool start()
    {}
	virtual void onStart()
    {}
	virtual void stop()
    {}
	virtual void onStop()
    {}

    virtual void onTick(TickPtr)
    {}
    virtual void onOrder(OrderPtr)
    {}
    virtual void onTrade(TradePtr)
    {}
    virtual void onBar(BarPtr)      //K-line
    {}
    virtual void onStopOrder(StopOrderPtr)
    {}
};


extern "C"
{
Strategy* createStrategyInstance(const char* aInstanceName, const char* aStrategyClassName, 
CtaEngine* engine)
{
    if(engine == NULL)
        return NULL;

    if(StrategyKingKeltner::className == aStrategyClassName)
    {
        auto s = new StrategyKingKeltner(aInstanceName, &engine);
        return s;
    }
    return NULL;
}

void destroyStrategyInstance(Strategy* aStrategyInstance)
{
    delete aStrategyInstance;
}

}

