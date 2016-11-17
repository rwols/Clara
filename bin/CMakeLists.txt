add_executable(refactor main.cpp)
add_executable(boosttest boosttest.cpp)
add_executable(clangapitest clangapitest.cpp)

# set_target_properties(clangapitest PROPERTIES COMPILE_FLAGS "-fPIC -fvisibility-inlines-hidden -Wall -fcolor-diagnostics -g -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

# target_include_directories(refactor PUBLIC Clara)
# target_include_directories(boosttest PUBLIC Clara)
# target_include_directories(clangapitest PUBLIC Clara)

target_link_libraries(refactor PUBLIC Clara)
target_link_libraries(boosttest PUBLIC Clara)
target_link_libraries(clangapitest PUBLIC Clara)

# target_link_libraries(clangapitest PUBLIC ${Clang_LIBRARIES} ${LLVM_LIBRARIES})
