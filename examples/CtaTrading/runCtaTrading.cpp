#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <logging.h>
#include "eventEngine.h"
#include "vtEngine.h"
#include "ctp/ctpGateway.h"
#include "ctaStrategy/ctaEngine.h"



int main()
{
	printf( "---------------------------\n");

	LOG_INFO << "start CTA strategy child process\n";

	auto ee = eventEngine();
	ee->start(true);
	LOG_INFO << ("start event engine");

	auto me = mainEngine(ee);
	auto gw = ctpGateway("CTP", ee);
	me->addGateway(gw);
	auto cta = ctaEngine(ee, me);
	me->addApp(cta);
	LOG_INFO << ("start main engine");

	//gw->subscribe(std::bind(&MainEngine::on_message, me),
	//		std::bind(&MainEngine::on_error, me),
	//		std::bind(&MainEngine::on_complete, me));
	gw->connect();
	LOG_INFO << ("start CTP gateway");

	cta->loadSettings("./catSettings.json");
	LOG_INFO << ("load CTA settings");

	cta->initAll();
	LOG_INFO << "init CTA";

	cta->startAll();
	LOG_INFO << "start CTA";

	ee->run();

	while(true)
	{
		char c = getchar();
		if(c == 'q')
			break;
	}

	cta->stopAll();
	ee->stop();

	return 0;
}


