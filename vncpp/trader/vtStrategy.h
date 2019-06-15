#pragma once

#include <stdint.h>
#include <memory>
#include <functional>
#include <set>
#include <map>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "logging.h"

class Strategy : public std::enable_shared_from_this<Strategy>
{
public:
    const std::string name;     //the instance name (with different symbol)
    virtual const std::string getClassName() = 0;

public:
    Strategy(const std::string& instanceName)
    : name(instanceName)
    {}
	virtual ~Strategy(){}

	virtual bool init() = 0;
    virtual void onInit() =0;
    virtual bool start() =0;
    virtual void onStart() =0;
    virtual void stop() =0;
    virtual void onStop() =0;

    virtual void onTick(TickPtr) =0;
    virtual void onOrder(OrderPtr) =0;
    virtual void onTrade(TradePtr) =0;
    virtual void onBar(BarPtr) =0;      //K-line

};

typedef std::shared_ptr<Strategy> StrategyPtr;

