#pragma once

struct ContractData
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

	ContractData(){}
	ContractData(const std::string& s, const std::string& ex, const std::string& g)
	: symbol(s), exchange(ex), vtSymbol(make_vtSymbol(s, ex)), gateway(g)
	{}
};

struct Timestamp
{
	time_t datetime;

	Timestamp(){}
	Timestamp(time_t tm)
	: datetime(tm)
	{}
};

struct TickData
{
	double lastPrice;
	int lastVolume;
	int totalVolume;
	int openInterest;

	TickData(){}
	TickData(double lastpx, int lastv, int ttv, int openItr)
	: lastPrice(lastpx), lastVolume(lastv), totalVolume(ttv), openInterest(openItr)
	{}
};

struct BarData
{
	double openPrice;
	double highPrice;
	double lowPrice;
	double closePrice;		//preClosePrice

	BarData(){}
	BarData(double o, double h, double l, double c)
	: openPrice(o), highPrice(h), lowPrice(l), closePrice(c)
	{}
};

struct PriceLimit
{
	double upperLimitPrice;
	double lowerLimitPrice;
};


struct MarketDepthItem
{
	int volume;
	int orderCount;
	double price;
};

struct MarketDepth
{
	std::vector<MarketDepthItem> ask;
	std::vector<MarketDepthItem> bid;
};

struct Tick : public ContractData, public Timestamp, public TickData, public BarData, public PriceLimit, public MarketDepth
{
public:
	Tick(){}
	Tick(const std::string& sym, const std::string& ex, const std::string& gw, 
			time_t tm, double lastpx, int lastv, int ttv, int openItr)
		: ContractData(sym, ex, gw), Timestamp(tm), TickData(lastpx, lastv, ttv, openItr)
	{}
};


struct Bar : public ContractData, public Timestamp, public BarData
{
	int totalVolume;
	int openInterest;
	int interval;		//time interval for k-line bar

	Bar() {}
	Bar(const std::string& sym, const std::string& ex, const std::string& gw, 
			time_t tm, double open, double high, double low, double close, int ttv, int openItr, int interval)
		: ContractData(sym, ex, gw), Timestamp(tm), BarData(open, high, low, close), 
		totalVolume(ttv), openInterest(openItr), interval(interval)
	{}
};

struct Trade : public ContractData, public Timestamp
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

struct Order : public ContractData, public Timestamp
{
	std::string orderID;
	std::string vtOrderID;

	int direction;	//long or short
	int offset;		//open or close
	double price;
	int totalVolume;
	int tradedVolume;
	int status;

};

struct Position : public ContractData, public Timestamp
{
	std::string vtPositionName;
	int direction;
	int position;
	int forzen;
	int avgPrice;
	int yesterdayPosition;
	double profit;

};

struct Account
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


struct Contract : public ContractData
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


