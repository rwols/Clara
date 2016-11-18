set(LLVM_LINK_COMPONENTS Core Support)

set(Python_ADDITIONAL_VERSIONS 3.3 3.4 3.5 3.6)
set(Boost_USE_STATIC_LIBS ON)
if (APPLE)
	find_package(Python3 REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
else ()
	set(BOOST_ROOT /usr/local)
	find_package(PythonLibs 3.5 REQUIRED)
	# find_package(Python3 REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
endif ()

set(source_files
	CancellableASTConsumer.cpp
	CancellableSyntaxOnlyAction.cpp
	CodeCompleteConsumer.cpp
	DiagnosticConsumer.cpp
	PythonBindings.cpp
	RenameFunctionASTConsumer.cpp
	RenameFunctionFrontendAction.cpp
	RenameFunctionFrontendActionFactory.cpp
	RenameFunctionVisitor.cpp
	Session.cpp)

add_clang_library(Clara SHARED ${source_files} 
	LINK_LIBS 
	clangBasic 
	clangLex 
	clangSema 
	clangFrontend 
	clangTooling)

if (APPLE)
	set_target_properties(Clara PROPERTIES SUFFIX ".so")
endif ()
set_target_properties(Clara PROPERTIES PREFIX "")

target_include_directories(Clara PUBLIC ${PYTHON_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(Clara PUBLIC ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})

install(TARGETS Clara DESTINATION $ENV{HOME}/.config/sublime-text-3/Packages/Clara)
add_custom_target(ClaraInstall 
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 
add_dependencies(ClaraInstall Clara ClaraPlugin) 