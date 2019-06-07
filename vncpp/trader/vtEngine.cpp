#include "vtEngine.h"

MainEnginePtr mainEngine(EventEnginePtr p)
{
	LOG_DEBUG << __FUNCTION__;
	return std::make_shared<MainEngine>(p);
}
