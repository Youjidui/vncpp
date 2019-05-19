#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <logging.h>
#include "eventEngine.h"
#include "vtEngine.h"
#include "ctp/ctpGateway.h"
#include "ctaStrategy/ctaEngine.h"

void on_error(uint32_t code, std::string const& text)
{
	LOG_ERROR << code << text;
}


void run_child_process()
{
	printf( "---------------------------\n");

	LOG_INFO << "start CTA strategy child process\n";

	auto ee = eventEngine();
	LOG_INFO << ("start event engine");

	auto me = mainEngine(ee);
	auto gw = ctpGateway("CTP", ee);
	me->addGateway(gw);
	auto stg = ctaStrategy(ee);
	me->addApp(stg);
	LOG_INFO << ("start main engine");

	//gw->subscribe(std::bind(&MainEngine::on_message, me),
	//		std::bind(&MainEngine::on_error, me),
	//		std::bind(&MainEngine::on_complete, me));
	gw->connect();
	LOG_INFO << ("start CTP gateway");

	

}

