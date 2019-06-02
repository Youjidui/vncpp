INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/log)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app/ctaStrategy)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/gateway)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/gateway/ctp)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/data)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/event)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/trader)


find_package(Boost COMPONENTS date_time filesystem system log asio)
message(STATUS "find the following boost libraries: ${Boost_LIBRARIES}")

aux_source_directory(. src)

add_executable(ctaTrading ${src})
add_dependencies(ctaTrading ctaEngine vTrader vEvent log )
#target_link_libraries(ctaTrading vTrader vEvent log ${Boost_LIBRARIES})
target_link_libraries(ctaTrading ctaEngine vTrader vEvent log vCtp 
	Boost::log Boost::filesystem Boost::date_time)



