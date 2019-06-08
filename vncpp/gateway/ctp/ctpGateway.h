#pragma once

#include <stdint.h>
#include <memory>
#include "vtGateway.h"

class CtpGateway : public Gateway
{
	public:
	virtual const std::string getClassName()
	{
		return "CTP Gateway";
	}

	CtpGateway(const std::string& aInstanceName, EventEngine& ee)
		: Gateway(aInstanceName, ee)
	{
	}

	virtual void connect()
	{}
	virtual void close()
	{}

	virtual void subscribe(SubscribeRequestPtr p)
	{}
	virtual OrderID sendOrder(OrderRequestPtr p)
	{
		return OrderID();
	}
	virtual void cancelOrder(CancelOrderRequestPtr p)
	{}

	virtual AccountPtr queryAccount()
	{
		return AccountPtr();
	}
	virtual std::vector<PositionPtr> queryPosition()
	{
		return std::vector<PositionPtr>();
	}
	//virtual void queryHistory(QueryHistoryRequestPtr p) = 0;
};

extern GatewayPtr ctpGateway(const std::string& aInstanceName, EventEnginePtr);

