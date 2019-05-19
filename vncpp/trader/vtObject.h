#pragma once

#include <stdint.h>
#include <time.h>
#include <string>
#include <memory>
#include <float.h>
#include "eventData.h"


//invalid price: DBL_MIN
//invalid volume: ULONG_MAX

typedef std::string Symbol;
typedef std::string OrderID;


struct _Contract
{
	//std::string symbol;		//can be set in channel info and remove from the object to shrink the data size
	std::string symbol;		//exchange encoded symbol
	std::string exchange;
	std::string vtSymbol;		//global unique symbol
	std::string gateway;

	static std::string make_vtSymbol(const std::string& s, const std::string& ex)
	{
		return s + '/' + ex;
	}

	_Contract(){}
	_Contract(const std::string& s, const std::string& ex, const std::string& g)
	: symbol(s), exchange(ex), vtSymbol(make_vtSymbol(s, ex)), gateway(g)
	{}
};

struct _Timestamp
{
	time_t datetime;

	_Timestamp(){}
	_Timestamp(time_t tm)
	: datetime(tm)
	{}
};

struct _Tick
{
	double lastPrice;
	int lastVolume;
	int totalVolume;
	int openInterest;

	_Tick(){}
	_Tick(double lastpx, int lastv, int ttv, int openItr)
	: lastPrice(lastpx), lastVolume(lastv), totalVolume(ttv), openInterest(openItr)
	{}
};

struct _Bar
{
	double open;
	double high;
	double low;
	double close;		//preClosePrice

	_Bar(){}
	_Bar(double o, double h, double l, double c)
	: open(o), high(h), low(l), close(c)
	{}
};

struct _PriceLimit
{
	double upperLimit;
	double lowerLimit;
};

/*
struct _MarketDepthItem
{
	int volume;
	int orderCount;
	double price;
};

struct _MarketDepth
{
	std::vector<_MarketDepthItem> ask;
	std::vector<_MarketDepthItem> bid;
};
*/

struct _MarketDepth
{
	double askPrice[5];
	int askVolume[5];
	double bidPrice[5];
	int bidVolume[5];
};

struct Tick : public EventData, public _Contract, public _Timestamp, public _Tick, public _Bar, public _PriceLimit, public _MarketDepth
{
public:
	Tick(){}
	Tick(const std::string& sym, const std::string& ex, const std::string& gw, 
			time_t tm, double lastpx, int lastv, int ttv, int openItr)
		: _Contract(sym, ex, gw), _Timestamp(tm), _Tick(lastpx, lastv, ttv, openItr)
	{}
};

typedef std::shared_ptr<Tick> TickPtr;

struct Bar : public EventData, public _Contract, public _Timestamp, public _Bar
{
	int volume;
	int openInterest;
	int interval;		//time interval for k-line bar

	Bar() {}
	Bar(const std::string& sym, const std::string& ex, const std::string& gw, 
			time_t tm, double open, double high, double low, double close, int ttv, int openItr, int interval)
		: _Contract(sym, ex, gw), _Timestamp(tm), _Bar(open, high, low, close), 
		volume(ttv), openInterest(openItr), interval(interval)
	{}
};

typedef std::shared_ptr<Bar> BarPtr;


struct Trade : public EventData, public _Contract, public _Timestamp
{
	std::string tradeID;
	std::string vtTradeID;
	std::string orderID;
	std::string vtOrderID;

	int direction;	//long or short
	int offset;		//open or close
	double price;
	int volume;
};

typedef std::shared_ptr<Trade> TradePtr;



enum ORDER_STATUS
{
	STATUS_NOTTRADED,
	STATUS_PARTTRADED,
	STATUS_ALLTRADED,
	STATUS_CANCELLED,
	STATUS_REJECTED,
	STATUS_UNKNOWN
};

enum DIRECTION
{
	DIRECTION_LONG = 0,
	DIRECTION_SHORT
};

struct Order : public EventData, public _Contract, public _Timestamp
{
	std::string orderID;
	std::string vtOrderID;

	int direction;	//long or short
	int offset;		//open or close
	double price;
	int totalVolume;
	int tradedVolume;
	int status;

	bool isActive()
	{
		return STATUS_NOTTRADED == status || STATUS_PARTTRADED == status;
		//return STATUS_ALLTRADED != status && STATUS_CANCELLED != status && STATUS_REJECTED != status;
	}
};

typedef std::shared_ptr<Order> OrderPtr;


struct Position : public EventData, public _Contract, public _Timestamp
{
	std::string vtPositionName;
	int direction;
	int position;
	int forzen;
	int avgPrice;
	int yesterdayPosition;
	double profit;

};

typedef std::shared_ptr<Position> PositionPtr;


struct Account : public EventData
{
	std::string account;	//account of the gateway
	std::string vtAccount;	//global account in this system

	double previousBalance;
	double balance;
	double available;
	double commission;
	double margin;
	double closeProfit;
	double positionProfit;
};

typedef std::shared_ptr<Account> AccountPtr;


struct Contract : public EventData, public _Contract
{
	std::string name;
	int productClass;
	int contractSize;
	double priceTick;

	double strikePrice;
	std::string underlyingSymbol;
	char optionType;		//call or put
	time_t expiryDate;
};

typedef std::shared_ptr<Contract> ContractPtr;


struct SubscribeRequest
{
	std::string symbol;
	std::string exchange;

	std::string productClass;
	std::string currency;
	time_t expiryDate;
	double strikePrice;
	char optionType;
};

typedef std::shared_ptr<SubscribeRequest> SubscribeRequestPtr;


struct OrderRequest
{
	std::string symbol;
	std::string exchange;
	std::string vtSymbol;
	double price;
	int volume;

	int priceType;
	int direction;
	int offset;

	int productClass;
	std::string currency;
	time_t expiry;
	double strikePrice;
	char optionType;
	std::string lastTradeDateOrContractMonth;
	double multiplier;
};

typedef std::shared_ptr<OrderRequest> OrderRequestPtr;


struct CancelOrderRequest
{
	std::string symbol;
	std::string exchange;
	std::string vtSymbol;

	std::string orderID;
	std::string frontID;
	std::string sessionID;
};

typedef std::shared_ptr<CancelOrderRequest> CancelOrderRequestPtr;

