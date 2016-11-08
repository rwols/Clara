cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(Clara CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(test)