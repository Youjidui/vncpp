INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/log)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/gateway)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/data)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/event)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/trader)

message(STATUS "${Boost_LIBRARIES}")

aux_source_directory(. src)

add_executable(ctaTrading ${src})
add_dependencies(ctaTrading vTrader vEvent log )
target_link_libraries(ctaTrading vTrader vEvent log ${Boost_LIBRARIES})



