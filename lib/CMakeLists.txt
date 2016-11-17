if (NOT LLVM_FOUND)
	message(FATAL_ERROR "LLVM was not found. Required to build.")
endif()

configure_file(Configuration.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Configuration.hpp)

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

add_library(cpp SHARED ${source_files})

if (APPLE)
	set_target_properties(cpp PROPERTIES SUFFIX ".so")
endif ()
set_target_properties(cpp PROPERTIES COMPILE_FLAGS "${LLVM_CXX_FLAGS}")
set_target_properties(cpp PROPERTIES LINK_FLAGS "${LLVM_LD_FLAGS}")
set_target_properties(cpp PROPERTIES PREFIX "")
set_target_properties(cpp PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(cpp PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(cpp PUBLIC ${Clang_INCLUDE_DIRS})
target_include_directories(cpp PUBLIC ${PYTHON_INCLUDE_DIRS})
target_include_directories(cpp PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(cpp PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(cpp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)

# Warn about everything.
target_compile_options(cpp PUBLIC "-Wall")

# Uncomment this to turn all warnings into errors,
# target_compile_options(cpp PUBLIC "-Werror")

target_link_libraries(cpp PUBLIC ${Clang_LIBRARIES})
target_link_libraries(cpp PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(cpp PUBLIC ${LLVM_SYSTEM_LIBRARIES})
target_link_libraries(cpp PUBLIC ${PYTHON_LIBRARIES})
target_link_libraries(cpp PUBLIC ${Boost_LIBRARIES})

# include(cotire)
# cotire(cpp)

