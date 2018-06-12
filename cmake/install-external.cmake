install(CODE "
	execute_process(
		COMMAND python3 \"${CMAKE_SOURCE_DIR}/cmake/install-share.py\" \"${CMAKE_INSTALL_PREFIX}\"
		RESULT_VARIABLE INSTALL_SHARE_RES
	)
	if(INSTALL_SHARE_RES)
		message(FATAL_ERROR \"RetDec share directory installation FAILED\")
	endif()
")
