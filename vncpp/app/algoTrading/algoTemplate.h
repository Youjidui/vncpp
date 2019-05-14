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


inline 
double roundValue(double value, double change)
{
    if(change > 0.00001 || change < -0.00001)
    {
        auto n = value / change;
        v = round(n, 0) * change;
        return v;
    }
    return value;
}


class AlgoEngine;

class AlgoTemplate
{
protected:
	const std::string name;		//the instance name (with different symbol)
	virtual const std::string getClassName() = 0;

    AlgoEngine& m_engine;
    boost::property_tree::ptree* parameters;

    bool active;
    std::map<OrderID, OrderPtr> activeOrderDict;

public:
    AlgoTemplate(const std::string& instanceName, AlgoEngine& engine)
    : name(instanceName)
    , m_engine(engine)
    , parameters(NULL)
    , active(false)
    {}


    virtual onTick(TickPtr t) = 0;
    virtual onTrade(TradePtr t) = 0;
    virtual onOrder(OrderPtr o) = 0;
    virtual onTimer() =0;
    virtual onStop() = 0;


	void setParameter(boost::property_tree::ptree* aParameters)
	{
		parameters = aParameters;
	}

    bool start()
    {
        if(parameters)
            active = true;
        return active;
    }

    void subscribe(const std::string& vtSymbol)
    {
        m_engine->subscribe(vtSymbol);
    }

    void stop()
    {
        active = false;
        cancelAll();

        onStop();
    }

public:
    void buy(const std::string& vtSymbol, double price, int volume,
    int priceType, int offset)
    {
        m_engine->buy(vtSymbol, price, volume, priceType, offset);
    }
    void sell(const std::string& vtSymbol, double price, int volume,
    int priceType, int offset)
    {
        m_engine->sell(vtSymbol, price, volume, priceType, offset);
    }
    void cancelOrder(const OrderID& vtOrderID)
    {
        m_engine->cancelOrder(vtOrderID);
    }
    void cancelAll()
    {
        if(!activeOrderDict.empty())
        {
            for(auto i : activeOrderDict)
            {
                cancelOrder(i.first);
            }
        }
    }

public:
    TickPtr getTick(const std::string& vtSymbol)
    {
        return m_engine->getTick(vtSymbol);
    }

    ContractPtr getContract(const std::string& vtSymbol)
    {
        return m_engine->getContract(vtSymbol);
    }

public:
    void updateTick(TickPtr t)
    {
        if(active)
            onTick(t);
    }

    void updateTrade(TradePtr t)
    {
        if(active)
            onTrade(t);
    }

    void updateOrder(OrderPtr o)
    {
        if(active)
        {
            if(o->isActive())
            {
                activeOrderDict[o->vtOrderID] = o;
            }
            else
            {
                auto i = activeOrderDict.find(o->vtOrderID);
                if(i != activeOrderDict.end())
                    activeOrderDict.erase(i);
            }

            onOrder(o);
        }
    }

    void updateTimer()
    {
        if(active)
            onTimer();
    }

};

typedef std::shared_ptr<AlgoTemplate> AlgoTemplatePtr;

