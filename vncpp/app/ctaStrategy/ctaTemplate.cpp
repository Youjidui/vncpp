#include <string.h>
#include "ctaTemplate.h"
#include "ctaEngine.h"

const std::string stopOrderPrefix("STOP_");

inline bool isStopOrderID(const OrderID& id)
{
	return (strncmp(stopOrderPrefix.c_str(), id.c_str(), stopOrderPrefix.length()) == 0);
}

OrderID Strategy::sendOrder(int orderType, double price, int volume, bool stopOrder)
{
	if(stopOrder)
		return m_ctaEngine.sendStopOrder(vtSymbol, orderType, price, volume, shared_from_this());
	else
		return m_ctaEngine.sendOrder(vtSymbol, orderType, price, volume, shared_from_this());

}

void Strategy::cancelOrder(const OrderID& vtOrderID)
{
	if(isStopOrderID(vtOrderID))
		m_ctaEngine.cancelStopOrder(vtOrderID);
	else
		m_ctaEngine.cancelOrder(vtOrderID);
}

void Strategy::cancelAll()
{
	m_ctaEngine.cancelAll(name);
}


void Strategy::insertTick(TickPtr tick)
{
	m_ctaEngine.insertData(tickDbName, vtSymbol, tick);
}
void Strategy::insertBar(BarPtr bar)
{
	m_ctaEngine.insertData(barDbName, vtSymbol, bar);
}
std::vector<TickPtr> Strategy::loadTick(int days)
{
	return m_ctaEngine.loadTick(tickDbName, vtSymbol, days);
}
std::vector<BarPtr> Strategy::loadBar(int days)
{
	return m_ctaEngine.loadBar(barDbName, vtSymbol, days);
}

void Strategy::saveSyncData()
{
	m_ctaEngine.saveSyncData(shared_from_this());
}

double Strategy::getPriceTick()
{
	return m_ctaEngine.getPriceTick(vtSymbol);
}

int Strategy::getEngineType()
{
	return m_ctaEngine.getEngineType();
}



void TargetPosTemplate::trade()
{
	if(!m_orderList.empty())
		cancelAll();
	else
	{
		int posChange = m_targetPosition - position;
		if(posChange == 0)
			return;

		double longPrice = 0;
		double shortPrice = 0;

		if(m_lastTick)
		{
			if(posChange > 0)
			{
				longPrice = m_lastTick->askPrice[0] + m_tickAdd;
				if(m_lastTick->upperLimit)
					longPrice = std::min(longPrice, m_lastTick->upperLimit);
			}
			else
			{
				shortPrice = m_lastTick->bidPrice[0] - m_tickAdd;
				if(m_lastTick->lowerLimit)
					shortPrice = std::max(shortPrice, m_lastTick->lowerLimit);
			}
		}
		else if(m_lastBar)
		{
			if(posChange > 0)
				longPrice = m_lastBar->close + m_tickAdd;
			else
				shortPrice = m_lastBar->close - m_tickAdd;
		}
		else
		{
			//do nothing
		}


		if(getEngineType() == ENGINETYPE_BACKTESTING)
		{
			if(posChange > 0)
			{
				auto id = buy(longPrice, posChange);
				m_orderList.insert(id);
			}
			else
			{
				auto id = Short(shortPrice, -posChange);
				m_orderList.insert(id);
			}
		}
		else
		{
			if(posChange > 0)
			{
				if(position < 0)
				{
					auto posCover = std::min(posChange, -position);
					auto id = cover(longPrice, posCover);
					m_orderList.insert(id);
				}
				else
				{
					auto posBuy = posChange;
					auto id = buy(longPrice, posBuy);
					m_orderList.insert(id);
				}
			}
			else
			{
				if(position > 0)
				{
					auto posSell = std::min(-posChange, position);
					auto id = sell(shortPrice, posSell);
					m_orderList.insert(id);
				}
				else
				{
					auto posShort = posChange;
					auto id = buy(shortPrice, posShort);
					m_orderList.insert(id);
				}
			}
		}
	}
}



