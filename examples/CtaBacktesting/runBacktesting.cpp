#include <stdint.h>
#include <stdio.h>
#include <functional>
//#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <logging.h>
#include "eventEngine.h"
#include "vtEngine.h"
//#include "ctp/ctpGateway.h"
#include "BacktestingEngine.h"

const std::string MINUTE_DB_NAME = "IF0000_1min.csv";

const std::string config = 
"{\n"
"\"strategies\" : [\n"
"{\n"
"\"instanceName\" : \"StrategyKingKeltner\",\n"
"\"className\" : \"StrategyKingKeltner\",\n"
"\"filePath\" : \"./strategy/libCtaStrategy.so\",\n"
"\"parameters\" : {\n"
"\"vtSymbol\" : \"IF0000\",\n"
"\"kkLength\" : \"1\",\n"
"\"kkDev\" : \"1.0\"\n"
"}\n"
"]\n"
"}";

const std::string configFileName = "./backtesting.config";

time_t time_from_iso_string(const std::string& dt)
{
    auto sd = boost::gregorian::date_from_iso_string(dt);
    struct tm dtm = boost::gregorian::to_tm(sd);
    return mktime(&dtm);
}

int main(int argc, char* argv[])
{
	printf("----------------------------\n");
	LOG_INFO << "start CTA strategy backtesting\n";
	auto ee = eventEngine();
	ee->start(false);
	LOG_INFO << "start event engine without working thread";
	auto me = mainEngine(ee);
	LOG_INFO << "start main engine";

    auto engine = std::make_shared<BacktestingEngine>(ee, me);
	me->addApp(engine);

	auto pe = persistenceEngine();
	pe->setParameter("file:///mnt/c/Source/vncpp/examples/CtaBacktesting/fileDB/");
	pe->dbConnect();
	engine->initHdsClient(pe);
    engine->setBacktestingMode(BacktestingEngine::BAR_MODE);
	const std::string dt("20120101");
    //auto t = time_from_iso_string(dt);
    engine->setStartDate(dt);
    engine->setSlippage(0.2);
    engine->setRate(0.3/10000);
    engine->setContractSize(300);
    engine->setPriceTick(0.2);

    engine->setDatabase(MINUTE_DB_NAME, "IF0000");
	engine->loadSettings(configFileName);
	LOG_INFO << "load CTA backtesting settings";
    //auto stg = std::make_shared<StrategyKingKeltner>();
    //engine->initStrategy(stg);
	engine->initAll();
	LOG_INFO << "init CTA";
	engine->startAll();

    engine->runBacktesting();

    engine->showBacktestingResult();

	LOG_INFO << argv[0] << " completed ";
    return 0;
}

