AUX_SOURCE_DIRECTORY(. SRC)
ADD_LIBRARY(vCtp SHARED ${SRC})
add_dependencies(vCtp vEvent log )
target_link_libraries(vCtp vEvent log ${Boost_LIBRARIES})

