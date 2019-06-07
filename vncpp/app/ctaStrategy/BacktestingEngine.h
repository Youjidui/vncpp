#pragma once
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <cmath>
#include <map>
//#include <bsoncxx/json.hpp>
//#include <mongocxx/client.hpp>
//#include <mongocxx/stdx.hpp>
//#include <mongocxx/uri.hpp>
#include "ctaBase.h"
#include "ctaTemplate.h"
#include "ctaEngine.h"
#include "vtPersistence.h"


inline 
double roundToPriceTick(double price, double priceTick)
{
    return std::round(price / priceTick) * priceTick;
}


class TradingResult
{
    public:
    double entryPrice;
    double exitPrice;
    time_t entryDt;
    time_t exitDt;
    int volume;     //position +/-
    double turnover;
    double commission;
    double slippage;
    double pnl;

    TradingResult(double entryPx, time_t entryDt, double exitPx, time_t exitDt,
    int volume, double rate, double slippage, double size)
    : entryPrice(entryPx)
    , exitPrice(exitPx)
    , entryDt(entryDt)
    , exitDt(exitDt)
    , volume(volume)
    , turnover((entryPx + exitPx)*size*abs(volume))
    , commission(this->turnover*rate)
    , slippage(slippage*2*size*abs(volume))
    , pnl((exitPx - entryPx)*volume*size - this->commission - this->slippage)
    {}
};


class DailyResult
{
public:
	time_t date;
	double closePrice;
	double previousClose;
	std::list<TradePtr> tradeList;
	int openPosition;
	int closePosition;
	double tradingPnl;
	double positionPnl;
	double totalPnl;

	int turnover;
	double commission;
	double slippage;
	double netPnl;

public:
	//DailyResult(time_t d, double closePx)
	//	: date(d), closePrice(closePx)
	//{}

	void addTrade(TradePtr t)
	{
		tradeList.push_back(t);
	}

	void calculatePnl(int openPosition = 0, int contractSize = 1, double commissionRate = 0, double slippage = 0)
	{
		this->openPosition  = openPosition;
		this->positionPnl = openPosition * (closePrice - previousClose) * contractSize;
		closePosition = openPosition;

		auto tradeCount = tradeList.size();
		for(auto t : tradeList)
		{
			auto posChange = (t->direction == DIRECTION_LONG) ? t->volume : -(t->volume);
			this->tradingPnl += posChange * (closePrice - t->price) * contractSize;
			this->closePosition += posChange;
			this->turnover += t->price * t->volume * contractSize;
			this->commission += t->price * t->volume * contractSize * commissionRate;
			this->slippage += t->volume * contractSize * slippage;
		}

		totalPnl = tradingPnl + positionPnl;
		netPnl = totalPnl - commission - slippage;
	}
};

class BackTestingResult
{
public:
    double capital;
    double maxCapital;
    double drawdown;

	double contractSize;
	double commissionRate;

    int totalResult;
    double totalTurnover;
    double totalCommission;
    double totalSlippage;

    std::vector<time_t> timeList;
    std::vector<double> pnlList;
    std::vector<double> capitalList;
    std::vector<double> drawdownList;

    int winningResult;
    int losingResult;
    double totalWinning;
    double totalLosing;

    double winningRate;
    double averageWinning;
    double averageLosing;
    double profitLossRatio;

    std::vector<int> posList;
    std::vector<time_t> tradeTimeList;
    std::vector<TradingResult> resultList;

public:
    BackTestingResult()
    {
        capital = 0;
        maxCapital = 0;
        drawdown = 0;
        totalResult = 0;
        totalTurnover = 0;
        totalCommission = 0;
        totalSlippage = 0;

        winningResult = 0;
        losingResult = 0;
        totalWinning = 0;
        totalLosing = 0;

        winningRate = 0;
        averageWinning = 0;
        averageLosing = 0;
        profitLossRatio = 0;
    }
};


class BacktestingEngine : public CtaEngine 
{
    /*
    CTA回测引擎
    函数接口和策略引擎保持一样，
    从而实现同一套代码从回测到实盘。
    */
public:
    static const int TICK_MODE = 0;
    static const int BAR_MODE = 1;

public:
    int engineType;
	int m_mode;

protected:
    int stopOrderCount;
    std::map<OrderID, StopOrderPtr> stopOrderDict;
    std::map<OrderID, StopOrderPtr> workingStopOrderDict;

    StrategyPtr strategy;
	std::string startDate;
	std::string endDate;
    int initDays;

