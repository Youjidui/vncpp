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
#include "eventEngine.h"
#include "algoTemplate.h"


class AlgoEngine
{
protected:
	EventEnginePtr m_ee;
    mainEngine;
    rpcServer;

    std::map<std::string, AlgoTemplatePtr> algoDict;
    std::map<OrderID, AlgoTemplatePtr> orderAlgoDict;
    std::multimap<Symbol, AlgoTemplatePtr> symbolAlgoDict;
    settingDict;
    historyDict;

public:
    AlgoEngine(EventEnginePtr ee)
    : m_ee(ee)
    {
        registerEvent();
    }

	void registerEvent()
	{
		m_ee->register(EVENT_TICK, std::bind(&AlgoEngine::processTickEvent, this, std::placeholders::_1));
		m_ee->register(EVENT_ORDER, std::bind(&AlgoEngine::processOrderEvent, this, std::placeholders::_1));
		m_ee->register(EVENT_TRADE, std::bind(&AlgoEngine::processTradeEvent, this, std::placeholders::_1));
        m_ee->register(EVENT_TIMER, std::bind(&AlgoEngine::processTimerEvent, this, std::placeholders::_1));
	}

    void stop()
    {
        if(rpcServer)
        {
            rpcServer->stop();
        }
    }

	void processTickEvent(Event e)
	{
		auto tick = e.dict;
        auto i = symbolAlgoDict.find(tick->vtSymbol);
        if(i != symbolAlgoDict.end())
        {
            for(auto j : i->second)
            {
                j->updateTick(tick);
            }
        }
	}

    void processOrderEvent(Event e)
    {
        auto o = e.dict;
        auto i = orderAlgoDict.find(o->vtOrderID);
        if(i != orderAlgoDict.end())
        {
            i.second->updateOrder(o);
        }
    }

    void processTradeEvent(Event e)
    {
        auto t = e.dict;
        auto i = orderAlgoDict.find(t->vtOrderID);
        if(i != orderAlgoDict.end())
        {
            i.second->updateOrder(t);
        }
    }

    void processTimerEvent(Event e)
    {
        for(auto i : algoDict)
        {
            i->updateTimer();
        }
    }

    std::string addAlgo(boost::property_tree::ptree& parameter)
    {
        auto className = parameter.get<std::string>("className");
        AlgoTemplatePtr algo = createAlgo(className);
        algo->setParameter(parameter);
        algoDict[algo->name] = algo;
        return algo->name;
    }

    void stopAlgo(const std::string& algoName)
    {
        auto i = algoDict.find(algoName);
        if(i != algoDict.end())
        {
            i->second->stop();
            alogDict.erase(i);
        }
    }

    void stopAll()
    {
        for(auto i : algoDict)
        {
            i.second->stop();
        }
        algoDict.clear();
    }

    void subscribe(AlgoTemplatePtr algo, const std::string& vtSymbol)
    {
        auto contract = mainEngine->getContract(vtSymbol);
        if(!contract)
            return;

        auto i = symbolAlgoDict.find(vtSymbol);
        if(i != symbolAlgoDict.end())
        {
            auto req = std::make_shared<VtSubscribeReq>();
            req->symbol = contract->symbol;
            req->exchange = contract->exchange;
            mainEngine->subscribe(req, contract->gatewayName);
        }
        symbolAlgoDict.insert(std::make_pair(vtSymbol, algo));
    }
}


