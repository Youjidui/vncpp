#pragma once
#include <fstream>

#include "vtPersistenceContainerBase.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
 
//defined in ../vncpp/trader/vtPersistence.h
//typedef std::map<std::string, std::string> Record;
//typedef std::vector<std::string> Record;

class CSVTableReader : public TableBase 
{
	std::ifstream stream;
	std::ifstream::pos_type begin_pos;
public:

public:
	CSVTableReader(const std::string& filePath)
	: stream(filePath)
	{
		LOG_DEBUG << __PRETTY_FUNCTION__;
		std::string line;
		std::getline(stream, line);
		boost::algorithm::split(m_head, line, boost::algorithm::is_any_of(","));
		begin_pos = stream.tellg();
	}
	
	virtual iterator begin()
	{
		LOG_DEBUG << __PRETTY_FUNCTION__;
		stream.seekg(begin_pos);
		return iterator(this, next());
	}

protected:
	virtual Record* next()
	{
		//LOG_DEBUG << __PRETTY_FUNCTION__;
		std::string line;
		std::getline(stream, line);
		boost::algorithm::split(m_cursor, line, boost::algorithm::is_any_of(","));
		return &m_cursor;
	}
};

typedef std::shared_ptr<CSVTableReader> CSVTableReaderPtr;

class FilePersistenceContainer : public PersistenceContainerBase
{
	const std::string head = "file://";
	const boost::filesystem::path root;
	std::map<std::string, CSVTableReaderPtr> m_tables;

public:
	FilePersistenceContainer(const std::string& filePath)
		: root(filePath.substr(head.length()))
	{
		LOG_DEBUG << __PRETTY_FUNCTION__;
	}

	virtual TablePtr operator[](const std::string& resName)
	{
		LOG_DEBUG << __PRETTY_FUNCTION__;
		auto i = m_tables.find(resName);
		if(i != m_tables.end())
			return i->second;

		auto path = root / resName;
		if(boost::filesystem::exists(path))
		{
			LOG_INFO << "Find data path: " << path.string();
			auto p = std::make_shared<CSVTableReader>(path.string());
			m_tables.insert(std::make_pair(resName, p));
			return p;
		}
		else
		{
			LOG_ERROR << "Cannot find the data path: " << path.string();
		}
		return TablePtr();

		if("bar" == resName)
		{
		}
		else if("tick" == resName)
		{
		}
	}

	virtual bool connect()
	{
		LOG_DEBUG << __PRETTY_FUNCTION__;
		return true;
	}
};


