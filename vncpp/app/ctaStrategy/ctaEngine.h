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
#include "ctaTemplate.h"



class CtaEngine
{
	protected:
		EventEnginePtr m_ee;

	public:
		typedef std::string Symbol;
		std::map<Symbol, StopOrderPtr> m_workingStopOrderDict;
		typedef std::string StrategyName;
		typedef std::string StopOrderID;
		std::map<StrategyName, StopOrderID>  m_strategyOrderDict;
		typedef std::list<StrategyPtr> StrategyList;
		std::map<Symbol, StrategyList> m_tickStrategyDict;
		typedef std::string OrderID;
		std::map<OrderID, StrategyPtr> m_orderStrategyDict;
		typedef std::string StrategyInstanceName;
		std::map<StrategyInstanceName, StrategyPtr> m_strategyDict;
		std::string m_settingfilePath;

	public:
		CtaEngine(EventEnginePtr ee) : m_ee(ee) {}

	protected:
	OrderID sendOrder(std::sting const& vtSymbol, int orderType, double price, int volume, StrategyPtr strategy)
	{
	}

	void cancelOrder(OrderID const& vOrderID)
	{
	}

	OrderID sendStopOrder(std::string const& vtSymbol， int orderType, double price, double stopPrice, int volume, StrategyPtr strategy)
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
						if(longSO)
						{
						}
						else
						{
						}
						
						auto vOrderID = sendOrder(so->vtSymbol, so->orderType, so->price, so->volume, so->strategy);
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
		auto tick = e.dict;
		processStopOrder(tick);

		auto it = m_tickStrategyDict.find(tick->vtSymbol);
		if(it != m_tickStrategyDict.end())
		{
			auto& sl = i->second;
			for(auto i : sl)
			{
				m_ee->post(std::bind(Strategy::onTick, i, tick));
			}
		}
	}

	void processOrderEvent(Event e)
	{
		auto order = e.dict;
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
			m_ee->post(std::bind(Strategy::onOrder, it->second, order));
		}

	}

	void processTradeEvent(Event e)
	{
		auto trade = e.dict;
		auto it = m_tradeSet.find(trade->vtTradeID);
		if(it != m_tradeSet.end())
			return;

		m_tradeSet.insert(trade->vtTradeID);
		auto ito = m_orderStrategyDict.find(trade->vtOrderID);
		if(ito != m_orderStrategyDict.end())
		{
			auto& s = ito->second;
			q = trade->volume;
			if(trade->direction == DIRECTION_SHORT)
				q = -q;
			s->pos += trade->volume;

			m_ee->post(std::bind(Strategy::onTrade, s, trade));
		}
	}

	public:

	void registerEvent()
	{
		m_ee->register(EVENT_TICK, std::bind(&CtaEngine::processTickEvent, this, std::placeholders::_1));
		m_ee->register(EVENT_ORDER, std::bind(&CtaEngine::processOrderEvent, this, std::placeholders::_1));
		m_ee->register(EVENT_TRADE, std::bind(&CtaEngine::processTradeEvent, this, std::placeholders::_1));
	}

	void insertData() 
	{
	}

	void loadBar()
	{
	}

	void loadTick()
	{
	}

	void writeCtaLog(std::string&& logContent)
	{
		auto e = makeLogEvent("CTA_STRATEGY", logContent);
		m_ee->emit(e);
	}

	//loadStrategy
	//each strategy instance should have a instance name, different than strategy name (more like a strategy class name)
	void setupStrategy(StrategyPtr s, const std::string& vtSymbol)
	{
		m_strategyDict[s->name] = s;
		m_strategyOrderDict[s->name] = std::set<OrderID>();
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

			for(auto i : m_orderStrategyDict)
			{
				if(i->second == s)
				{
					cancelOrder(i->first);
				}
			}

			for(auto i : m_workingStopOrderDict)
			{
				if(i->second->strategy == s)
				{
					cancelStopOrder(i->first);
				}
			}

			m_ee->post(std::bind(&Strategy::onStop, s));
		}
	}

	void subscribeMarketData(StrategyPtr s)
	{
		auto c = m_mainEngine->getContract(s->vtSymbol);
		if(c)
		{
			auto req = std::make_shared<SubscribeRequest>(
				c->symbol, c->exchange, s->currency.empty() ? c->currency : s->currency,
				s->productClass
			);
			m_mainEngine->subscribe(req, c->getwayName);
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
			initStrategy(i->first);
		}
	}

	void saveSetting()
	{
		boost::property_tree::ptree ptall;
		for(auto i : m_strategyDict)
		{
			ptall.put_child(i->first, i->second->parameters);
		}
		boost::property_tree::json_parser::write_json(m_settingfilePath, ptall);
	}

	void loadSetting()
	{
		boost::property_tree::ptree ptall;
		boost::property_tree::json_parser::read_json(m_settingfilePath, ptall);

		for(auto i : m_strategyDict)
		{
			//i->second->parameters = ptall.get_child(i->first);
			auto& p = ptall.get_child(i->first);
			i->second->init(&p);
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
		auto c = m_mainEngine->getContract(vtSymbol);
		if(c)
		{
			return c->priceTick;
		}
		return 0;
	}
};

