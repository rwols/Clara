set(LLVM_LINK_COMPONENTS Core Support)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(source_files
	CodeCompleteConsumer.cpp
	CompilationDatabase.cpp
	DiagnosticConsumer.cpp
	PythonBindings.cpp
	Session.cpp
	SessionOptions.cpp)

set(PYBIND11_PYTHON_VERSION 3.3)
set(PYTHON_EXECUTABLE python${PYBIND11_PYTHON_VERSION})
add_subdirectory(pybind11)
pybind11_add_module(Clara ${source_files})
target_link_libraries(Clara LINK_PRIVATE clangFrontend clangTooling)
file(GLOB headers ../../../lib/Headers/*.h)

install(TARGETS Clara 
	DESTINATION ${claraInstallFolder}
	PERMISSIONS OWNER_READ)

install(FILES ${headers} 
	DESTINATION ${claraInstallFolder}/include 
	PERMISSIONS OWNER_READ)

add_custom_target(ClaraInstall 
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 

add_dependencies(ClaraInstall Clara ClaraPlugin) 