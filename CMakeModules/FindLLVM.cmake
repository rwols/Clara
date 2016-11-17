# Find LLVM
#
# It defines the following variables
# LLVM_FOUND         - True if LLVM found.
# LLVM_INCLUDE_DIRS  - where to find LLVM include files, use in conjunction with target_include_directories
# LLVM_CORE_LIBRARIES - where to find the  LLVM core libraries
# LLVM_LIBRARIES     - where to find LLVM libs, use in conjunction with target_link_libraries
# LLVM_VERSION_MAJOR - The major version number
# LLVM_VERSION_MINOR - The minor version number
# LLVM_VERSION       - Is the same as ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}
# LLVM_CXX_FLAGS     - Required compilation flags
# LLVM_LD_FLAGS      - Required linker flags

find_program(LLVM_CONFIG_EXECUTABLE llvm-config DOC "llvm-config executable")

if (NOT LLVM_CONFIG_EXECUTABLE)
	message(FATAL_ERROR "Could NOT find LLVM config utility.")
endif ()

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
	OUTPUT_VARIABLE LLVM_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\1" LLVM_VERSION_MAJOR "${LLVM_VERSION}")
string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\2" LLVM_VERSION_MINOR "${LLVM_VERSION}")

# You might wonder why not just use LLVM_VERSION in the first place.
# Well this strips the characters "svn" from the version string that
# may be present.
set(LLVM_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}")

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
	OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
	OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles
	OUTPUT_VARIABLE LLVM_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles core
	OUTPUT_VARIABLE LLVM_CORE_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_CORE_LIBRARIES "${LLVM_CORE_LIBRARIES}")

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
	OUTPUT_VARIABLE LLVM_SYSTEM_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_SYSTEM_LIBRARIES "${LLVM_SYSTEM_LIBRARIES}")

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
	OUTPUT_VARIABLE LLVM_CXX_FLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
	OUTPUT_VARIABLE LLVM_LD_FLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LLVM 
	FOUND_VAR LLVM_FOUND
	REQUIRED_VARS LLVM_LIBRARIES LLVM_INCLUDE_DIRS LLVM_SYSTEM_LIBRARIES LLVM_CORE_LIBRARIES
	VERSION_VAR LLVM_VERSION)

mark_as_advanced(LLVM_CONFIG_EXECUTABLE)
mark_as_advanced(LLVM_FOUND)
mark_as_advanced(LLVM_VERSION)
mark_as_advanced(LLVM_VERSION_MAJOR)
mark_as_advanced(LLVM_VERSION_MINOR)
mark_as_advanced(LLVM_INCLUDE_DIRS)
mark_as_advanced(LLVM_LIBRARIES)
mark_as_advanced(LLVM_CFLAGS)
mark_as_advanced(LLVM_LDFLAGS)
mark_as_advanced(LLVM_SYSTEM_LIBRARIES)
mark_as_advanced(LLVM_CORE_LIBRARIES)
