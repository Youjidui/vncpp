AUX_SOURCE_DIRECTORY(. E_SRC)
ADD_LIBRARY(vEvent SHARED ${E_SRC})
add_dependencies(vEvent log )
target_link_libraries( vEvent log ${Boost_LIBRARIES})

install(TARGETS vEvent DESTINATION .)

