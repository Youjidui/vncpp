#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include <vector>
//#include <tuple>

enum EventType
{
	EVENT_ERROR,
	EVENT_LOG,
	EVENT_TIMER,
	EVENT_TICK,
	EVENT_ORDER,
	EVENT_TRADE,
	EVENT_POSITION,
	EVENT_ACCOUNT,
	EVENT_CONTRACT,
	EVENT_HISTORY,
	EVENT_CTA_LOG
};

class EventData
{
public:
	virtual ~EventData() {}
};

typedef std::shared_ptr<EventData> EventDataPtr;

class Event
{
	public:
	EventType type;
	//std::string subType;	//vtSymbol for tick/trade/position, or vtOrderID for order, or Account for account
	EventDataPtr dict;

	public:
	Event(EventType type1, EventDataPtr& args)
		: type(type1),
		dict(args)
	{
	}
	Event(EventType type1, EventDataPtr&& args)
		: type(type1),
		dict(std::move(args))
	{
	}
};

//Should be as normal log line to LOG module with different log level?
class LogData : public EventData
{
	public:
	std::string moduleName;
	time_t logTime;
	std::string logContent;
	
	LogData(std::string&& aModuleName, std::string&& aLogContent)
	: moduleName(aModuleName), logContent(aLogContent)
	{}
	LogData(std::string const& aModuleName, std::string const& aLogContent)
	: moduleName(aModuleName), logContent(aLogContent)
	{}
};


inline 
Event makeLogEvent(std::string&& aModuleName, std::string&& aLogContent)
{
	auto data = std::make_shared<LogData>(aModuleName, aLogContent);
	return Event(EVENT_CTA_LOG, data);
}

/*
class ErrorData : public EventData
{
	public:
	int m_code;
	std::string m_text;
	
	ErrorData(int code, std::string const& text)
	: m_code(code), m_text(text)
	{	}
	ErrorData(int code, std::string&& text)
	: m_code(code), m_text(std::move(text))
	{	}
}

inline
Event make_error_event(int code, std::string const& text)
{
	return Event (ENT_ERROR, std::make_shared<ErrorData>(code, text));
}

inline
bool extract_error_event(Event const& e, int& code, std::string& text)
{
	if(ENT_ERROR == e.type)
	{
		auto p = std::dynamic_pointer_cast<ErrorData>(e.m_dict);
		code = p->m_code;
		text = p->m_text;
		return true;
	}
	return false;
}
*/



