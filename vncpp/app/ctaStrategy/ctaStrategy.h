#pragma once

#include "vtEngine.h"
#include "ctaEngine.h"

class CtaStrategy : public App
{
	public:
		CtaEngine ctaEngine;
		//and UI

	CtaStrategy(MainEnginePtr m, EventEnginePtr e);
		  
	virtual bool start();
	virtual void stop();
};

extern AppPtr ctaStrategy(MainEnginePtr m, EventEnginePtr e);


