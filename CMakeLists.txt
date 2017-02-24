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

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(Clara VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (APPLE)
	set(claraInstallFolderPrefix "$ENV{HOME}/Library/Application Support/Sublime Text 3")
else()
	set(claraInstallFolderPrefix "$ENV{HOME}/.config/sublime-text-3")
endif()
set(currentSublimePackagesFolder "${claraInstallFolderPrefix}/Packages")
set(claraInstallFolder "${currentSublimePackagesFolder}/Clara")

mark_as_advanced(claraInstallFolderPrefix)
mark_as_advanced(claraInstallFolder)

add_subdirectory(plugin)
add_subdirectory(lib)
add_subdirectory(include)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(Clara_arch x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(Clara_arch x86)
else()
	message(FATAL_ERROR "Unknown architecture! (Neither 32 nor 64 bits)")
endif()

set(Clara_version ${PROJECT_NAME}-${PROJECT_VERSION})
set(Clara_package_name ${Clara_version}-${CMAKE_SYSTEM_NAME}-${Clara_arch}.tar.tgz)
set(Clara_tar_output "${CMAKE_BINARY_DIR}/${Clara_package_name}")

add_custom_target(ClaraPackage
	COMMAND ${CMAKE_COMMAND} -E tar zcvf "${Clara_tar_output}" Clara
	DEPENDS ClaraInstall
	WORKING_DIRECTORY "${currentSublimePackagesFolder}"
	COMMENT "Packaging Clara")
