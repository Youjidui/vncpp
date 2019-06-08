function(copy_target_to target_name dest_dir)
	if(WIN32)
	    set(COPY copy)
	else()
	    set(COPY cp)
	endif(WIN32)

	ADD_CUSTOM_COMMAND(
	    TARGET ${target_name}
		POST_BUILD
		#COMMAND "[ -d \"${dest_dir}\" ] || mkdir \"${dest_dir}\" "
	    COMMAND ${COPY} ARGS $<TARGET_FILE:${target_name}> ${dest_dir}
	)
endfunction()


function(copy_target target_name)
	copy_target_to(${target_name} ${PROJECT_BINARY_DIR})
endfunction()
