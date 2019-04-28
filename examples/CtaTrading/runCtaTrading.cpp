#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <logging.h>
#include "eventEngine.h"
#include "vtEngine.h"

void on_error(Event e)
{
	printf("error: %d, %s", e.code, e.text());
}


void run_child_process()
{
	printf( "---------------------------\n");

	LOG_INFO << "start CTA strategy child process\n";

	auto ee = event_engine();
	LOG_INFO << ("start event engine");

	auto me = main_engine(ee);
	auto gw = ctp_gateway();
	me->add_gateway(gw);
	auto stg = cta_strategy();
	me->add_app(stg);
	LOG_INFO << ("start main engine");

	gw->subscribe(std::bind(MainEngine::on_message, me),
			std::bind(MainEngine::on_error, me),
			std::bind(MainEngine::on_complete, me));
	gw->start();
	LOG_INFO << ("start CTP gateway");

	

}

