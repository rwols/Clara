# PLEASE READ!
#
# Make sure that the folder that contains this file
# is placed in the folder /path/to/llvm/tools/clang/tools
# Also make sure to enable exceptions and RTTI.
# You can either do this with ccmake or via the command line.
# The relevant CMake variables are LLVM_ENABLE_EH and LLVM_ENABLE_RTTI.
#
# To only build this library and not the whole LLVM framework,
# do something like this on the command line:
#
# $ mkdir build
# $ cd build
# $ cmake .. -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DCLANG_RESOURCE_DIR=/usr/local/lib/clang/4.0.0
# $ make ClaraInstall -j8
#
# This builds only this library (and all of its dependencies)
# with 8 threads. It should not take more than 3 minutes, while
# doing a "naked" make can easily keep compiling for more than an hour.

# At least version 3.2 is needed because we want to use CMake's built-in zip
# functionality. This is available since version 3.2.
cmake_minimum_required(VERSION 3.2)
project(Clara VERSION 0.2.0 LANGUAGES CXX)

option(Clara_DEPLOY "Enable deployment to GitHub" OFF)

# Find Sublime Text. No point in building Clara when Sublime is not present.
find_program(SUBLIME_EXECUTABLE
    NAMES
        subl
    DOC
        "Sublime Text executable"
    )

if (SUBLIME_EXECUTABLE)
    message(STATUS "Found Sublime Text: ${SUBLIME_EXECUTABLE}")
else()
    message(FATAL_ERROR "Could not find Sublime Text executable.")
endif()

execute_process(
    COMMAND 
        ${SUBLIME_EXECUTABLE} --version
    OUTPUT_VARIABLE 
        SUBLIME_VERSION)

string(REGEX REPLACE
    "Sublime Text Build ([0-9]+)"
    "\\1" SUBLIME_VERSION "${SUBLIME_VERSION}")

string(STRIP ${SUBLIME_VERSION} SUBLIME_VERSION)
set(SUBLIME_VERSION ${SUBLIME_VERSION} CACHE INTERNAL "Sublime Text version")

message(STATUS "Sublime Text version: ${SUBLIME_VERSION}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (APPLE)
    set(SUBLIME_PLATFORM osx CACHE INTERNAL "")
	set(claraInstallFolderPrefix "$ENV{HOME}/Library/Application Support/Sublime Text 3")
elseif(WIN32)
    set(SUBLIME_PLATFORM windows CACHE INTERNAL "")
    set(claraInstallFolderPrefix "$ENV{HOME}/AppData/Roaming/Sublime Text 3")
elseif(UNIX)
    set(SUBLIME_PLATFORM linux CACHE INTERNAL "")
	set(claraInstallFolderPrefix "$ENV{HOME}/.config/sublime-text-3")
else()
    message(FATAL_ERROR "Unsupported Sublime Text platform.")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SUBLIME_PLATFORM_EXT ${SUBLIME_PLATFORM}-x64 CACHE INTERNAL "")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(SUBLIME_PLATFORM_EXT ${SUBLIME_PLATFORM}-x32 CACHE INTERNAL "")
else()
    message(FATAL_ERROR "Unknown bitwidth! (Neither 32 nor 64)")
endif()

message(STATUS "Sublime Text platform: ${SUBLIME_PLATFORM_EXT}")

set(currentSublimePackagesFolder "${claraInstallFolderPrefix}/Packages")
set(claraInstallFolder "${currentSublimePackagesFolder}/Clara")

mark_as_advanced(claraInstallFolderPrefix)
mark_as_advanced(claraInstallFolder)

add_subdirectory(plugin)
add_subdirectory(lib)
add_subdirectory(include)
add_subdirectory(test)

if (${Clara_DEPLOY})
    add_subdirectory(deploy)
endif()


