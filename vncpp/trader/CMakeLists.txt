AUX_SOURCE_DIRECTORY(. VT_SRC)

ADD_LIBRARY(vTrader SHARED ${VT_SRC})
add_dependencies(vTrader vEvent log)
target_link_libraries(vTrader vEvent log ${Boost_LIBRARIES}) 
#target_link_libraries(vTrader vEvent log ${Boost_LIBRARIES} Boost::log Boost::filesystem Boost::date_time)

install(TARGETS vTrader DESTINATION .)
copy_target(vTrader)
