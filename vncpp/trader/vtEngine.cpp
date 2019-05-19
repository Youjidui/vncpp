#include "vtEngine.h"

MainEnginePtr mainEngine(EventEnginePtr p)
{
	return std::make_shared<MainEngine>(p);
}
