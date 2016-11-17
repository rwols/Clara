find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
if (APPLE)
	message(STATUS "We're on apple.")
	find_package(Python3 REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
else ()
	message(STATUS "assuming unix")
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
	#RenameFunctionASTConsumer.cpp
	#RenameFunctionFrontendAction.cpp
	#RenameFunctionFrontendActionFactory.cpp
	#RenameFunctionVisitor.cpp
	Session.cpp)

add_library(Clara SHARED ${source_files})

if (APPLE)
	set_target_properties(Clara PROPERTIES SUFFIX ".so")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXX_FLAGS}")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${LLVM_LD_FLAGS}")
# set_target_properties(Clara PROPERTIES COMPILE_FLAGS "${LLVM_CXX_FLAGS}")
# set_target_properties(Clara PROPERTIES LINK_FLAGS "${LLVM_LD_FLAGS}")
set_target_properties(Clara PROPERTIES PREFIX "")
set_target_properties(Clara PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(Clara PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${Clang_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${PYTHON_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${Boost_INCLUDE_DIRS})

# Warn about everything.
target_compile_options(Clara PUBLIC "-Wall")

# Uncomment this to turn all warnings into errors,
# target_compile_options(Clara PUBLIC "-Werror")

target_link_libraries(Clara PUBLIC ${Clang_LIBRARIES})
target_link_libraries(Clara PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(Clara PUBLIC ${PYTHON_LIBRARIES})
target_link_libraries(Clara PUBLIC ${Boost_LIBRARIES})

# include(cotire)
# cotire(cpp)

