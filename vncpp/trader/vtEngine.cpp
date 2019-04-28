#include "vtEngine.h"

MainEnginePtr main_engine(EventEnginePtr p)
{
	return std::make_shared<MainEngine>(p);
}
