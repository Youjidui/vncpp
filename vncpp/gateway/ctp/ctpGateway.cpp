#include "ctpGateway.h"

gatewayPtr ctp_gateway()
{
	return std::make_shared<ctpGateway>();
}

