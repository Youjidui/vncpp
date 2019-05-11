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

time_t time_from_iso_string(const std::string& dt)
{
    auto sd = boost::gregorian::date_from_iso_string(dt);
    struct tm dtm = boost::gregorian::to_tm(sd);
    return mktime(&dtm);
}

int main(int argc, char* argv[])
{
    auto engine = std::make_shared<BacktestingEngine>();
    engine->setBacktestingMode(BacktestingEngine::BAR_MODE);
    auto t = time_from_iso_string("20120101");
    engine->setStartDate(t);
    engine->setSlippage(0.2);
    engine->setRate(0.3/10000);
    engine->setSize(300);
    engine->setPriceTick(0.2);

    engine->setDatabase(MINUTE_DB_NAME, "IF0000");

    auto stg = std::make_shared<StrategyKingKeltner>();
    engine->initStrategy(stg);

    engine->runBacktesting();

    engine->showBacktestingResult();

    return 0;
}

