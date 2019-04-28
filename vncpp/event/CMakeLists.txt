AUX_SOURCE_DIRECTORY(. DIR_SRC)
ADD_LIBRARY(vEvent SHARED ${DIR_SRC})
add_dependencies(vEvent log )
target_link_libraries( vEvent log ${Boost_LIBRARIES})

