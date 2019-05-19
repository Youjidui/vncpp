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


typedef std::string OrderID;
typedef OrderID StopOrderID;


class Strategy;


class Order
{
    public:
    std::string orderID;        //exchange
    std::string vtOrderID;      //vncpp
    std::string symbol;         //exchange
    std::string vtSymbol;       //vncpp
    int direction;
    int offset;
    double price;
    int totalVolume;
    int tradedVolume;
    int status;
    time_t dt;
};

class Trade
{
    public:
    std::string tradeID;        //exchange
    std::string vtTradeID;      //vncpp 
    std::string vtSymbol;       //vncpp
    std::string orderID;
    int direction;
    int offset;
    double price;
    int volume;
    time_t dt;

};

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

