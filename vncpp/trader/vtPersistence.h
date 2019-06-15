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
#include <boost/property_tree/ptree.hpp>
#include "logging.h"
#include "vtObject.h"
#include "eventEngine.h"
#include "vtPersistenceContainerBase.h"


/*
c++ --std=c++11 <input>.cpp
  -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/libmongoc-1.0 \
  -I/usr/local/include/bsoncxx/v_noabi -I/usr/local/include/libbson-1.0 \
  -L/usr/local/lib -lmongocxx -lbsoncxx
*/


//typedef std::map<std::string, std::string> Record;
typedef std::vector<std::string> Record;


class PersistenceEngine
{
	std::string dbURI;
	//std::shared_ptr<mongocxx::client> dbClient;
	PersistenceContainerPtr dbClient;

public:
	//typedef std::function<void (TableBase::iterator )>
	
public:
	//PersistenceEngine();

	void setParameter(const boost::property_tree::ptree& parameters)
	{
		dbURI = parameters.get("DB_URI", "mongodb://localhost:27017");
	}

	virtual PersistenceContainerPtr createPersistenceContainer(const std::string& dbURI);

	void dbConnect()
	{
		if(!dbClient)
		{
			dbClient = createPersistenceContainer(dbURI);
		}
		dbClient->connect();
	}

    void dbInsert(const std::string& dbTablePath, const Record& data) 
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto collection = db[collectionName];
            //auto result = collection.insert_one(data);
        }
    }

	//resource name is like a file path, the level is separated by slash /
	TablePtr dbQuery(const std::string& resourceName)
    {
        if(dbClient)
        {
			auto t = (*dbClient)[resourceName];
			return t;
        }
		else
		{
			LOG_WARNING << __FUNCTION__ << ": no dbClient";
		}
		return TablePtr();
    }

	TableBase::iterator dbQuery(const std::string& resourceName, std::string&& where, std::string&& orderby)
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
			auto t = (*dbClient)[resourceName];
			return t->begin();
        }
		else
		{
			LOG_WARNING << __FUNCTION__ << ": no dbClient";
		}
		throw std::runtime_error("no dbClient");
    }

    void dbUpdate(const std::string& dbTablePath, const Record& data)
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto c = db[collectionName];
            //c.replace_one(filter, data, upsert);
        }
		else
		{
			LOG_WARNING << __FUNCTION__ << ": no dbClient";
		}
    }

    void dbDelete(const std::string& dbTablePath, const Record& data) 
    {
        if(dbClient)
        {
            //auto db = dbClient[dbName];
            //auto c = db[collectionName];
            //c.delete_one(filter);
        }
		else
		{
			LOG_WARNING << __FUNCTION__ << ": no dbClient";
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


class HistoryDataEngine
{
protected:
	PersistenceEnginePtr m_pe;

public:
	HistoryDataEngine(PersistenceEnginePtr pe) : m_pe(pe) {}

	void loadHistoryData(const std::string& dbName, const std::string& symbol, date_t startDate, date_t endDate, std::function<void (Record&)> onRecord)
	{
		if(m_pe)
		{
			auto t = m_pe->dbQuery(dbName);
			for(auto i = t->begin(); i != t->end(); ++i)
			{
				auto si = t->columnIndex("symbol");
				auto di = t->columnIndex("Datetime");
				if((*i)[si] == symbol)
				{
					if(di > startDate && di < endDate)
					{
						onRecord(*i);
					}
				}
			}
			
		}
		else
		{
			LOG_WARNING << __FUNCTION__ << ": no PersistenceEngine";
		}
	}
	
	
};

typedef std::shared_ptr<HistoryDataEngine> HistoryDataEnginePtr;


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
	{
		return true;
	}

	ContractPtr getContract(const Symbol& s)
	{
		return ContractPtr();
	}

	void updateOrderReq(OrderRequestPtr req, const std::string& orderID)
	{
	}
};

typedef std::shared_ptr<DataEngine> DataEnginePtr;