    std::string dbName;
    std::string symbol;
	
    double capital;
    double slippage;
    double rate;
    double contractSize;
    double priceTick;

    //std::shared_ptr<mongocxx::client> dbClient;
    //std::shared_ptr<mongocxx::cursor> dbCursor;
    //std::shared_ptr<RpcClient> hasClient;
	PersistenceEnginePtr m_pe;

    std::vector<TickPtr> m_initTickData;
	std::vector<BarPtr> m_initBarData;

    time_t dataStartDate;
    time_t dataEndDate;
    time_t strategyStartDate;

    int limitOrderCount;
    std::map<OrderID, OrderPtr> limitOrderDict;
    std::map<OrderID, OrderPtr> workingLimitOrderDict;

    int tradeCount;
    std::map<TradeID, TradePtr> tradeDict;

    TickPtr m_tick;
    BarPtr m_bar;
    time_t m_dt;

    std::map<date_t, DailyResult> dailyResultDict;

public:
    BacktestingEngine(EventEnginePtr ee, MainEnginePtr m)
		: CtaEngine(ee, m)
    {
		LOG_DEBUG << __FUNCTION__;
	}

	void setDatabase(const std::string& db, const std::string& symbol)
	{
		LOG_DEBUG << __FUNCTION__;
		this->dbName = db;
		this->symbol = symbol;
	}

