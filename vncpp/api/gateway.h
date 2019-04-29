#pragma once

#include <stdint.h>
#include <memory>
#include "eventEngine.h"

class gateway
{
	EventEnginePtr m_ee;
	public:
	gateway(EventEnginePtr ee)
	: m_ee(ee)
	{ }
	
	virtual ~gateway() {}
};


