PROJECT(vncpp)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
SET(CMAKE_SKIP_BUILD_RPATH  TRUE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "./")
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)


set(Boost_DEBUG ON)
SET(BUILD_SHARED_LIBS ON)
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS log filesystem locale thread system atomic date_time )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  #target_link_libraries(foo ${Boost_LIBRARIES})
  MESSAGE(STATUS " *** Find Boost: ${Boost_LIBRARIES} in ${Boost_LIBRARY_DIRS} ***")
  MESSAGE(STATUS " *** Find Boost: ${Boost_LIBRARIES} in ${BOOST_LIBRARYDIR}, ${Boost_LIBRARY_DIR_RELEASE} ***")
  MESSAGE(STATUS " *** Boost: ${Boost_VERSION}, ${Boost_LIB_VERSION}, ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}, ${Boost_INCLUDE_DIRS}, ${Boost_LIBRARIES}, ${Boost_log_LIBRARY_RELEASE} ***")
else()
  MESSAGE(FATAL_ERROR "Cannot find boost library")
endif()

#if("1.70.0" STREQUAL ${Boost_VERSION})
#else()
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_ALL_DYN_LINK)
#endif()

add_definitions(-pipe -ggdb -fPIC -Wall)
add_definitions(-Wl,--as-needed)

include(cmake/copy_target.cmake)


ADD_SUBDIRECTORY(vncpp)
ADD_SUBDIRECTORY(examples)


IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()


set(CPACK_GENERATOR "tz")
set(CPACK_PACKAGE_NAME "vncpp")
set(CPACK_PACKAGE_VERSION "0.1")
set(CPACK_SET_DESTDIR true)
#set(CPACK_INSTALL_PREFIX "/opt/vncpp/")
set(CPACK_PACKAGE_DESCRIPTION "a simple trading framework adapt from vnpy")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY, "trading framework")
#SET(CPACK_SYSTEM_NAME ${PLATFORM})

INCLUDE(InstallRequiredSystemLibraries)

INCLUDE(CPack)


