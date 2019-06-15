#pragma once
#include <stdint.h>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
//#include <iostream>
#include <set>
#include "logging.h"
#include "eventEngine.h"
#include "vtEngine.h"
#include "vtPersistence.h"
#include "ctaTemplate.h"
#include "vtApp.h"


enum ENGINE_TYPE
{
	ENGINETYPE_TRADING = 0,
	ENGINETYPE_BACKTESTING
};


class CtaEngine : public App
{
protected:
	EventEnginePtr m_ee;
	MainEnginePtr mainEngine;
	int engineType;
	CtaStrategyConfigurationManager m_configManager;

    PersistenceEnginePtr m_pe;
    HistoryDataEnginePtr m_history;


public:
	std::map<Symbol, StopOrderPtr> m_workingStopOrderDict;
	typedef std::string StrategyName;
	typedef std::set<StopOrderID> OrderIDSet;
	std::map<StrategyName, OrderIDSet>  m_strategyOrderDict;
	typedef std::vector<StrategyPtr> StrategyList;
	std::map<Symbol, StrategyList> m_tickStrategyDict;
	std::map<OrderID, StrategyPtr> m_orderStrategyDict;
	typedef std::string StrategyInstanceName;
	std::map<StrategyInstanceName, StrategyPtr> m_strategyDict;
	std::set<std::string> m_tradeSet;

public:
	CtaEngine(EventEnginePtr ee, MainEnginePtr m) 
		: App("CTA", "CTA Strategy") 
		, m_ee(ee), mainEngine(m)
	{
		LOG_DEBUG << __FUNCTION__;
		registerEvent();
	}

	int getEngineType()
	{
		return engineType;
	}

	//init HistoryDataServic client
    bool initHdsClient(PersistenceEnginePtr p)
    {
		LOG_DEBUG << __FUNCTION__;
        //auto reqAddress = "tcp://localhost:5555";
        //auto subAddress = "tcp://localhost:7777";
        //hdsClient = std::make_shared<RpcClient>(reqAddress, subAddress);
        //hdsClient->start();
		m_pe = p;
		m_history = std::make_shared<HistoryDataEngine>(p);
		return true;
    }

protected:


public:
	OrderID sendOrder(std::string const& vtSymbol, int orderType, double price, int volume, StrategyPtr strategy)
	{
		LOG_DEBUG << __FUNCTION__;
		return OrderID();
	}

	void cancelOrder(OrderID const& vOrderID)
	{
		LOG_DEBUG << __FUNCTION__;
	}

	OrderID sendStopOrder(std::string const& vtSymbol, int orderType, double price, int volume, StrategyPtr strategy)
	{
		LOG_DEBUG << __FUNCTION__;
		return OrderID();
	}

	void cancelStopOrder(StopOrderID const& vOrderID)
	{
		LOG_DEBUG << __FUNCTION__;
		cancelOrder(vOrderID);
	}

	void cancelAll(const std::string& aStrategyInstanceName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto i = m_strategyOrderDict.find(aStrategyInstanceName);
		if(i != m_strategyOrderDict.end())
		{}
	}

	void processStopOrder(TickPtr& tick)
	{
		LOG_DEBUG << __FUNCTION__;
		auto& vtSymbol = tick->vtSymbol;
		auto it = m_tickStrategyDict.find(vtSymbol);
		if(it != m_tickStrategyDict.cend())
		{
			for(auto i = m_workingStopOrderDict.begin(); i != m_workingStopOrderDict.end(); )
			{
				auto& so = i->second;
				if(so->vtSymbol == vtSymbol)
				{
					auto longSO = so->direction == DIRECTION_LONG;
					auto shortSO = so->direction == DIRECTION_SHORT; 
					auto longTrigged = longSO && tick->lastPrice >= so->stopPrice;
					auto shortTrigged = shortSO && tick->lastPrice <= so->stopPrice;

					if(longTrigged || shortTrigged)
					{
						double price = 0;
						if(longSO)
						{
							if (tick->upperLimit != DBL_MIN)
								price = tick->upperLimit;
							else
								price = tick->askPrice[0] + getPriceTick(tick->vtSymbol) * 5;
						}
						else
						{
							if(tick->lowerLimit != DBL_MIN)
								price = tick->lowerLimit;
							else
								price = tick->bidPrice[0] - getPriceTick(tick->vtSymbol) * 5;
						}
						
						auto vOrderID = sendOrder(so->vtSymbol, so->orderType, price, so->volume, so->strategy);
						if(!vOrderID.empty())
						{
							i = m_workingStopOrderDict.erase(i);

							auto s = m_strategyOrderDict.find(so->strategy->name);
							if(s != m_strategyOrderDict.end())
							{
								s->second.erase(so->stopOrderID);
							}
							so->status = STOPORDER_TRIGGERED;
							so->strategy->onStopOrder(so);
						}
						else 
						{
							++i;
						}
					}
				}
			}
		}
	}

