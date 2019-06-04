#pragma once

#include <stdint.h>
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <boost/asio.hpp>
#include "eventData.h"



typedef std::function<void (Event)> EventHandler;
typedef std::function<void ()> CompleteHandler;
typedef std::function<void (uint32_t, std::string const&)> ErrorHandler;

class EventLoop
{
	protected:
	std::shared_ptr<boost::asio::io_service> m_context;
	std::unique_ptr<boost::asio::io_service::work> m_donothing;
	std::unique_ptr<std::thread> m_worker;

	void internalRun()
	{
		m_context->run();
	}

	public:
	void start(bool subThread = false)
	{
		m_context.reset(new boost::asio::io_service);
		m_donothing.reset(new boost::asio::io_service::work(*m_context.get()));
		if(subThread)
			m_worker.reset(new std::thread(std::bind(&EventLoop::internalRun, this)));
	}

	void run()
	{
		if(m_worker)	return;
		else internalRun();
	}

	void stop()
	{
		m_donothing.reset();
		if(m_context)
			m_context->stop();
		if(m_worker)
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
	struct EventHandlerRegistryItem
	{
		uint32_t id;
		EventHandler h;
		EventHandlerRegistryItem(uint32_t new_id, EventHandler&& rh)
			: id(new_id), h(rh)
		{}
	};

	protected:
	uint32_t next_new_id;
	typedef std::vector<EventHandlerRegistryItem> EventHandlerVector;
	std::map<EventType, EventHandlerVector> m_handlerMap;

	public:
	EventHandlerRegistry()
		: next_new_id(0)
	{}

	uint32_t register_(EventType e, EventHandler&& h)
	{
		auto i = m_handlerMap.find(e);
		if(m_handlerMap.end() == i)
		{
			EventHandlerVector v;
			v.push_back(EventHandlerRegistryItem{ ++next_new_id, std::move(h)});
			m_handlerMap.insert(std::make_pair(e, std::move(v)));
		}
		else
		{
			i->second.push_back(EventHandlerRegistryItem{ ++next_new_id, std::move(h)});
		}
		return next_new_id;
	}

	void unregister_(EventType e, uint32_t id)
	{
		auto i = m_handlerMap.find(e);
		if(m_handlerMap.end() != i)
		{
			auto& v = i->second;
			for(auto n = v.begin(); n != v.end(); ++n)
			{
				if(n->id == id)
				{
					v.erase(n);
					break;
				}
			}
		}
	}

	void onEvent(Event e)
	{
		auto i = m_handlerMap.find(e.type);
		if(i != m_handlerMap.cend())
		{
			auto& v = i->second;
			for(auto f : v)
			{
				f.h(e);
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
		m_observerMap.erase(id);
	}

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
extern EventEnginePtr eventEngine();


