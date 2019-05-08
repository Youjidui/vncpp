#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <cmath>
#include <map>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>


inline 
double roundToPriceTick(double price, double priceTick)
{
    return std::round(price / priceTick) * priceTick;
}


class BacktestingEngine
{
    /*
    CTA回测引擎
    函数接口和策略引擎保持一样，
    从而实现同一套代码从回测到实盘。
    */
public:
    const char* TICK_MODE = 'tick';
    const char* BAR_MODE = 'bar';

    public:
    int engineType;

    protected:
    int stopOrderCount;
    std::map<OrderID, StopOrderPtr> stopOrderDict;
    std::map<OrderID, StopOrderPtr> workingStopOrderDict;

    StrategyPtr strategy;
    std::string mode;
    time_t startDate;
    time_t endDate;
    int initDays;

    double capital;
    double slippage;
    double rate;
    double contractSize;
    double priceTick;

    std::shared_ptr<mongocxx::client> dbClient;
    std::shared_ptr<mongocxx::cursor> dbCursor;
    std::shared_ptr<RpcClient> hasClient;

    std::vector<> initData;
    std::string dbName;
    std::string symbol;

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

    std::map<std::string, > dailyResultDict;

public:
    BacktestingEngine()
    {}

    //void setStartDate(time_t startDate, int initDays = 10)
    //dateFormat = "20100416"
    void setStartDate(std::string const& startDate, int initDays = 10)
    {
        this->startDate = startDate;
        this->initDays = initDays;
        struct tm tm1 = {0};
        sscanf(startDate.c_str(), "%4d%2d%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday);
        this->dataStartDate = mktime(&tm1);
    }

    void setEndDate(std::string const& endDate)
    {
        this->endDate = endDate;
        struct tm tm1 = {0};
        sscanf(endDate.c_str(), "%4d%2d%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday);
        tm1.tm_hour = 23;
        tm1.tm_min = 59;
        this->dataEndDate = mktime(&tm1);
    }


    public:
    bool initHdsClient()
    {
        auto reqAddress = "tcp://localhost:5555";
        auto subAddress = "tcp://localhost:7777";
        hdsClient = std::make_shared<RpcClient>(reqAddress, subAddress);
        hdsClient->start();
    }

    void loadHistoryData()
    {
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


    }


    void runBacktesting()
    {
        loadHistoryData();

        
        strategy->init();
        strategy->onInit();

        strategy->start();
        strategy->onStart();

        for(d : dbCursor)
        {
            if(mode == BAR_MODE)
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
    }

    void newBar(BarPtr bar)
    {
        m_bar = bar;
        m_dt = bar->datetime;

        crossLimitOrder();
        crossStopOrder();
        strategy->onBar(bar);
        updateDailyClose(bar->datetime, bar->close);
    }

    void newTick(TickPtr t)
    {
        m_tick = t;
        m_dt = t->datetime;

        crossLimitOrder();
        crossStopOrder();
        strategy->onTick(t);
        updateDailyClose(t->datetime, t->lastPrice);
    }

    void initStrategy(StrategyPtr s)
    {
        strategy = s;
    }

    void crossLimitOrder(BarPtr bar)
    {
        auto buyCrossPrice = bar->low;
        auto sellCrossPrice = bar->high;
        auto buyBestCrossPrice = bar->open;
        auto sellBestCrossPrice = bar->open;
        crossLimitOrder_(buyCrossPrice, sellCrossPrice, buyBestCrossPrice, sellBestCrossPrice);
    }

    void crossLimitOrder(TickPtr t)
    {
        auto buyCrossPrice = t->askPrice[0];
        auto sellCrossPrice = t->bidPrice[0];
        auto buyBestCrossPrice = t->askPrice[0];
        auto sellBestCrossPrice = t->bidPrice[0];
        crossLimitOrder_(buyCrossPrice, sellCrossPrice, buyBestCrossPrice, sellBestCrossPrice);
    }

    void crossLimitOrder_(double buyCrossPrice, double sellCrossPrice, 
    double buyBestCrossPrice, double sellBestCrossPrice)
    {
        for(auto i = workingLimitOrderDict.begin(); 
        i != workingLimitOrderDict.end() ++i)
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
                t->dt = m_dt;
                
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
        auto buyCrossPrice = b->high;
        auto sellCrossPrice = b->low;
        auto bestCrossPrice = b->open;
        crossStopOrder_(buyCrossPrice, sellCrossPrice, bestCrossPrice);
    }

    void crossStopOrder(TickPtr t)
    {
        auto buyCrossPrice = t->lastPrice;
        auto sellCrossPrice = t->lastPrice;
        auto bestCrossPrice = t->lastPrice;
        crossStopOrder_(buyCrossPrice, sellCrossPrice, bestCrossPrice);
    }

    void crossStopOrder_(double buyCrossPrice, double sellCrossPrice, double bestCrossPrice)
    {
        for(auto i = workingStopOrderDict.begin(); 
        i != workingStopOrderDict.end() ++i)
        {
            auto orderID = i->first;
            auto so = i->second;

            auto buyCross = (so->direction == DIRECTION_LONG &&
            so->price <= buyCrossPrice);
            auto sellCross = (so->direction == DIRECTION_SHORT &&
            so->price >= sellCrossPrice);

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
                no->price = so->price;
                no->totalVolume = so->volume;
                no->status = STATUS_ALLTRADED;
                no->dt = m_dt;

                limitOrderDict[str] = no;


                sprintf(str,"%x",++tradeCount);
                auto t = std::make_shared<Trade>();
                t->tradeID = str;
                t->vtTradeID = str;
                t->vtSymbol = so->vtSymbol;

                if(buyCross)
                {
                    t->price = std::max(so->price, bestCrossPrice);
                    strategy->position += so->volume;
                }
                else
                {
                    t->price = std::min(so->price, bestCrossPrice);
                    strategy->position -= so->volume;
                }

                t->orderID = no->orderID;
                t->direction = no->direction;
                t->offset = no->offset;

                t->volume = no->totalVolume;
                t->dt = m_dt;
                
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
        char str[20];
        sprintf(str,"%x",++limitOrderCount);
        auto no = std::make_shared<Order>();
        no->orderID = str;
        no->vtOrderID = str;
        no->vtSymbol = vtSymbol;
        no->price = roundToPriceTick(price);
        no->totalVolume = volume;
        no->status = STATUS_NEW;
        no->dt = m_dt;

        if orderType == CTAORDER_BUY:
            no->direction = DIRECTION_LONG
            no->offset = OFFSET_OPEN
        else if orderType == CTAORDER_SELL:
            no->direction = DIRECTION_SHORT
            no->offset = OFFSET_CLOSE
        else if orderType == CTAORDER_SHORT:
            no->direction = DIRECTION_SHORT
            no->offset = OFFSET_OPEN
        else if orderType == CTAORDER_COVER:
            no->direction = DIRECTION_LONG
            no->offset = OFFSET_CLOSE     


        limitOrderDict[str] = no;
        workingLimitOrderDict[str] = no
        return str;
    }

    void cancelOrder(const std::string& orderID)
    {
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
        char str[20];
        sprintf(str,"%s%x", STOPORDERPREFIX, ++stopOrderCount);
        auto so = std::make_shared<StopOrder>();
        so->vtSymbol = vtSymbol;
        so->price = roundToPriceTick(price);
        so->volume = volume;
        so->strategy = s;
        so->status = STOPORDER_WAITING;
        so->stopOrderID = str;

        if orderType == CTAORDER_BUY:
            so->direction = DIRECTION_LONG
            so->offset = OFFSET_OPEN
        else if orderType == CTAORDER_SELL:
            so->direction = DIRECTION_SHORT
            so->offset = OFFSET_CLOSE
        else if orderType == CTAORDER_SHORT:
            so->direction = DIRECTION_SHORT
            so->offset = OFFSET_OPEN
        else if orderType == CTAORDER_COVER:
            so->direction = DIRECTION_LONG
            so->offset = OFFSET_CLOSE

        stopOrderDict[str] = so;
        workingStopOrderDict[str] = so;

        strategy->onStopOrder(so);
        return str;
    }

    void cancelStopOrder(const std::string& stopOrderID)
    {
        auto i = workingStopOrderDict.find(stopOrderID);
        if(i != workingStopOrderDict.end())
        {
            auto& so = i->second;
            so->status = STOPORDER_CANCELLED
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
        return initData;
    }

    std::vector<TickPtr>& loadTick(const std::string& dbName, const std::string& collectionName, const std::string& startDate)
    {
        return initData;
    }

    void cancelAll()
    {
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
};


