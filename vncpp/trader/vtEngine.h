#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include "eventEngine.h"
#include "vtObject.h"
#include "vtGateway.h"


class MainEngine
{
protected:
	EventEnginePtr eventEngine;
	DataEnginePtr dataEngine;
	RiskManagerEnginePtr rmEngine;
	std::map<std::string, GatewayPtr> gatewayDict;
	std::map<std::string, AppPtr> appDict;

	std::string todayDate;
	std::string dbClient;


public:
	MainEngine(EventEnginePtr ee)
		: eventEngine(ee)
		, dataEngine(std::make_shared<DataEngine>(ee))
	{
	}

	void addGateway(GatewayPtr gw)
	{
		gatewayDict[gw->getName()] = gw;
	}

	void addApp(AppPtr app)
	{
		appDict[app->getName()] = app;
	}

	GatewayPtr getGateway(const std::string& gwName)
	{
		auto i = gatewayDict.find(gwName);
		if(i != gatewayDict.end())
			return i->second;
		return GatewayPtr();
	}

	void connect(const std::string& gwName)
	{
		auto gw = getGateway(gwName);
		if(gw)
			gw->connect();
	}

	void subscribe(SubscribeRequestPtr req, const std::string& gwName)
	{
		auto gw = getGateway(gwName);
		if(gw)
			gw->subscribe(req);
	}

	OrderID sendOrder(OrderRequestPtr req, const std::string& gwName)
	{
		if(rmEngine && !rmEngine->checkRisk(req, gwName))
		{
			return OrderID();
		}

		auto gw = getGateway(gwName);
		if(gw)
		{
			auto orderID = gw->sendOrder(req);
			dataEngine->updateOrderReq(req, orderID);
			return orderID;
		}
		else
			return OrderID();
	}

	void cancelOrder(CancelOrderRequestPtr req, const std::string& gwName)
	{
		auto gw = getGateway(gwName);
		if(gw)
		{
			gw->cancelOrder(req);
		}
	}

	AccountPtr qryAccount(const std::string& gwName)
	{
		auto gw = getGateway(gwName);
		if(gw)
		{
			return gw->qryAccount();
		}
		else
			return AccountPtr();
	}

	std::vector<PositionPtr> qryPosition(const std::string& gwName)
	{
		auto gw = getGateway(gwName);
		if(gw)
		{
			return gw->qryPosition();
		}
		else
			return std::vector<PositionPtr>();
	}


	void exit()
	{
		for(auto i : gatewayDict)
		{
			i.second->close();
		}

		for(auto j : appDict)
		{
			i.second->stop();
		}

		dataEngine->saveContracts();

		eventEngine->stop();
	}
};

typedef std::shared_ptr<MainEngine> MainEnginePtr;

extern MainEnginePtr main_engine(EventEnginePtr p);


