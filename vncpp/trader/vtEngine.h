#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include "eventEngine.h"
#include "vtObject.h"
#include "vtGateway.h"
#include "vtPersistence.h"
#include "vtApp.h"


class MainEngine
{
protected:
public:
	EventEnginePtr eventEngine;
	DataEnginePtr dataEngine;
	//RiskManagerEnginePtr rmEngine;
protected:
	std::map<std::string, GatewayPtr> gatewayDict;
	std::map<std::string, AppPtr> appDict;

	std::string todayDate;

    std::shared_ptr<boost::property_tree::ptree> parameters;


public:
	MainEngine(EventEnginePtr ee)
		: eventEngine(ee)
		, dataEngine(std::make_shared<DataEngine>(ee))
	{
		LOG_DEBUG << __FUNCTION__;
	}

	void addGateway(GatewayPtr gw)
	{
		LOG_DEBUG << __FUNCTION__;
		gatewayDict[gw->name] = gw;
	}

	void addApp(AppPtr app)
	{
		LOG_DEBUG << __FUNCTION__;
		appDict[app->name] = app;
	}

	GatewayPtr getGateway(const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto i = gatewayDict.find(gwName);
		if(i != gatewayDict.end())
			return i->second;
		return GatewayPtr();
	}

	void connect(const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto gw = getGateway(gwName);
		if(gw)
			gw->connect();
	}

	void subscribe(SubscribeRequestPtr req, const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto gw = getGateway(gwName);
		if(gw)
			gw->subscribe(req);
	}

	OrderID sendOrder(OrderRequestPtr req, const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		//if(rmEngine && !rmEngine->checkRisk(req, gwName))
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
		LOG_DEBUG << __FUNCTION__;
		auto gw = getGateway(gwName);
		if(gw)
		{
			gw->cancelOrder(req);
		}
	}

	AccountPtr qryAccount(const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto gw = getGateway(gwName);
		if(gw)
		{
			return gw->queryAccount();
		}
		else
			return AccountPtr();
	}

	std::vector<PositionPtr> qryPosition(const std::string& gwName)
	{
		LOG_DEBUG << __FUNCTION__;
		auto gw = getGateway(gwName);
		if(gw)
		{
			return gw->queryPosition();
		}
		else
			return std::vector<PositionPtr>();
	}

	ContractPtr getContract(const Symbol& s)
	{
		LOG_DEBUG << __FUNCTION__;
		return dataEngine->getContract(s);
	}

	void exit()
	{
		LOG_DEBUG << __FUNCTION__;
		for(auto i : gatewayDict)
		{
			i.second->close();
		}

		for(auto j : appDict)
		{
			j.second->stopAll();
		}

		dataEngine->saveContracts();

		eventEngine->stop();
	}


};

typedef std::shared_ptr<MainEngine> MainEnginePtr;

extern MainEnginePtr mainEngine(EventEnginePtr p);






