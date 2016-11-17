find_package(Python3 REQUIRED)

add_executable(refactor main.cpp)
add_executable(boosttest boosttest.cpp)
add_executable(clangapitest clangapitest.cpp)

include(TargetSetRTTI)
target_set_rtti(clangapitest ON)

set_target_properties(clangapitest PROPERTIES COMPILE_FLAGS "-fPIC -fvisibility-inlines-hidden -Wall -fcolor-diagnostics -g -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

target_include_directories(refactor PUBLIC cpp)
target_include_directories(boosttest PUBLIC ${PYTHON_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
# target_include_directories(clangapitest PUB)

target_link_libraries(refactor PUBLIC cpp)
target_link_libraries(boosttest PUBLIC ${Boost_LIBRARIES})
target_link_libraries(boosttest PUBLIC ${PYTHON_LIBRARIES})

target_link_libraries(clangapitest PUBLIC ${Clang_LIBRARIES} ${LLVM_LIBRARIES} ${LLVM_SYSTEM_LIBRARIES})
