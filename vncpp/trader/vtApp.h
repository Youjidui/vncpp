#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <functional>


//middle office module
class App
{
	public:
	std::string name;
	std::string displayName;

	public:
	App(const std::string& appName, const std::string& appDisplayName)
		: name(appName), displayName(appDisplayName)
	{}
	virtual ~App() {}

	virtual void loadSettings(const std::string& filePath) = 0;
	virtual void saveSettings(const std::string& filePath) = 0;
	virtual void initAll() = 0;
	virtual void startAll() = 0;
	virtual void stopAll() = 0;
};


typedef std::shared_ptr<App> AppPtr;


