#include <stdint.h>
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <boost/asio.hpp>
//#include <boost/tuple.hpp>

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
	std::map<uint32_t, EventHandlerVector> m_handlerMap;

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

	uint32_t subscribe(EventHandler dh, CompleteHandler ch, ErrorHandler eh)
	{
	}

	void unsubscribe(uint32_t id)
	{
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

class EventEngine : public EventLoop, public EventHandlerRegistry
{
	protected:
	public:
	void emit(Event e)
	{
		post(std::bind(&EventHandlerRegistry::onEvent, this, e));
	}

	
};


typedef std::shared_ptr<EventEngine> EventEnginePtr;
extern EventEnginePtr event_engine();


