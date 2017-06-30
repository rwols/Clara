# pybind11 demands exceptions and RTTI

set(LLVM_LINK_COMPONENTS
    Core 
    Support
    )

set(CLANG_LIBS
    clangFrontend
    clangTooling
    )

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# We need to have an exact version match for Sublime Text otherwise Python will
# complain about threads not being initialized.
set(PYBIND11_PYTHON_VERSION 3.3)
set(PYTHON_EXECUTABLE python${PYBIND11_PYTHON_VERSION})

add_subdirectory(pybind11)

set(source_files
    CodeCompleter.cpp
    CompilationDatabaseWatcher.cpp
    PythonBindings.cpp
    )

pybind11_add_module(Clara ${source_files})

foreach(comp ${LLVM_LINK_COMPONENTS})
    list(APPEND comps LLVM${comp})
endforeach(comp)

target_link_libraries(Clara LINK_PRIVATE ${CLANG_LIBS} ${comps})

file(GLOB_RECURSE builtin_headers ../../../lib/Headers/*.h)
install(FILES ${builtin_headers}
    DESTINATION "${claraInstallFolder}/include")

install(TARGETS Clara 
    DESTINATION "${claraInstallFolder}")

add_custom_target(ClaraInstall 
    ${CMAKE_COMMAND} 
    -DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake) 

add_dependencies(ClaraInstall Clara ClaraPlugin)
