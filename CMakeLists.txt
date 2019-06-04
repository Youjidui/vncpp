PROJECT(vncpp)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(Boost_DEBUG 1)
#find_package(Boost COMPONENTS date_time filesystem system log asio)
find_package(Boost COMPONENTS date_time filesystem system log )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  #target_link_libraries(foo ${Boost_LIBRARIES})
  MESSAGE(STATUS "Find Boost: ${Boost_LIBRARIES} in ${Boost_LIBRARY_DIRS}")
else()
  MESSAGE(FATAL_ERROR "Cannot find boost library")
endif()
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_ALL_DYN_LINK)

ADD_SUBDIRECTORY(vncpp)
ADD_SUBDIRECTORY(examples)


set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

