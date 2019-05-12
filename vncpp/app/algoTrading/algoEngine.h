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



class AlgoEngine
{
protected:
	EventEnginePtr m_ee;
    mainEngine;
    rpcServer;

    algoDict;
    orderAlgoDict;
    std::map<Symbol, > symbolAlgoDict;
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
}
