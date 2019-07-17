if(WIN32)
else(WIN32)
set(CTP_DIR ${PROJECT_SOURCE_DIR}/3rd/ctpapi/x64_linux/)
message("${CTP_DIR}")
INCLUDE_DIRECTORIES(${CTP_DIR})
LINK_DIRECTORIES(${CTP_DIR})
set(ctpmd thostmduserapi)
set(ctptd thosttraderapi)
endif(WIN32)

AUX_SOURCE_DIRECTORY(. CTP_SRC)
ADD_LIBRARY(vCtp SHARED ${CTP_SRC})
add_dependencies(vCtp vEvent log vTrader )
target_link_libraries(vCtp vEvent log vTrader boost_locale ${Boost_LIBRARIES} ${ctpmd} ${ctptd})

install(TARGETS vCtp DESTINATION .)
install(FILES
	${CTP_DIR}/libthostmduserapi.so
	${CTP_DIR}/libthosttraderapi.so
	DESTINATION .)
copy_target(vCtp)
