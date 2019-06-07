AUX_SOURCE_DIRECTORY(. CTA_SRC)
ADD_LIBRARY(ctaEngine SHARED ${CTA_SRC})
add_dependencies(ctaEngine log vEvent vTrader )
target_link_libraries( ctaEngine log vEvent vTrader ${Boost_LIBRARIES} dl)

ADD_SUBDIRECTORY(strategy)

install(TARGETS ctaEngine DESTINATION .)
copy_target(ctaEngine)
