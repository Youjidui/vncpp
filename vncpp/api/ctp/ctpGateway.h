#pragma once

#include <stdint.h>
#include <memory>
#include "gateway.h"

class ctpGateway : public gateway
{
};

extern gatewayPtr ctp_gateway();

