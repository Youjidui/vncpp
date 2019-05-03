#include <stdint.h>
#include <memory>
#include <functional>
#include <list>
#include <map>
#include "log.h"
#include "eventEngine.h"


class StopOrder
{
	public:
	std::string vtSymbol;

};

typedef std::shared_ptr<StopOrder> StopOrderPtr;

class Strategy
{
	public:
};

typedef std::shared_ptr<Strategy> StrategyPtr;

class CtaEngine
{
	public:
		typedef std::string Symbol;
		std::map<Symbol, StopOrderPtr> m_workingStopOrderDict;
		std::map<Symbol, StrategyPtr> m_tickStrategyDict;
		typedef std::string StrategyName;
		typedef std::string StopOrderID;
		std::map<StrategyName, StopOrderID>  m_strategyOrderDict;
		typedef std::list<StrategyPtr> StrategyList;
		std::map<Symbol, StrategyList> m_tickStrategyDict;
		typedef std::string OrderID;
		std::map<OrderID, StrategyPtr> m_orderStrategyDict;

	public:
	//return a list of order ID
		const std::vector<uint32_t> sendOrder(std::sting const& vtSymbol, int orderType, double price, int volume, uint32_t strategy)
	{

	}

		void cancelOrder(std::vector<uint32_t> const& vOrderID)
		{
		}

		const std::vector<uint32_t> sendStopOrder(std::string const& vtSymbol， int orderType, double price, double stopPrice, int volume, uint32_t strategy)
		{
		}

		void cancelStopOrder(std::vector<uint32_t> const& vOrderID)
		{
			cancelOrder(vOrderID);
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
				m_ee->emit(std::bind(Strategy::onTick, i, tick));
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
			m_ee->emit(std::bind(Strategy::onOrder, it->second, order));
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

			m_ee->emit(std::bind(Strategy::onTrade, s, trade));
		}
	}
}


