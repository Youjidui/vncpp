#include "ctaEngine.h"


CtaEnginePtr ctaEngine(EventEnginePtr e, MainEnginePtr m)
{
	return std::make_shared<CtaEngine>(e, m);
}


