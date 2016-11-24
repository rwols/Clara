
#if (APPLE)
#	find_package(DL)
#	find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
#	if (NOT COREFOUNDATION_FRAMEWORK)
#		message(FATAL_ERROR "Could not find CoreFoundation framework.")
#	endif()
#endif()

find_program(Python3_EXECUTABLE python3 DOC "python3 executable")

if (NOT Python3_EXECUTABLE)
	message(FATAL_ERROR "Could NOT find python3 executable.")
endif()

find_program(Python3_CONFIG_EXECUTABLE python3-config DOC "python3-config executable")

if (NOT Python3_CONFIG_EXECUTABLE)
	message(FATAL_ERROR "Could NOT find python3 config utility.")
endif()

execute_process(
	COMMAND ${Python3_EXECUTABLE} --version
	OUTPUT_VARIABLE Python3_VERSION
	ERROR_VARIABLE Python3_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)

string(SUBSTRING "${Python3_VERSION}" 7 -1 Python3_VERSION)

execute_process(
	COMMAND ${Python3_CONFIG_EXECUTABLE} --includes
	OUTPUT_VARIABLE PYTHON_INCLUDE_DIRS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "-I" "" PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")
string(REPLACE " " ";" PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")

execute_process(
	COMMAND ${Python3_CONFIG_EXECUTABLE} --ldflags
	OUTPUT_VARIABLE PYTHON_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

#message(FATAL_ERROR ${PYTHON_LIBRARIES})

#execute_process(
#	COMMAND ${Python3_CONFIG_EXECUTABLE} --prefix
#	OUTPUT_VARIABLE python3_libdir
#	OUTPUT_STRIP_TRAILING_WHITESPACE)

#if (APPLE)
#	string(REPLACE "-framework CoreFoundation" "" PYTHON_LIBRARIES "${PYTHON_LIBRARIES}")
#endif()
#string(REPLACE "-ldl" "" PYTHON_LIBRARIES "${PYTHON_LIBRARIES}")
#string(REPLACE "-l" "" PYTHON_LIBRARIES "${PYTHON_LIBRARIES}")
#string(REPLACE " " "" PYTHON_LIBRARIES "${PYTHON_LIBRARIES}")

#find_library(PYTHON_LIBRARIES ${PYTHON_LIBRARIES} PATHS ${python3_libdir} PATH_SUFFIXES lib NO_DEFAULT_PATH)
#set(PYTHON_LIBRARIES "${python3_libdir}/lib/lib${PYTHON_LIBRARIES}.a")

#if (APPLE)
#	set(PYTHON_LIBRARIES "${PYTHON_LIBRARIES}" "${COREFOUNDATION_FRAMEWORK}" "${DL_LIBRARIES}")
#endif()

#foreach(lib ${PYTHON_LIBRARIES})
#	message(STATUS ${lib})
#endforeach()
#foreach(dir ${PYTHON_INCLUDE_DIRS})
#	message(STATUS ${dir})
#endforeach()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Python3 
	FOUND_VAR Python3_FOUND
	VERSION_VAR Python3_VERSION
	REQUIRED_VARS PYTHON_INCLUDE_DIRS PYTHON_LIBRARIES)

mark_as_advanced(Python3_FOUND)
mark_as_advanced(PYTHON_LIBRARIES)
mark_as_advanced(PYTHON_INCLUDE_DIRS)