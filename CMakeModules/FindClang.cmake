if (NOT LLVM_FOUND)
	message(FATAL_ERROR "LLVM is required for Clang.")
endif ()

# Find Clang
#
# It defines the following variables
# Clang_FOUND         - True if Clang was found.
# Clang_INCLUDE_DIRS  - where to find Clang include files
# Clang_LIBRARIES     - list of clang libs
# Clang_VERSION_MAJOR - The major version number
# Clang_VERSION_MINOR - The minor version number
# Clang_VERSION       - Is the same as ${Clang_VERSION_MAJOR}.${Clang_VERSION_MINOR}

macro(find_and_add_lib library_name)
	find_library(CLANG_${library_name}_LIB ${library_name} ${LLVM_LIBRARY_DIRS} ${CLANG_LIBRARY_DIRS})
	mark_as_advanced(CLANG_${library_name}_LIB)
	if (CLANG_${library_name}_LIB)
		set(Clang_LIBRARIES ${Clang_LIBRARIES} ${CLANG_${library_name}_LIB})
	else ()
		message(WARNING "Clang: Could not find ${library_name}.")
	endif ()
endmacro(find_and_add_lib)

find_and_add_lib(clangTooling)
find_and_add_lib(clangFrontendTool)
find_and_add_lib(clangFrontend)
find_and_add_lib(clangDriver)
find_and_add_lib(clangSerialization)
find_and_add_lib(clangCodeGen)
find_and_add_lib(clangParse)
find_and_add_lib(clangSema)
find_and_add_lib(clangStaticAnalyzerFrontend)
find_and_add_lib(clangStaticAnalyzerCheckers)
find_and_add_lib(clangStaticAnalyzerCore)
find_and_add_lib(clangAnalysis)
find_and_add_lib(clangARCMigrate)
find_and_add_lib(clangRewrite)
find_and_add_lib(clangRewriteFrontend)
find_and_add_lib(clangEdit)
find_and_add_lib(clangAST)
find_and_add_lib(clangLex)
find_and_add_lib(clangBasic)
find_and_add_lib(clang)

find_path(Clang_INCLUDE_DIRS clang/Basic/Version.h HINTS ${LLVM_INCLUDE_DIRS})
file(READ ${Clang_INCLUDE_DIRS}/clang/Basic/Version.inc rawVersion)
string(REGEX REPLACE "^.*CLANG_VERSION_MAJOR ([0-9]+).*$" "\\1" Clang_VERSION_MAJOR "${rawVersion}")
string(REGEX REPLACE "^.*CLANG_VERSION_MINOR ([0-9]+).*$" "\\1" Clang_VERSION_MINOR "${rawVersion}")
set(Clang_VERSION "${Clang_VERSION_MAJOR}.${Clang_VERSION_MINOR}")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Clang 
	FOUND_VAR Clang_FOUND
	REQUIRED_VARS Clang_LIBRARIES Clang_INCLUDE_DIRS
	VERSION_VAR Clang_VERSION)

mark_as_advanced(Clang_FOUND)
mark_as_advanced(Clang_INCLUDE_DIRS)
mark_as_advanced(Clang_LIBRARIES)
mark_as_advanced(Clang_VERSION_MAJOR)
mark_as_advanced(Clang_VERSION_MINOR)
mark_as_advanced(Clang_VERSION)
