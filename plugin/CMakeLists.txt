configure_file(__init__.py.in "${CMAKE_CURRENT_BINARY_DIR}/__init__.py")

install(
    DIRECTORY
        . 
	DESTINATION
        "${claraInstallFolder}"
	PATTERN CMakeLists.txt EXCLUDE
    PATTERN *.in EXCLUDE)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/__init__.py"
    DESTINATION
        "${claraInstallFolder}")

add_custom_target(ClaraPlugin
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake)
add_dependencies(ClaraPlugin Clara)
