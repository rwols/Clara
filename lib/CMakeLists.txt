find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)
find_package(PythonLibs 3.5 REQUIRED)
find_package(Boost COMPONENTS python-py35 REQUIRED)

configure_file(Configuration.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Configuration.hpp)

set(source_files
	PythonBindings.cpp
	RenameFunctionASTConsumer.cpp
	RenameFunctionFrontendAction.cpp
	RenameFunctionFrontendActionFactory.cpp
	RenameFunctionVisitor.cpp
	Project.cpp)

add_library(Clara SHARED ${source_files})
set_target_properties(Clara PROPERTIES PREFIX "")

target_include_directories(Clara PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${Clang_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${PYTHON_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(Clara PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(Clara PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)

# Warn about everything.
target_compile_options(Clara PUBLIC "-Wall")

# Uncomment this to turn all warnings into errors,
# target_compile_options(Clara PUBLIC "-Werror")

target_link_libraries(Clara PUBLIC ${Clang_LIBRARIES})
target_link_libraries(Clara PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(Clara PUBLIC ${LLVM_SYSTEM_LIBRARIES})
target_link_libraries(Clara PUBLIC ${PYTHON_LIBRARIES})
target_link_libraries(Clara PUBLIC ${Boost_LIBRARIES})

include(cotire)
cotire(Clara)