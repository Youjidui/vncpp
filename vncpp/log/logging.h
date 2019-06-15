#pragma once
#include <boost/log/trivial.hpp>


#define LOG_TRACE 		BOOST_LOG_TRIVIAL(trace)
#define LOG_DEBUG		BOOST_LOG_TRIVIAL(debug)
#define LOG_INFO		BOOST_LOG_TRIVIAL(info)
#define LOG_WARNING		BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR		BOOST_LOG_TRIVIAL(error)
#define LOG_FATAL		BOOST_LOG_TRIVIAL(fatal)


#ifdef __linux__
#undef __FUNCTION__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif

#ifdef __GNUC__ 
#define PRIVATE __attribute__((visibility("hidden")))
#define PUBLIC __attribute__((visibility("default")))
#else
#define PRIVATE 
#define PUBLIC_EXPORT __declspec(dllexpor) 
#define PUBLIC_IMPORT __declspec(dllexport) 
#endif 

