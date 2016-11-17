cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(Clara CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
if (APPLE)
	find_package(Python3 REQUIRED)
	find_package(Boost COMPONENTS python3 REQUIRED)
else ()
	find_package(PythonLibs 3.5 REQUIRED)
	find_package(Boost COMPONENTS python-py35 REQUIRED)
endif ()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(test)