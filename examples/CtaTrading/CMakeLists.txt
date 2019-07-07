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
INCLUDE_DIRECTORIES(${CTP_DIR})
LINK_DIRECTORIES(${CTP_DIR})
endif(WIN32)


aux_source_directory(. ct_src)

add_executable(ctaTrading ${ct_src})
add_dependencies(ctaTrading ctaEngine vTrader vEvent log )
#target_link_libraries(ctaTrading vTrader vEvent log ${Boost_LIBRARIES})
#target_link_libraries(ctaTrading ctaEngine vCtp vTrader vEvent log 	Boost::log Boost::filesystem Boost::thread Boost::system Boost::date_time pthread)
target_link_libraries(ctaTrading ctaEngine vCtp vTrader vEvent log 	boost_log boost_filesystem boost_thread boost_system  boost_date_time pthread)

install(TARGETS ctaTrading DESTINATION .)

copy_target(ctaTrading)
