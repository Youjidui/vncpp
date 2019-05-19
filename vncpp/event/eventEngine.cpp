#include "eventEngine.h"

EventEnginePtr eventEngine()
{
	return std::make_shared<EventEngine>();
}
