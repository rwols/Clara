cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(sublime-ide)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(source_files
	src/ExampleASTConsumer.cpp
	src/ExampleFrontendAction.cpp
	src/ExampleFrontendActionFactory.cpp
	src/ExampleVisitor.cpp
	src/main.cpp)

find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

add_executable(refactor ${source_files})

target_include_directories(refactor PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(refactor PUBLIC ${Clang_INCLUDE_DIRS})

include(TargetSetRTTI)
target_set_rtti(refactor OFF)

target_compile_options(refactor PUBLIC "-Wall")
target_compile_options(refactor PUBLIC "-Werror")

target_link_libraries(refactor PUBLIC ${Clang_LIBRARIES})
target_link_libraries(refactor PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(refactor PUBLIC ${LLVM_SYSTEM_LIBRARIES})

add_subdirectory(test)