INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/log)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app/ctaStrategy)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/gateway)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/gateway/ctp)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/data)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/event)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/trader)
#why need CTP_DIR????
if(WIN32)
else(WIN32)
set(CTP_DIR ${PROJECT_SOURCE_DIR}/3rd/ctpapi/x64_linux/)
message("${CTP_DIR}")
INCLUDE_DIRECTORIES(${CTP_DIR})
LINK_DIRECTORIES(${CTP_DIR})
endif(WIN32)


aux_source_directory(. src)

add_executable(ctaBacktesting ${src})
add_dependencies(ctaBacktesting ctaStrategy ctaEngine vTrader vEvent log )
#target_link_libraries(ctaBacktesting vTrader vEvent log ${Boost_LIBRARIES})
target_link_libraries(ctaBacktesting ctaEngine vTrader vEvent log vCtp 	Boost::log Boost::filesystem Boost::date_time)

install(TARGETS ctaBacktesting DESTINATION .)
install(FILES backtesting.config DESTINATION .)

#include(cmake/copy_target.cmake)
copy_target(ctaBacktesting)

