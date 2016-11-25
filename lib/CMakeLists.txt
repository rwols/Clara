set(LLVM_LINK_COMPONENTS Core Support)
if (CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf")
endif()

set(source_files
	CancellableASTConsumer.cpp
	CancellableSyntaxOnlyAction.cpp
	CodeCompleteConsumer.cpp
	DiagnosticConsumer.cpp
	PythonBindings.cpp
	PythonGILEnsurer.cpp
	RenameFunctionASTConsumer.cpp
	RenameFunctionFrontendAction.cpp
	RenameFunctionFrontendActionFactory.cpp
	RenameFunctionVisitor.cpp
	Session.cpp
	SessionOptions.cpp)

set(PYBIND11_PYTHON_VERSION 3.3)
set(PYTHON_EXECUTABLE python${PYBIND11_PYTHON_VERSION})
add_subdirectory(pybind11)
pybind11_add_module(Clara ${source_files})
target_link_libraries(Clara LINK_PRIVATE clangFrontend clangTooling)
install(TARGETS Clara DESTINATION "${claraInstallFolder}")
add_custom_target(ClaraInstall 
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 
add_dependencies(ClaraInstall Clara ClaraPlugin) 