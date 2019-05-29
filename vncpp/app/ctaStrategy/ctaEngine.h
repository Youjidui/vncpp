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
//#include "vtPersistence.h"
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
		registerEvent();
	}

	int getEngineType()
	{
		return engineType;
	}

public:
	OrderID sendOrder(std::string const& vtSymbol, int orderType, double price, int volume, StrategyPtr strategy)
	{
	}

	void cancelOrder(OrderID const& vOrderID)
	{
	}

	OrderID sendStopOrder(std::string const& vtSymbol, int orderType, double price, int volume, StrategyPtr strategy)
	{
	}

	void cancelStopOrder(StopOrderID const& vOrderID)
	{
		cancelOrder(vOrderID);
	}

	void cancelAll(const std::string& aStrategyInstanceName)
	{
		auto i = m_strategyOrderDict.find(aStrategyInstanceName);
		if(i != m_strategyOrderDict.end())
		{}
	}

	void processStopOrder(TickPtr& tick)
	{
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
		m_ee->register_(EVENT_TICK, std::bind(&CtaEngine::processTickEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_ORDER, std::bind(&CtaEngine::processOrderEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_TRADE, std::bind(&CtaEngine::processTradeEvent, this, std::placeholders::_1));
	}

	void insertData(const std::string& dbName, const std::string& vtSymbol, BarPtr) 
	{
	}

	void insertData(const std::string& dbName, const std::string& vtSymbol, TickPtr) 
	{
	}

	void loadBar(const std::string& dbName, const std::string& vtSymbol, int days)
	{
	}

	void loadTick(const std::string& dbName, const std::string& vtSymbol, int days)
	{
	}

	void writeCtaLog(std::string&& logContent)
	{
		auto e = makeLogEvent("CTA_STRATEGY", std::move(logContent));
		m_ee->emit(e);
	}

	//loadStrategy
	//each strategy instance should have a instance name, different than strategy name (more like a strategy class name)
	void setupStrategy(StrategyPtr s, const std::string& vtSymbol)
	{
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
		for(auto i : m_strategyDict)
		{
			initStrategy(i.first);
		}
	}

	void startAll()
	{
		for(auto i : m_strategyDict)
		{
			startStrategy(i.first);
		}
	}

	void stopAll()
	{
		for(auto i : m_strategyDict)
		{
			stopStrategy(i.first);
		}
	}

	void saveSettings(const std::string& filePath)
	{
		boost::property_tree::ptree ptall;
		for(auto i : m_strategyDict)
		{
			ptall.put_child(i.first, *(i.second->parameters));
		}
		boost::property_tree::json_parser::write_json(filePath, ptall);
	}

	void loadSettings(const std::string& filePath)
	{
		boost::property_tree::ptree ptall;
		boost::property_tree::json_parser::read_json(filePath, ptall);

		for(auto i : m_strategyDict)
		{
			//the first level of the .json is the strategy name
			//the second level is the parameters
			auto& p = ptall.get_child(i.first);
			i.second->setParameter(&p);
		}
	}

	void saveSyncData(StrategyPtr s)
	{
		
	}

	void loadSyncData(StrategyPtr s)
	{

	}

	double getPriceTick(const std::string& vtSymbol)
	{
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

