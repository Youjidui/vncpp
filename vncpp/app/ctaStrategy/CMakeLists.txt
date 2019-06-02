AUX_SOURCE_DIRECTORY(. DIR_SRC)
ADD_LIBRARY(ctaEngine SHARED ${DIR_SRC})
add_dependencies(ctaEngine log vEvent vTrader )
target_link_libraries( ctaEngine log vEvent vTrader ${Boost_LIBRARIES} dl)

ADD_SUBDIRECTORY(strategy)

