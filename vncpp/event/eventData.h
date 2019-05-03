#include <stdint.h>
#include <functional>
#include <memory>
#include <map>
#include <vector>

enum event_type
{
	//ENT_ERROR,
	ENT_TIMER,
	ENT_MESSAGE
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
	enum event_type type;
	EventDataPtr dict;

	public:
	Event(enum event_type type1, const EventDataPtr& args)
		: type(type1),
		dict(std::move(args))
	{
	}
	Event(enum event_type type1, EventDataPtr&& args)
		: type(type1),
		dict(std::move(args))
	{
	}
};

/*
class ErrorData : public EventData
{
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
	if(ENT_ERROR == e.m_type)
	{
		auto p = std::dynamic_pointer_cast<ErrorData>(e.m_dict);
		code = p->m_code;
		text = p->m_text;
		return true;
	}
	return false;
}
*/

