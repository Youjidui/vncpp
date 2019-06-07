#include "eventEngine.h"
#include "logging.h"

EventEnginePtr eventEngine()
{
	LOG_DEBUG << __FUNCTION__;
	return std::make_shared<EventEngine>();
}
