AUX_SOURCE_DIRECTORY(. ALGO_SRC)
ADD_LIBRARY(algoEngine SHARED ${ALGO_SRC})
add_dependencies(algoEngine log vEvent vTrader )
target_link_libraries( algoEngine log vEvent vTrader ${Boost_LIBRARIES} dl)

#ADD_SUBDIRECTORY(strategy)

#install(algoEngine DESTINATION ${PROJECT_BINARY_DIR})
install(algoEngine DESTINATION .)
copy_target(algoEngine)
