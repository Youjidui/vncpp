AUX_SOURCE_DIRECTORY(. DIR_SRC)

ADD_LIBRARY(vTrader SHARED ${DIR_SRC})
add_dependencies(vTrader vEvent log)
target_link_libraries(vTrader vEvent log ${Boost_LIBRARIES})

