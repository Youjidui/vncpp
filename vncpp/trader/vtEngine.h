#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include "eventEngine.h"

class MainEngine
{
	EventEnginePtr m_eventEngine;
	std::map<std::string, GatewayPtr> m_gateways;
	std::map<std::string, AppPtr> m_apps;

	public:
	MainEngine(EventEnginePtr ee)
		: m_eventEngine(ee)
	{
	}

	void addGateway(GatewayPtr gw)
	{
		m_gateways[gw->getName()] = gw;
	}

	void addApp(AppPtr app)
	{
		m_apps[app->getName()] = app;
	}

};

typedef std::shared_ptr<MainEngine> MainEnginePtr;

extern MainEnginePtr main_engine(EventEnginePtr p);


