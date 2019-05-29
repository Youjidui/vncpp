#pragma once
#include <stdint.h>
#include <memory>
#include <functional>
#include <string.h>
#include <string>
//#include <bsoncxx/json.hpp>
//#include <mongocxx/client.hpp>
//#include <mongocxx/stdx.hpp>
//#include <mongocxx/uri.hpp>
#include "vtObject.h"



/*
c++ --std=c++11 <input>.cpp
  -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/libmongoc-1.0 \
  -I/usr/local/include/bsoncxx/v_noabi -I/usr/local/include/libbson-1.0 \
  -L/usr/local/lib -lmongocxx -lbsoncxx
*/


class PersistenceContainerBase
{
	public:
	virtual ~PersistenceContainerBase(){}
};

typedef std::shared_ptr<PersistenceContainerBase> PersistenceContainerPtr;

class PersistenceEngine
{
    //boost::property_tree::ptree* parameters;
	std::string dbURI;
	//std::shared_ptr<mongocxx::client> dbClient;
	PersistenceContainerPtr dbClient;

public:
	//PersistenceEngine();

	void setParameter(const boost::property_tree::ptree& parameters)
	{
		dbURI = parameters.get("DB_URI", "mongodb://localhost:27017");
	}

	void dbConnect()
	{
		if(!dbClient)
		{
			const std::string mongodb = "mongodb";
			if(strncmp(mongodb.c_str(), dbURI.c_str(), mongodb.length()) == 0)
			{
				//dbClient = std::make_shared<mongocxx::client>(dbURI);
			}
		}
	}

    void dbInsert(const std::string& dbName, const std::string& collectionName,
      //bsoncxx::document::value&& data)
	  std::string&& json_str)
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto collection = db[collectionName];
            //auto result = collection.insert_one(data);
        }
    }

    void dbQuery(const std::string& dbName, const std::string& collectionName,
		std::string&& queryCondition)
      //bsoncxx::document::value&& data,
      //std::function<void (mongocxx::cursor&)> onQuery,
      //std::string&& sortKey, sortDirection = ASCENDING)
    {
        if(dbClient)
        {/*
            auto db = dbClient[dbName];
            auto collection = db[collectionName];

            if(sortKey.empty())
            {
                auto cursor = collection.find(data);
                onQuery(cursor)
            }
            else
            {
                auto cursor = collection.find(data).sort(sortKey, sortDirection);
                onQuery(cursor)
            }*/
        }
    }

    void dbUpdate(const std::string& dbName, const std::string& collectionName,
    //bsoncxx::document::value&& data,
	std::string&& json_str,
    std::string&& filter, bool upsert = false)
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto c = db[collectionName];
            //c.replace_one(filter, data, upsert);
        }
    }

    void dbDelete(const std::string& dbName, const std::string& collectionName,
    std::string&& filter)
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto c = db[collectionName];
            //c.delete_one(filter);
        }
    }

    void dbLogging(Event e)
    {
		/*
        auto log = std::dynamic_pointer_cast<Log>(e.dict);
        bsoncxx::document::value doc_value = bsoncxx::builder
        << "content" << log->logContent
        << "time" << log->logTime
        << "gateway" << log->moduleName
        << bsoncxx::builder::stream::finalize;

        dbInsert(LOG_DB_NAME, todayDate, doc_value);
		*/
    }
};

typedef std::shared_ptr<PersistenceEngine> PersistenceEnginePtr;



class DataEngine
{
public:
    std::string contractFileName;
    std::string contractFilePath;

    EventEnginePtr m_ee;

	std::map<Symbol, TickPtr> tickDict;
	std::map<Symbol, ContractPtr> contractDict;
	std::map<OrderID, OrderPtr> orderDict;
	std::map<OrderID, TradePtr> tradePtr;

    DataEngine(EventEnginePtr ee)
    : m_ee(ee)
    {

    }

    void init(const std::string& contractFilePath)
    {
        //loadContracts(contractFilePath);
        registerEvent();
    }

    void registerEvent()
    {
		m_ee->register_(EVENT_TICK, std::bind(&DataEngine::processTickEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_ORDER, std::bind(&DataEngine::processOrderEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_TRADE, std::bind(&DataEngine::processTradeEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_CONTRACT, std::bind(&DataEngine::processContractEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_POSITION, std::bind(&DataEngine::processPositionEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_ACCOUNT, std::bind(&DataEngine::processAccountEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_LOG, std::bind(&DataEngine::processLogEvent, this, std::placeholders::_1));
		m_ee->register_(EVENT_ERROR, std::bind(&DataEngine::processErrorEvent, this, std::placeholders::_1));
    }

    void processTickEvent(Event e)
    {
        auto tick = std::dynamic_pointer_cast<Tick>(e.dict);
        tickDict[tick->vtSymbol] = tick;
    }

    void processContractEvent(Event e)
    {
        auto contract = std::dynamic_pointer_cast<Contract>(e.dict);
        contractDict[contract->vtSymbol] = contract;
        contractDict[contract->symbol] = contract;
    }

	void processOrderEvent(Event e)
	{
		auto o = std::dynamic_pointer_cast<Order>(e.dict);
		orderDict[o->vtOrderID] = o;
	}

	void processTradeEvent(Event e)
	{
	}

	void processPositionEvent(Event e)
	{
	}

	void processAccountEvent(Event e)
	{
	}

	void processLogEvent(Event e)
	{
	}

	void processErrorEvent(Event e)
	{
	}

	//bool loadContracts(const std
	bool saveContracts()
	{}

	ContractPtr getContract(const Symbol& s)
	{
		return ContractPtr();
	}

	void updateOrderReq(OrderRequestPtr req, const std::string& orderID)
	{
	}
};

typedef std::shared_ptr<DataEngine> DataEnginePtr;


