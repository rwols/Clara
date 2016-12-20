
set(SUBLIME_VERSION 3126)

if (APPLE)
	set(SUBLIME_PLATFORM osx)
elseif (WIN32)
	set(SUBLIME_PLATFORM windows)
elseif (UNIX)
	set(SUBLIME_PLATFORM linux)
endif (APPLE)

configure_file(Configuration.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Configuration.hpp)
target_include_directories(Clara PUBLIC 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<INSTALL_INTERFACE:include/Clara>)
target_include_directories(Clara PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
