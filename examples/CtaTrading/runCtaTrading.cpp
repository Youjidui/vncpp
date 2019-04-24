#include <stdio.h>
#include <functional>

void on_error(event e)
{
	printf("error: %d, %s", e.code, e.text());
}


void run_child_process()
{
	printf( "---------------------------\n");

	auto le = log_engine();
	le->info("start CTA strategy child process\n");

	auto ee = event_engine();
	le->info("start event engine");

	auto me = main_engine(ee);
	auto gw = ctp_gateway();
	me->add_gateway(gw);
	auto stg = cta_strategy();
	me->add_app(stg);
	le->info("start main engine");

	gw->subscribe(std::bind(MainEngine::on_message, me),
			std::bind(MainEngine::on_error, me),
			std::bind(MainEngine::on_complete, me));
	gw->start();
	le->info("start CTP gateway");

	

}

