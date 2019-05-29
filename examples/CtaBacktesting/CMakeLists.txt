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

add_executable(ctaBacktesting ${src})
add_dependencies(ctaBacktesting vTrader vEvent log )
#target_link_libraries(ctaBacktesting vTrader vEvent log ${Boost_LIBRARIES})
target_link_libraries(ctaBacktesting vTrader vEvent log ctaStrategy vCtp 
	Boost::log Boost::filesystem Boost::date_time)



