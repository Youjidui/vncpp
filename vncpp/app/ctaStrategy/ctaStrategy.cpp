#include "ctaStrategy.h"

CtaStrategy::CtaStrategy(MainEnginePtr m, EventEnginePtr e)
: App("CtaStrategy", "CTA Strategy")
, ctaEngine(e, m)
{}

bool CtaStrategy::start()
{
}

void CtaStrategy::stop()
{
}


AppPtr ctaStrategy(MainEnginePtr m, EventEnginePtr e)
{
	return std::make_shared<CtaStrategy>(m, e);
}

