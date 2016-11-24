set(LLVM_LINK_COMPONENTS Core Support)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
set(Python_ADDITIONAL_VERSIONS 3.3 3.4 3.5 3.6)

if (APPLE)
	set(Boost_USE_STATIC_LIBS OFF) # Needs a fix, but no idea how.
	find_package(PythonLibs 3.3 EXACT REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
else ()
	set(Boost_USE_STATIC_LIBS ON) # This is the desired property.
	set(BOOST_ROOT /usr/local)
	find_package(PythonLibs 3.3 EXACT REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
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
	Session.cpp)

add_library(Clara SHARED ${source_files})

if (APPLE)
	set_target_properties(Clara PROPERTIES SUFFIX ".so")
endif ()
set_target_properties(Clara PROPERTIES PREFIX "")

target_include_directories(Clara PUBLIC ${PYTHON_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(Clara PUBLIC clangTooling ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})
if (UNIX)
	target_link_libraries(Clara PUBLIC -lutil)
	# target_link_libraries(testmodule PUBLIC -lutil)
endif()

install(TARGETS Clara DESTINATION "${claraInstallFolder}")
add_custom_target(ClaraInstall 
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 
add_dependencies(ClaraInstall Clara ClaraPlugin) 