#include <stdint.h>
#include <memory>
#include <functional>


class ctaEngine
{
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
}


