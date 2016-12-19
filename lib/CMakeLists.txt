# pybind11 demands exceptions and RTTI
unset(LLVM_REQUIRES_RTTI CACHE)
unset(LLVM_REQUIRES_EH CACHE)
unset(CLANG_RESOURCE_DIR CACHE)
set(LLVM_REQUIRES_RTTI ON CACHE INTERNAL "Enable run time type information" )
set(LLVM_REQUIRES_EH ON CACHE INTERNAL "Enable Exception handling")
set(CLANG_RESOURCE_DIR ${CMAKE_INSTALL_PREFIX}/lib CACHE INTERNAL
  "Relative directory from the Clang binary to its resource files.")

set(LLVM_LINK_COMPONENTS Core Support)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# We need to have an exact version match for Sublime Text otherwise Python will
# complain about threads not being initialized.
set(PYBIND11_PYTHON_VERSION 3.3)
set(PYTHON_EXECUTABLE python${PYBIND11_PYTHON_VERSION})

# Override this variable in the cache so that the resource directory is found.
# We're doing an absolute path because plugins are run from the plugin_host
# executable. That executable resides in a folder unrelated to the usual
# /usr/local/lib, so we just write an absolute path.

add_subdirectory(pybind11)

set(source_files
	CodeCompleteConsumer.cpp
	CompilationDatabase.cpp
	DiagnosticConsumer.cpp
	PythonBindings.cpp
	Session.cpp
	SessionOptions.cpp)

pybind11_add_module(Clara ${source_files})
target_link_libraries(Clara LINK_PRIVATE clangFrontend clangTooling)

file(GLOB_RECURSE builtin_headers ../../../lib/Headers/*.h)
install(FILES ${builtin_headers}
	DESTINATION "${claraInstallFolder}/include")

install(TARGETS Clara 
	DESTINATION "${claraInstallFolder}")

add_custom_target(ClaraInstall 
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 

add_dependencies(ClaraInstall Clara ClaraPlugin)