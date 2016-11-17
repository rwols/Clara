configure_file(Configuration.hpp.in ${CMAKE_BINARY_DIR}/Configuration.hpp)
target_include_directories(Clara PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(Clara PUBLIC ${CMAKE_BINARY_DIR})
