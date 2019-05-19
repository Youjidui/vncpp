#pragma once
#include <stdint.h>
#include <memory>
#include <functional>
#include <list>
#include <map>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "logging.h"
//#include "eventEngine.h"
#ifdef __linux__
#include <dlfcn.h>
#elif defined(__WINDOWS)
#include <Windows.h>
#endif
#include "eventEngine.h"
#include "vtObject.h"


class Gateway
{
protected:
public:
	const std::string name;		//the instance name (with different symbol)
	virtual const std::string getClassName() = 0;

    EventEngine& m_eventEngine;

	//gateway parameters
	//std::string m_settingfilePath;
	boost::property_tree::ptree* parameters;

public:
	Gateway(const std::string& aInstanceName, EventEngine& ee)
		: name(aInstanceName), m_eventEngine(ee)
	{
	}

	virtual ~Gateway() {}

	void onTick(TickPtr p)
	{
		m_eventEngine.emit(Event(EVENT_TICK, p));
	}

	void onTrade(TradePtr p)
	{
		m_eventEngine.emit(Event(EVENT_TRADE, p));
	}

	void onOrder(OrderPtr p)
	{
		m_eventEngine.emit(Event(EVENT_ORDER, p));
	}

	void onPosition(PositionPtr p)
	{
		m_eventEngine.emit(Event(EVENT_POSITION, p));
	}

	void onAccount(AccountPtr p)
	{
		m_eventEngine.emit(Event(EVENT_ACCOUNT, p));
	}

	void onContract(ContractPtr p)
	{
		m_eventEngine.emit(Event(EVENT_CONTRACT, p));
	}
/*
	void onHistory(HistoryPtr p)
	{
		m_eventEngine.emit(Event(EVENT_HISTORY, p));
	}
*/
	virtual void connect() = 0;
	virtual void close() = 0;

	virtual void subscribe(SubscribeRequestPtr p) = 0;
	virtual OrderID sendOrder(OrderRequestPtr p) = 0;
	virtual void cancelOrder(CancelOrderRequestPtr p) = 0;

	virtual AccountPtr queryAccount() = 0;
	virtual std::vector<PositionPtr> queryPosition() = 0;
	//virtual void queryHistory(QueryHistoryRequestPtr p) = 0;
};


//the gateway implementation has to implement and export those function for Engine
typedef Gateway* (*FUNCTION_createGatewayInstance)(const char* aGatewayInstanceName, const char* aGatewayClassName, EventEngine* engine);
typedef void (*FUNCTION_destroyGatewayInstance)(Gateway* aGatewayInstance);


//for the built-in gateway created by C++ keyword new
inline 
void staticLibraryDestroyGatewayInstance(Gateway* p, void* aHost = NULL)
{
	delete p;
}

//for the gateway created by FUNCTION_createGatewayInstance from DLL
void dynamicLibraryDestroyGatewayInstance(Gateway* p, void* aHost);

struct GatewayDeleter
{
	typedef void (*FUNCTION_deleter)(Gateway*, void*);
	FUNCTION_deleter deleter;
	void* parameter;

	GatewayDeleter(FUNCTION_deleter d, void* p)
	: deleter(d), parameter(p)
	{}
	void operator()(Gateway* p)
	{
		deleter(p, parameter);
	}
};


//typedef std::shared_ptr<Gateway, GatewayDeleter> GatewayPtr;
typedef std::shared_ptr<Gateway> GatewayPtr;






class GatewayLoaderForDynamicLibrary
{
	protected:
	typedef std::string GatewayClassName;
	typedef std::string ModuleName;
	typedef void* ModuleHandle;
	std::map<GatewayClassName, ModuleHandle> m_dllHandleDict;

	friend void dynamicLibraryDestroyGatewayInstance(Gateway* p, void* aHost);

	protected:
	//there may be more than a Gateway in a DLL, using the Gateway name to identity each other
	Gateway* loadGateway(const std::string& aInstanceName, const std::string& aGatewayClassName,
     const std::string& aModuleName, EventEngine& engine)
	{
		Gateway* r = nullptr;
		#ifdef __linux__
		void* handle = dlopen(aModuleName.c_str(), RTLD_NOW);
		#elif defined(__WINDOWNS)
		void* handle = LoadLibrary(aModuleName.c_str());
		#endif

		if(handle)
		{
			FUNCTION_createGatewayInstance createfn = (FUNCTION_createGatewayInstance)
			#ifdef __linux__
			dlsym
			#elif defined(__WINDOWS)
			GetProcAddress 
			#endif
			(handle, "createGatewayInstance");
			if(createfn)
				r = (Gateway*)createfn(aInstanceName.c_str(), aGatewayClassName.c_str(), &engine);
			if(r)
				//to free the module
				m_dllHandleDict.insert(std::make_pair(r->getClassName(), handle));
			else
				#ifdef __linux__
				dlclose(handle);
				#elif defined(__WINDOWS)
				FreeLibrary(handle);
				#endif

		}
		return r;
	}

	void freeGateway(Gateway* s)
	{
		if(s)
		{
			auto it = m_dllHandleDict.find(s->getClassName());
			if(it != m_dllHandleDict.end())
			{
				auto handle = it->second;
				FUNCTION_destroyGatewayInstance destroyFn = (FUNCTION_destroyGatewayInstance)
				#ifdef __linux__
				dlsym
				#elif defined(__WINDOWS)
				GetProcAddress 
				#endif
				(handle, "destroyGatewayInstance");
				if(destroyFn)
					destroyFn(s);

				#ifdef __linux__
				dlclose(it->second);
				#elif defined(__WINDOWS)
				FreeLibrary(it->second);
				#endif
			}

		}
	}

	public:
	GatewayPtr load(const std::string& aInstanceName, const std::string& aGatewayName, 
    const std::string& aModuleName, EventEngine& engine)
	{
		auto p = loadGateway(aInstanceName, aGatewayName, aModuleName, engine);
		if(p)
			return GatewayPtr(p, GatewayDeleter(&dynamicLibraryDestroyGatewayInstance, this));
		else
			throw std::runtime_error("null pointer. load Gateway failed");
	}
};

inline
void dynamicLibraryDestroyGatewayInstance(Gateway* p, void* aHost)
{
	auto h = (GatewayLoaderForDynamicLibrary*)aHost;
	if(h)
	{
		h->freeGateway(p);
	}
}



