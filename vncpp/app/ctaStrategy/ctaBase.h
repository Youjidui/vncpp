#pragma once
#include <stdint.h>
#include <memory>
#include <functional>
#include <list>
#include <map>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "logging.h"
#include "eventEngine.h"
#include "vtObject.h"


typedef OrderID StopOrderID;

const char * const  STOPORDERPREFIX = "SO_";


class Strategy;
typedef std::shared_ptr<Strategy> StrategyPtr;


enum CTA_ORDER_TYPE
{
	CTAORDER_BUY = 0,
	CTAORDER_SELL,
	CTAORDER_SHORT,
	CTAORDER_COVER
};

enum STOP_ORDER_STATUS
{
	STOPORDER_TRIGGERED = 100,
	STOPORDER_CANCELLED,
	STOPORDER_WAITING
};

struct StopOrder
{
	public:
	std::string vtSymbol;
	std::string symbol;
    int orderType;
    int direction;
    int offset;
    double stopPrice;
    int volume;

	StrategyPtr strategy;
    StopOrderID stopOrderID;
    int status;

public:
};

typedef std::shared_ptr<StopOrder> StopOrderPtr;

