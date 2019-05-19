#include "ctaEngine.h"


CtaEnginePtr ctaEngine(EventEnginePtr e)
{
	return std::make_shared<CtaEngine>(e);
}


