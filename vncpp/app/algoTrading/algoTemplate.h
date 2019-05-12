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

    virtual onTick(TickPtr t) = 0;
    virtual onTrade(TradePtr t) = 0;
    virtual onOrder(OrderPtr o) = 0;
    virtual onTimer() =0;
    virtual onStop() = 0;

    void subscribe(const std::string& vtSymbol)
    {
        m_engine->subscribe(vtSymbol);
    }

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

    void stop()
    {
        active = false;
        cancelAll();

        onStop();
    }
};