	void processTickEvent(Event e)
	{
		LOG_DEBUG << __FUNCTION__;
		auto tick = std::dynamic_pointer_cast<Tick>(e.dict);
		processStopOrder(tick);

		auto it = m_tickStrategyDict.find(tick->vtSymbol);
		if(it != m_tickStrategyDict.end())
		{
			auto& sl = it->second;
			for(auto i : sl)
			{
				m_ee->post(std::bind(&Strategy::onTick, i, tick));
			}
		}
	}

	void processOrderEvent(Event e)
	{
		LOG_DEBUG << __FUNCTION__;
		auto order = std::dynamic_pointer_cast<Order>(e.dict);
		auto& vtOrderID = order->vtOrderID;
		auto it = m_orderStrategyDict.find(vtOrderID);
		if(it != m_orderStrategyDict.end())
		{
			if(order->isActive() == false)
			{
				auto i = m_strategyOrderDict.find(it->second->name);
				if(i != m_strategyOrderDict.end())
				{
					i->second.erase(vtOrderID);
				}
			}
			m_ee->post(std::bind(&Strategy::onOrder, it->second, order));
		}

	}

	void processTradeEvent(Event e)
	{
		LOG_DEBUG << __FUNCTION__;
		auto trade = std::dynamic_pointer_cast<Trade>(e.dict);
		auto it = m_tradeSet.find(trade->vtTradeID);
		if(it != m_tradeSet.end())
			return;

		m_tradeSet.insert(trade->vtTradeID);
		auto ito = m_orderStrategyDict.find(trade->vtOrderID);
		if(ito != m_orderStrategyDict.end())
		{
			auto& s = ito->second;
			auto q = trade->volume;
			if(trade->direction == DIRECTION_SHORT)
				q = -q;
			s->position += trade->volume;

			m_ee->post(std::bind(&Strategy::onTrade, s, trade));
		}
	}

public:

