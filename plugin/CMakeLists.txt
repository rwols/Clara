install(DIRECTORY . DESTINATION "${claraInstallFolder}")

add_custom_target(ClaraPlugin
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake)
add_dependencies(ClaraPlugin Clara)