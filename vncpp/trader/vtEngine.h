#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include "eventEngine.h"

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

};

typedef std::shared_ptr<MainEngine> MainEnginePtr;

extern MainEnginePtr main_engine(EventEnginePtr p);