	void registerEvent()
	{
		LOG_DEBUG << __FUNCTION__;
		m_ee->register_(EVENT_TICK, std::bind(&CtaEngine::processTickEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_ORDER, std::bind(&CtaEngine::processOrderEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_TRADE, std::bind(&CtaEngine::processTradeEvent, this, std::placeholders::_1));
	}

public:
	//DB
	//should be removed	
	/*
	void insertData(const std::string& dbName, const std::string& vtSymbol, BarPtr) 
	{
		LOG_DEBUG << __FUNCTION__;
	}

	void insertData(const std::string& dbName, const std::string& vtSymbol, TickPtr) 
	{
		LOG_DEBUG << __FUNCTION__;
	}

	std::vector<BarPtr> loadBar(const std::string& dbName, const std::string& vtSymbol, int days)
	{
		LOG_DEBUG << __FUNCTION__;
		return std::vector<BarPtr>();
	}

	std::vector<TickPtr> loadTick(const std::string& dbName, const std::string& vtSymbol, int days)
	{
		LOG_DEBUG << __FUNCTION__;
		return std::vector<TickPtr>();
	}
	*/ 

	void insertData(const std::string& dbName, const std::string vtSymbol, const Record& r) 
	{
		LOG_DEBUG << __FUNCTION__;
		m_pe->dbInsert(dbName, r);
	}

	void loadHistoryData(const std::string& dbName, const std::string& vtSymbol, int days,
		std::function<void (const Record&)> onRecord)
	{
		LOG_DEBUG << __FUNCTION__;

		time_t now = time(NULL);
		auto dateEndDate = now;		//now/(24 * 60 * 60);
		auto strategyStartDate = now - (days * 24 * 60 * 60);
		m_history->loadHistoryData(dbName, vtSymbol, strategyStartDate, dateEndDate, onRecord);
	}

	//should be removed
	void writeCtaLog(std::string&& logContent)
	{
		LOG_DEBUG << __FUNCTION__;
		auto e = makeLogEvent("CTA_STRATEGY", std::move(logContent));
		m_ee->emit(e);
	}

	//loadStrategy
	//each strategy instance should have a instance name, different than strategy name (more like a strategy class name)
	
	void setupStrategy(StrategyPtr s, const std::string& vtSymbol)
	{
		LOG_DEBUG << __FUNCTION__;
		m_strategyDict[s->name] = s;
		m_strategyOrderDict[s->name] = OrderIDSet();
		auto it = m_tickStrategyDict.find(s->vtSymbol);
		if(it == m_tickStrategyDict.end())
		{
			StrategyList l;
			l.push_back(s);
			m_tickStrategyDict.insert(std::make_pair(s->vtSymbol, l));
		}
		else
		{
			it->second.push_back(s);
		}
	}

	//is this function necessary? should be merged into startStrategy()
	void initStrategy(const std::string& name)
	{
		LOG_DEBUG << __FUNCTION__;
		auto i = m_strategyDict.find(name);
		if(i != m_strategyDict.cend())
		{
			auto& s = i->second;
			s->init();
			loadSyncData(s);
			subscribeMarketData(s);

			m_ee->post(std::bind(&Strategy::onInit, s));
		}
	}

	void startStrategy(const std::string& name)
	{
		LOG_DEBUG << __FUNCTION__;
		auto i = m_strategyDict.find(name);
		if(i != m_strategyDict.cend())
		{
			auto& s = i->second;
			s->start();
			//do something
			m_ee->post(std::bind(&Strategy::onStart, s));
		}
	}

	void stopStrategy(const std::string& name)
	{
		LOG_DEBUG << __FUNCTION__;
		auto i = m_strategyDict.find(name);
		if(i != m_strategyDict.cend())
		{
			auto& s = i->second;
			s->stop();

			for(auto j : m_orderStrategyDict)
			{
				if(j.second.get() == s.get())
				{
					cancelOrder(j.first);
				}
			}

			for(auto k : m_workingStopOrderDict)
			{
				if(k.second->strategy->name == s->name)
				{
					cancelStopOrder(k.first);
				}
			}

			m_ee->post(std::bind(&Strategy::onStop, s));
		}
	}

	void subscribeMarketData(StrategyPtr s)
	{
		LOG_DEBUG << __FUNCTION__;
		auto c = mainEngine->getContract(s->vtSymbol);
		if(c)
		{
			auto req = std::make_shared<SubscribeRequest>(
				c->symbol, c->exchange, s->currency.empty() ? c->currency : s->currency,
				s->productClass
			);
			mainEngine->subscribe(req, c->gatewayName);
		}
		else
		{
			//writeCtaLog()
			LOG_ERROR << "Cannot find contract " << s->vtSymbol << " for strategy " << s->name;
		}
	}

	void initAll()
	{
		LOG_DEBUG << __FUNCTION__;
		for(auto i : m_strategyDict)
		{
			initStrategy(i.first);
		}
	}

	void startAll()
	{
		LOG_DEBUG << __FUNCTION__;
		for(auto i : m_strategyDict)
		{
			startStrategy(i.first);
		}
	}

	void stopAll()
	{
		LOG_DEBUG << __FUNCTION__;
		for(auto i : m_strategyDict)
		{
			stopStrategy(i.first);
		}
	}

	void saveSettings(const std::string& filePath)
	{
		LOG_DEBUG << __FUNCTION__;
		m_configManager.saveConfiguration(filePath, m_strategyDict);
	}

	void loadSettings(const std::string& filePath)
	{
		LOG_DEBUG << __FUNCTION__;
		m_configManager.loadConfiguration(filePath);
		m_configManager.loadStrategies(*this, this->m_strategyDict);
	}

	void saveSyncData(StrategyPtr s)
	{
		LOG_DEBUG << __FUNCTION__;
		
	}

	void loadSyncData(StrategyPtr s)
	{
		LOG_DEBUG << __FUNCTION__;

	}

	double getPriceTick(const std::string& vtSymbol)
	{
		LOG_DEBUG << __FUNCTION__;
		auto c = mainEngine->getContract(vtSymbol);
		if(c)
		{
			return c->priceTick;
		}
		return 0;
	}
};

typedef std::shared_ptr<CtaEngine> CtaEnginePtr;

extern CtaEnginePtr ctaEngine(EventEnginePtr e, MainEnginePtr m);

