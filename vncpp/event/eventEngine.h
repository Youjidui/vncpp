#include <stdint.h>
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <boost/asio.hpp>
//#include <tuple>

enum event_type
{
	//ENT_ERROR,
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
	enum event_type type;
	//std::string subType;	//vtSymbol for tick/trade/position, or vtOrderID for order, or Account for account
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

//Should be as normal log line to LOG module with different log level?
class LogData : public EventData
{
	public:
	std::string moduleName;
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




typedef std::function<void (Event)> EventHandler;
typedef std::function<void ()> CompleteHandler;
typedef std::function<void (uint32_t, std::string const&)> ErrorHandler;

class EventLoop
{
	protected:
	std::shared_ptr<boost::asio::io_context> m_context;
	std::unique_ptr<boost::asio::io_context::work> m_donothing;
	std::unique_ptr<std::thread> m_worker;

	void run()
	{
		m_context->run();
	}

	public:
	void start()
	{
		m_context.reset(new boost::asio::io_context);
		m_donothing.reset(new boost::asio::io_context::work(m_context.get()));
		m_worker.reset(new std::thread(std::bind(&EventEngine::run, this)));
	}

	void stop()
	{
		m_donothing.reset();
		m_context.stop();
		m_worker->join();
	}

	template<typename FUNCTION>
	void post(FUNCTION&& f)
	{
		m_context->post(f);
	}

};


class EventHandlerRegistry
{
	protected:
	typedef std::vector<EventHandler> EventHandlerVector;
        EventHandlerVector	m_handlers;
	std::map<enum event_type, EventHandlerVector> m_handlerMap;

	public:
	void register(enum event_type e, EventHandler h)
	{
		auto i = m_handlerMap.find(e);
		if(m_handlerMap.end() == i)
		{
			EventHandlerVector v;
			v.push_back(h);
			m_handlerMap.insert(std::make_pair(e, v));
		}
		else
		{
			i->second.push_back(h);
		}
	}

	void unregister(enum event_type, EventHandler)
	{
		auto i = m_handlerMap.find(e);
		if(m_handlerMap.end() != i)
		{
			auto& v = i->second;
			for(auto n = v.begin(); n != v.end(); ++n)
			{
				if(*n == h)
				{
					v.erase(n);
					break;
				}
			}	
		}
	}

	protected:
	void onEvent(Event e)
	{
		auto i = m_handlerMap.find(e.m_type);
		if(i != m_handlerMap.cend())
		{
			auto& v = i->second;
			for(auto f : v)
			{
				f(e);
			}
		}
	}

};


class Observable
{
	protected:
	typedef uint32_t subscription_id;
	typedef std::tuple<EventHandler, CompleteHandler, ErrorHandler> observer;
	typedef std::map<subscription_id, observer> ObserverMap;
	typedef std::vector<observer> ObserverVector;
	ObserverMap m_observerMap;
	std::atomic<subscription_id> m_next_new_id;

	public:
	Observable() : m_next_new_id(0)
	{}
	
	uint32_t subscribe(EventHandler dh, CompleteHandler ch, ErrorHandler eh)
	{
		auto i = ++m_next_new_id;
		m_observerMap[i] = std::make_tuple(dh, ch, eh);
		return i;
	}

	void unsubscribe(uint32_t id)
	{
		m_observerMap.erase(i);
	}

	protected:
	void onData(Event e)
	{
		for(auto i : m_observerMap)
		{
			std::get<0>(i.second)(e);
		}
	}

	void onComplete()
	{
		for(auto i : m_observerMap)
		{
			std::get<1>(i.second)();
		}
	}

	void onError(uint32_t code, std::string const& text)
	{
		for(auto i : m_observerMap)
		{
			std::get<2>(i.second)(code, text);
		}
	}

};



class EventEngineEv : public EventLoop, public EventHandlerRegistry
{
	protected:
	public:
	void emit(Event e)
	{
		post(std::bind(&EventHandlerRegistry::onEvent, this, e));
	}
};


class EventEngineRx : public Observable
{
	protected:
	std::shared_ptr<EventLoop> m_eventLoop;
	
	public:
	EventEngineRx(std::shared_ptr<EventLoop> l)
	: m_eventLoop(l)
	{ }
	
	public:
	void data(Event e)
	{
		m_eventLoop->post(std::bind(&Observable::onData, this, e));
	}

	void error(uint32_t code, std::string const& text)
	{
		m_eventLoop->post(std::bind(&Observable::onError, this, code, text));
	}

	void complete()
	{
		m_eventLoop->post(std::bind(&Observable::onComplete, this));
	}
	
	template<typename FUNCTION>
	void post(FUNCTION&& f)
	{
		m_eventLoop->post(f);
	}
};

typedef EventEngineEv EventEngine;
typedef std::shared_ptr<EventEngine> EventEnginePtr;
extern EventEnginePtr event_engine();


