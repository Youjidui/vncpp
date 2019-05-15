#pragma once
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>


/*
c++ --std=c++11 <input>.cpp
  -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/libmongoc-1.0 \
  -I/usr/local/include/bsoncxx/v_noabi -I/usr/local/include/libbson-1.0 \
  -L/usr/local/lib -lmongocxx -lbsoncxx
*/


class PersistenceEngine
{
    //boost::property_tree::ptree* parameters;
	std::string dbURI;
	std::shared_ptr<mongocxx::client> dbClient;

public:
	PersistenceEngine();

	void setParameter(const boost::property_tree::ptree& parameters)
	{
		dbURI = parameters.get("DB_URI", "mongodb://localhost:27017");
	}

	void dbConnect()
	{
		if(!dbClient)
		{
			dbClient = std::make_shared<mongocxx::client>(dbURI);
		}
	}

    void dbInsert(const std::string& dbName, const std::string& collectionName,
      bsoncxx::document::value&& data)
    {
        if(dbClient)
        {
            auto db = dbClient[dbName];
            auto collection = db[collectionName];
            auto result = collection.insert_one(data);
        }
    }

    void dbQuery(const std::string& dbName, const std::string& collectionName,
      bsoncxx::document::value&& data, std::string&& sortKey, 
      sortDirection = ASCENDING)
    {
        if(dbClient)
        {
            auto db = dbClient[dbName];
            auto collection = db[collectionName];

            if(sortKey.empty())
            {
                auto cursor = collection.find(data);
            }
            else
            {
                auto cursor = collection.find(data).sort(sortKey, sortDirection);
            }
            
        }
    }
};


class DataEngine
{
public:
};
