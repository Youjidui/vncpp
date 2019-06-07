function(copy_target target_name)
	if(WIN32)
	    set(COPY copy)
	else()
	    set(COPY cp)
	endif(WIN32)

	ADD_CUSTOM_COMMAND(
	    TARGET ${target_name}
	    POST_BUILD
	    COMMAND ${COPY} ARGS $<TARGET_FILE:${target_name}> ${PROJECT_BINARY_DIR}
	)
endfunction()

