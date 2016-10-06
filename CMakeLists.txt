cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(sublime-ide)

set(CMAKE_CXX_STANDARD 14)

find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

add_executable(hello src/main.cpp)