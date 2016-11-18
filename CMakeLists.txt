# PLEASE READ!
#
# Make sure that the folder that contains this file
# is placed in the folder /path/to/llvm/tools/clang/tools/extra/
# Also make sure to enable exceptions and RTTI.
# You can either do this with ccmake or via the command line.
# The relevant CMake variables are LLVM_ENABLE_EH and LLVM_ENABLE_RTTI.
#
# To only build this library and not the whole LLVM framework,
# do something like this on the command line:
#
# $ mkdir build
# $ cd build
# $ cmake .. -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON
# $ make Clara -j8
#
# This builds only this library (and all of its dependencies)
# with 8 threads. It should not take more than 3 minutes, while
# doing a "naked" make can easily keep compiling for more than an hour.

# set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib)
add_subdirectory(include)
add_subdirectory(bin)
add_subdirectory(test)