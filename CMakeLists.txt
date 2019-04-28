PROJECT(vncpp)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

find_package(Boost COMPONENTS date_time filesystem system log asio)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  #target_link_libraries(foo ${Boost_LIBRARIES})
else()
  MESSAGE(FATAL_ERROR "Cannot find boost library")
endif()

ADD_SUBDIRECTORY(vncpp)
ADD_SUBDIRECTORY(examples)


set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

