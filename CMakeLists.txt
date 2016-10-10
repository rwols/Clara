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

add_executable(hello ${source_files})

target_include_directories(hello PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(hello PUBLIC ${Clang_INCLUDE_DIRS})

# target_compile_options(hello PUBLIC "-fno-rtti")

include(TargetSetRTTI)
target_set_rtti(hello OFF)

# target_compile_definitions(hello PUBLIC -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

target_link_libraries(hello PUBLIC ${Clang_LIBRARIES})
target_link_libraries(hello PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(hello PUBLIC ${LLVM_SYSTEM_LIBRARIES})