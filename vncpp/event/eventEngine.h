#include <stdint.h>
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/tuple.hpp>

enum event_type
{
	ENT_TIMER
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
	enum event_type m_type;
	EventDataPtr m_dict;

	public:
	Event(enum event_type type, const EventDataPtr& args)
		: m_type(type),
		m_dict(std::move(args))
	{
	}
	Event(enum event_type type, EventDataPtr&& args)
		: m_type(type),
		m_dict(std::move(args))
	{
	}
};


typedef std::function<void (Event)> DataHandler;
typedef std::function<void ()> CompleteHandler;
typedef std::function<void (uint32_t, std::string const&)> ErrorHandler;

class EventEngine
{
	public:
	void start()
	{
	}

	void stop()
	{
	}

	void register(enum event_type, EventHandler)
	{
	}

	void unregister(enum event_type, EventHandler)
	{
	}

	void put(Event e)
	{
	}

	uint32_t subscribe(EventHandler dh, CompleteHandler ch, ErrorHandler eh)
	{
	}

	void unsubscribe(uint32_t id)
	{
	}
};


