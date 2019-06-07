AUX_SOURCE_DIRECTORY(. SRC)
ADD_LIBRARY(log SHARED ${SRC})
#add_dependencies(log boost_log)
target_link_libraries(log ${Boost_LIBRARIES})

install(TARGETS log  DESTINATION .)

#include(cmake/copy_target.cmake)
copy_target(log)


