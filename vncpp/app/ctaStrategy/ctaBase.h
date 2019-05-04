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


typedef std::string OrderID;
typedef OrderID StopOrderID;


class Strategy;

class StopOrder
{
	public:
	std::string vtSymbol;
    int orderType;
    int direction;
    int offset;
    double price;
    int volume;

    Strategy& strategy;
    StopOrderID stopOrderID;
    int status;
};

typedef std::shared_ptr<StopOrder> StopOrderPtr;