    //void setStartDate(time_t startDate, int initDays = 10)
    //dateFormat = "20100416"
    void setStartDate(std::string const& startDate, int initDays = 10)
    {
		LOG_DEBUG << __FUNCTION__;
        this->startDate = startDate;
        this->initDays = initDays;
        struct tm tm1 = {0};
        sscanf(startDate.c_str(), "%4d%2d%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday);
        this->dataStartDate = mktime(&tm1);
    }

    void setEndDate(std::string const& endDate)
    {
		LOG_DEBUG << __FUNCTION__;
        this->endDate = endDate;
        struct tm tm1 = {0};
        sscanf(endDate.c_str(), "%4d%2d%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday);
        tm1.tm_hour = 23;
        tm1.tm_min = 59;
        this->dataEndDate = mktime(&tm1);
    }

	void setBacktestingMode(int mode) { m_mode = mode;}
	void setSlippage(double s) { slippage = s;}
	void setRate(double r) { rate = r; }
	void setContractSize(double s) { contractSize = s; }
	void setPriceTick(double pt) { priceTick = pt; }

    public:
    bool initHdsClient()
    {
		LOG_DEBUG << __FUNCTION__;
        auto reqAddress = "tcp://localhost:5555";
        auto subAddress = "tcp://localhost:7777";
        //hdsClient = std::make_shared<RpcClient>(reqAddress, subAddress);
        //hdsClient->start();
    }

    void loadHistoryData()
    {/*
		LOG_DEBUG << __FUNCTION__;
        mongocxx::uri uri("mongodb://localhost:27017");
        auto dbClient = std::make_shared<mongocxx::client>(uri);
        auto collection = dbClient[dbName][symbol];

        if(hdsClient)
            initCursor = hdsClient->loadHistoryData(dbName, symbol, dataStartDate, strategyStartDate);
        else
        {
            auto tpDataStartDate = std::chrono::system_clock::from_time_t(dataStartDate);
            auto tpStrategyStartDate = std::chrono::system_clock::from_time_t(strategyStartDate);
            initCursor = collection.find(
                bsoncxx::builder::stream::document() << "datetime" 
                << bsoncxx::builder::stream::open_document
                << "$gte" << tpDataStartDate
                << "$lt" << tpStrategyStartDate
                << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::finalize;
            ).sort("datetime");
        }
        
        initData.clear();
        for(auto doc : initCursor)
        {
            std::cout << bsoncxx::to_json(doc) << "\n";
            if(mode == BAR_Mode)
            {

            }
            else
            {
                
            }
        }


        if(hdsClient)
            initCursor = hdsClient->loadHistoryData(dbName, symbol, strategyStartDate, dataEndDate);
        else
        {
            if(dataEndDate == 0)
            {
                auto tpStrategyStartDate = std::chrono::system_clock::from_time_t(strategyStartDate);
                dbCursor = collection.find(
                    bsoncxx::builder::stream::document() << "datetime"
                    << bsoncxx::builder::stream::open_document
                    << "$gte" << tpStrategyStartDate
                    << bsoncxx::builder::stream::close_document
                    << bsoncxx::builder::stream::finalize;
                ).sort("datetime");
            }
            else
            {
                auto tpStrategyStartDate = std::chrono::system_clock::from_time_t(strategyStartDate);
                auto tpDataEndDate = std::chrono::system_clock::from_time_t(dataEndDate);
                dbCursor = collection.find(
                    bsoncxx::builder::stream::document() << "datetime" 
                    << bsoncxx::builder::stream::open_document
                    << "$gte" << tpStrategyStartDate
                    << "$lt" << tpDataEndDate
                    << bsoncxx::builder::stream::close_document
                    << bsoncxx::builder::stream::finalize;
                ).sort("datetime");
            }
            
        }

*/
    }


    void runBacktesting()
    {
		LOG_DEBUG << __FUNCTION__;

        loadHistoryData();

       	auto i = m_strategyDict.begin();
		if(i != m_strategyDict.end())
		{
			strategy = i->second;
        	strategy->init();
        	strategy->onInit();

        	strategy->start();
        	strategy->onStart();
		}

		/*
        for(auto d : dbCursor)
        {
            if(m_mode == BAR_MODE)
            {
                auto bar = std::make_shared<Bar>();
                loadFromDB(d, bar);
                newBar(bar);
            }
            else
            {
                auto tick = std::make_shared<Tick>();
                loadFromDB(d, tick);
                newTick(tick);
            }
        }
		*/
    }

    void newBar(BarPtr bar)
    {
		LOG_DEBUG << __FUNCTION__;
        m_bar = bar;
        m_dt = bar->datetime;

        crossLimitOrder(bar);
        crossStopOrder(bar);
        strategy->onBar(bar);
        updateDailyClose(bar->datetime, bar->close);
    }

    void newTick(TickPtr t)
    {
		LOG_DEBUG << __FUNCTION__;
        m_tick = t;
        m_dt = t->datetime;

        crossLimitOrder(t);
        crossStopOrder(t);
        strategy->onTick(t);
        updateDailyClose(t->datetime, t->lastPrice);
    }

	void updateDailyClose(time_t dt, double price)
	{
		LOG_DEBUG << __FUNCTION__;
		auto date = dateFrom(dt);
		auto i = dailyResultDict.find(date);
		if(i == dailyResultDict.end())
		{
			DailyResult dr = {date, price, 0};
			dailyResultDict.insert(std::make_pair(date, dr));
		}
		else
		{
			dailyResultDict[date].closePrice = price;
		}
	}

    void initStrategy(StrategyPtr s)
    {
		LOG_DEBUG << __FUNCTION__;
        strategy = s;
    }

    void crossLimitOrder(BarPtr bar)
    {
		LOG_DEBUG << __FUNCTION__;
        auto buyCrossPrice = bar->low;
        auto sellCrossPrice = bar->high;
        auto buyBestCrossPrice = bar->open;
        auto sellBestCrossPrice = bar->open;
        crossLimitOrder_(buyCrossPrice, sellCrossPrice, buyBestCrossPrice, sellBestCrossPrice);
    }

    void crossLimitOrder(TickPtr t)
    {
		LOG_DEBUG << __FUNCTION__;
        auto buyCrossPrice = t->askPrice[0];
        auto sellCrossPrice = t->bidPrice[0];
        auto buyBestCrossPrice = t->askPrice[0];
        auto sellBestCrossPrice = t->bidPrice[0];
        crossLimitOrder_(buyCrossPrice, sellCrossPrice, buyBestCrossPrice, sellBestCrossPrice);
    }

    void crossLimitOrder_(double buyCrossPrice, double sellCrossPrice, 
    double buyBestCrossPrice, double sellBestCrossPrice)
    {
		LOG_DEBUG << __FUNCTION__;
        for(auto i = workingLimitOrderDict.begin(); 
        i != workingLimitOrderDict.end(); ++i)
        {
            auto orderID = i->first;
            auto o = i->second;

            auto buyCross = (o->direction == DIRECTION_LONG &&
            o->price >= buyCrossPrice && buyCrossPrice > 0);
            auto sellCross = (o->direction == DIRECTION_SHORT &&
            o->price <= sellCrossPrice && sellCrossPrice > 0);

            if(buyCross || sellCross)
            {
                char str[20];
                sprintf(str,"%x",++tradeCount);
                auto t = std::make_shared<Trade>();
                t->tradeID = str;
                t->vtTradeID = str;
                t->vtSymbol = o->vtSymbol;
                t->orderID = o->orderID;
                t->direction = o->direction;
                t->offset = o->offset;

                if(buyCross)
                {
                    t->price = std::min(o->price, buyBestCrossPrice);
                    strategy->position += o->totalVolume;
                }
                else
                {
                    t->price = std::max(o->price, sellBestCrossPrice);
                    strategy->position -= o->totalVolume;
                }
                t->volume = o->totalVolume;
                t->datetime = m_dt;
                
                strategy->onTrade(t);
                tradeDict[str] = t;

                o->tradedVolume = o->totalVolume;
                o->status = STATUS_ALLTRADED;
                strategy->onOrder(o);

                i = workingLimitOrderDict.erase(i);
                continue;
            }
        }
    }

    void crossStopOrder(BarPtr b)
    {
		LOG_DEBUG << __FUNCTION__;
        auto buyCrossPrice = b->high;
        auto sellCrossPrice = b->low;
        auto bestCrossPrice = b->open;
        crossStopOrder_(buyCrossPrice, sellCrossPrice, bestCrossPrice);
    }

    void crossStopOrder(TickPtr t)
    {
		LOG_DEBUG << __FUNCTION__;
        auto buyCrossPrice = t->lastPrice;
        auto sellCrossPrice = t->lastPrice;
        auto bestCrossPrice = t->lastPrice;
        crossStopOrder_(buyCrossPrice, sellCrossPrice, bestCrossPrice);
    }

    void crossStopOrder_(double buyCrossPrice, double sellCrossPrice, double bestCrossPrice)
    {
		LOG_DEBUG << __FUNCTION__;
        for(auto i = workingStopOrderDict.begin(); 
        i != workingStopOrderDict.end(); ++i)
        {
            auto orderID = i->first;
            auto so = i->second;

            auto buyCross = (so->direction == DIRECTION_LONG &&
            so->stopPrice <= buyCrossPrice);
            auto sellCross = (so->direction == DIRECTION_SHORT &&
            so->stopPrice >= sellCrossPrice);

            if(buyCross || sellCross)
            {
                char str[20];

                so->status = STOPORDER_TRIGGERED;
                strategy->onStopOrder(so);


                sprintf(str,"%x",++limitOrderCount);
                auto no = std::make_shared<Order>();
                no->orderID = str;
                no->vtOrderID = str;
                no->symbol = so->symbol;
                no->vtSymbol = so->vtSymbol;
                no->direction = so->direction;
                no->offset = so->offset;
                no->price = so->stopPrice;
                no->totalVolume = so->volume;
                no->status = STATUS_ALLTRADED;
                no->datetime = m_dt;

                limitOrderDict[str] = no;


                sprintf(str,"%x",++tradeCount);
                auto t = std::make_shared<Trade>();
                t->tradeID = str;
                t->vtTradeID = str;
                t->vtSymbol = so->vtSymbol;

                if(buyCross)
                {
                    t->price = std::max(so->stopPrice, bestCrossPrice);
                    strategy->position += so->volume;
                }
                else
                {
                    t->price = std::min(so->stopPrice, bestCrossPrice);
                    strategy->position -= so->volume;
                }

                t->orderID = no->orderID;
                t->direction = no->direction;
                t->offset = no->offset;

                t->volume = no->totalVolume;
                t->datetime = m_dt;
                
                strategy->onTrade(t);
                tradeDict[str] = t;

                no->tradedVolume = no->totalVolume;
                no->status = STATUS_ALLTRADED;
                strategy->onOrder(no);

                i = workingStopOrderDict.erase(i);
                continue;
            }
        }
    }

    OrderID sendOrder(const std::string& vtSymbol, int orderType, double price, int volume, StrategyPtr s)
    {
		LOG_DEBUG << __FUNCTION__;
        char str[20];
        sprintf(str,"%x",++limitOrderCount);
        auto no = std::make_shared<Order>();
        no->orderID = str;
        no->vtOrderID = str;
        no->vtSymbol = vtSymbol;
        no->price = roundToPriceTick(price, getPriceTick(s));
        no->totalVolume = volume;
        no->status = STATUS_NEW;
        no->datetime = m_dt;

        if (orderType == CTAORDER_BUY)
		{
            no->direction = DIRECTION_LONG;
            no->offset = OFFSET_OPEN;
		}
        else if (orderType == CTAORDER_SELL)
		{
            no->direction = DIRECTION_SHORT;
            no->offset = OFFSET_CLOSE;
		}
        else if (orderType == CTAORDER_SHORT)
		{
            no->direction = DIRECTION_SHORT;
            no->offset = OFFSET_OPEN;
		}
        else if (orderType == CTAORDER_COVER)
		{
            no->direction = DIRECTION_LONG;
            no->offset = OFFSET_CLOSE;
		}

        limitOrderDict[str] = no;
        workingLimitOrderDict[str] = no;
        return str;
    }

    void cancelOrder(const std::string& orderID)
    {
		LOG_DEBUG << __FUNCTION__;
        auto i = workingLimitOrderDict.find(orderID);
        if(i != workingLimitOrderDict.end())
        {
            auto& o = i->second;
            o->status = STATUS_CANCELLED;
            
            strategy->onOrder(o);
            workingLimitOrderDict.erase(i);
        }
    }

    StopOrderID sendStopOrder(const std::string& vtSymbol, int orderType, double price, int volume, StrategyPtr s)
    {
		LOG_DEBUG << __FUNCTION__;
        char str[20];
        sprintf(str,"%s%x", STOPORDERPREFIX, ++stopOrderCount);
        auto so = std::make_shared<StopOrder>();
        so->vtSymbol = vtSymbol;
        so->stopPrice = roundToPriceTick(price, getPriceTick(s));
        so->volume = volume;
        so->strategy = s;
        so->status = STOPORDER_WAITING;
        so->stopOrderID = str;

        if (orderType == CTAORDER_BUY)
		{
            so->direction = DIRECTION_LONG;
            so->offset = OFFSET_OPEN;
		}
        else if (orderType == CTAORDER_SELL)
		{
            so->direction = DIRECTION_SHORT;
            so->offset = OFFSET_CLOSE;
		}
        else if (orderType == CTAORDER_SHORT)
		{
            so->direction = DIRECTION_SHORT;
            so->offset = OFFSET_OPEN;
		}
        else if (orderType == CTAORDER_COVER)
		{
            so->direction = DIRECTION_LONG;
            so->offset = OFFSET_CLOSE;
		}

        stopOrderDict[str] = so;
        workingStopOrderDict[str] = so;

        strategy->onStopOrder(so);
        return str;
    }

    void cancelStopOrder(const std::string& stopOrderID)
    {
		LOG_DEBUG << __FUNCTION__;
        auto i = workingStopOrderDict.find(stopOrderID);
        if(i != workingStopOrderDict.end())
        {
            auto& so = i->second;
            so->status = STOPORDER_CANCELLED;
            strategy->onStopOrder(so);
            workingStopOrderDict.erase(i);
        }
    }

    void putStrategyEvent()
    {}

    void insertData(const std::string& dbName, const std::string& collectionName, void* data, int len)
    {}

    std::vector<BarPtr>& loadBar(const std::string& dbName, const std::string& collectionName, const std::string& startDate)
    {
		LOG_DEBUG << __FUNCTION__;
        return m_initBarData;
    }

    std::vector<TickPtr>& loadTick(const std::string& dbName, const std::string& collectionName, const std::string& startDate)
    {
		LOG_DEBUG << __FUNCTION__;
        return m_initTickData;
    }

    void cancelAll()
    {
		LOG_DEBUG << __FUNCTION__;
        for(auto i : workingLimitOrderDict)
        {
            cancelOrder(i.first);
        }
        for(auto i : workingStopOrderDict)
        {
            cancelStopOrder(i.first);
        }
    }

    void saveSyncData(StrategyPtr)
    {}

    double getPriceTick(StrategyPtr)
    {
        return priceTick;
    }

    BackTestingResult calculateBacktestingResult()
    {
		LOG_DEBUG << __FUNCTION__;
        std::vector<TradingResult> resultList;
        std::list<Trade> longTrade;
        std::list<Trade> shortTrade;
        std::vector<time_t> tradeTimeList;
        std::vector<int> posList;

        for(auto i : tradeDict)
        {
            auto t = *(i.second);
            if(t.direction == DIRECTION_LONG)
            {
                if(shortTrade.empty())
                {
                    longTrade.push_back(t);
                }
                else
                {
                    while(true)
                    {
                        auto& entryTrade = shortTrade.front();
                        auto& exitTrade = t;

                        auto closedVolume = std::min(exitTrade.volume, entryTrade.volume);
                        auto result = TradingResult(entryTrade.price, entryTrade.datetime,
                        exitTrade.price, exitTrade.datetime, -closedVolume,
                        rate, slippage, contractSize);
                        resultList.push_back(result);

                        posList.push_back(-1);
                        posList.push_back(0);

                        tradeTimeList.push_back(result.entryDt);
                        tradeTimeList.push_back(result.exitDt);

                        entryTrade.volume -= closedVolume;
                        exitTrade.volume -= closedVolume;

                        if(0 == entryTrade.volume)
                        {
                            shortTrade.pop_front();
                        }
                        if(0 == exitTrade.volume)
                        {
                            break;
                        }

                        if(exitTrade.volume > 0)
                        {
                            if(shortTrade.empty())
                            {
                                longTrade.push_back(exitTrade);
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                if(longTrade.empty())
                {
                    shortTrade.push_back(t);
                }
                else
                {
                    while(true)
                    {
                        auto& entryTrade = longTrade.front();
                        auto& exitTrade = t;

                        auto closedVolume = std::min(exitTrade.volume, entryTrade.volume);
                        auto result = TradingResult(entryTrade.price, entryTrade.datetime,
                        exitTrade.price, exitTrade.datetime, closedVolume,
                        rate, slippage, contractSize);
                        resultList.push_back(result);

                        posList.push_back(1);
                        posList.push_back(0);

                        tradeTimeList.push_back(result.entryDt);
                        tradeTimeList.push_back(result.exitDt);

                        entryTrade.volume -= closedVolume;
                        exitTrade.volume -= closedVolume;

                        if(0 == entryTrade.volume)
                        {
                            longTrade.pop_front();
                        }
                        if(0 == exitTrade.volume)
                        {
                            break;
                        }

                        if(exitTrade.volume > 0)
                        {
                            if(longTrade.empty())
                            {
                                shortTrade.push_back(exitTrade);
                                break;
                            }
                        }
                    }
                }
                
            }
            
        }

        double endPrice = 0;
        if(m_mode == BAR_MODE)
            endPrice = m_bar->close;
        else
        {
            endPrice = m_tick->lastPrice;
        }

        for(auto i : longTrade)
        {
            auto& t = (i);
            auto result = TradingResult(t.price, t.datetime, endPrice, m_dt,
            t.volume, rate, slippage, contractSize);
            resultList.push_back(std::move(result));
        }

        for(auto i : shortTrade)
        {
            auto& t = (i);
            auto result = TradingResult(t.price, t.datetime, endPrice, m_dt,
            -t.volume, rate, slippage, contractSize);
            resultList.push_back(std::move(result));
        }

        BackTestingResult r;
        if(resultList.empty())
        {

        }
        else
        {
            for(auto i : resultList)
            {
                r.capital += i.pnl;
                r.maxCapital = std::max(r.capital, r.maxCapital);
                r.drawdown = r.capital + r.maxCapital;

                r.pnlList.push_back(i.pnl);
                r.timeList.push_back(i.exitDt);
                r.capitalList.push_back(r.capital);
                r.drawdownList.push_back(r.drawdown);

                r.totalResult++;
                r.totalTurnover += i.turnover;
                r.totalCommission += i.commission;
                r.totalSlippage += i.slippage;

                if(i.pnl >= 0)
                {
                    r.winningResult ++;
                    r.totalWinning += i.pnl;
                }
                else
                {
                    r.losingResult ++;
                    r.totalLosing += i.pnl;
                }
            }

            r.winningRate = r.winningResult/r.totalResult*100;
            if(r.winningResult > 0)
            {
                r.averageWinning = r.totalWinning/r.winningResult;
            }
            if(r.losingResult > 0)
            {
                r.averageLosing = r.totalLosing/r.losingResult;
            }
            if(r.averageLosing > 0)
            {
                r.profitLossRatio = r.averageWinning/r.averageLosing;
            }

            r.tradeTimeList.swap(tradeTimeList);
            r.posList.swap(posList);
        }
        return r;
    }

    void clearBacktestingResult()
    {
		LOG_DEBUG << __FUNCTION__;
        limitOrderCount = 0;
        limitOrderDict.clear();
        workingLimitOrderDict.clear();

        stopOrderCount = 0;
        stopOrderDict.clear();
        workingStopOrderDict.clear();

        tradeCount = 0;
        tradeDict.clear();
    }

	void showBacktestingResult()
	{
		LOG_DEBUG << __FUNCTION__;
		auto d = calculateBacktestingResult();
	}
};


