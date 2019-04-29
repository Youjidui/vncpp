#include "eventEngine.h"

EventEnginePtr event_engine()
{
	return std::make_shared<EventEngineRx>();
}
