AUX_SOURCE_DIRECTORY(. DIR_SRC)
ADD_LIBRARY(ctaStrategy SHARED ${DIR_SRC})
add_dependencies(ctaStrategy log vEvent vTrader )
target_link_libraries( ctaStrategy log vEvent vTrader ${Boost_LIBRARIES})

