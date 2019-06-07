#include "ctpGateway.h"

GatewayPtr ctpGateway(const std::string& aInstanceName, EventEnginePtr e)
{
	LOG_DEBUG << __FUNCTION__;
	//return GatewayPtr(new CtpGateway(aInstanceName, e), GatewayDeleter(&staticLibraryDestroyGatewayInstance, NULL));
	return std::make_shared<CtpGateway>(aInstanceName, *e);
}

