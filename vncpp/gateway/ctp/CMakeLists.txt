AUX_SOURCE_DIRECTORY(. SRC)
ADD_LIBRARY(vCtp SHARED ${SRC})
add_dependencies(vCtp vEvent log vTrader )
target_link_libraries(vCtp vEvent log vTrader ${Boost_LIBRARIES})

install(TARGETS vCtp DESTINATION .)
copy_target(vCtp)
