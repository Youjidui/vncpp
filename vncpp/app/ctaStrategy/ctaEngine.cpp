#include "ctaEngine.h"


CtaEnginePtr ctaEngine(EventEnginePtr e, MainEnginePtr m)
{
	LOG_DEBUG << __FUNCTION__;
	return std::make_shared<CtaEngine>(e, m);
}


