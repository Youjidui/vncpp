INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/vncpp/app/ctaStrategy)

AUX_SOURCE_DIRECTORY(. CS_SRC)
ADD_LIBRARY(ctaStrategy SHARED ${CS_SRC})
add_dependencies(ctaStrategy ctaEngine log vEvent vTrader )
target_link_libraries( ctaStrategy ctaEngine log vEvent vTrader ${Boost_LIBRARIES} )

install(TARGETS ctaStrategy DESTINATION ./strategy/)

copy_target_to(ctaStrategy ${PROJECT_BINARY_DIR}/strategy/)
#copy_target_to(ctaStrategy ${PROJECT_BINARY_DIR})
