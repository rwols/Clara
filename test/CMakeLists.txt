# NOTE: Executables in this directory are for testing
# purposes for the stuff in ../lib.

# add_executable(hello hello.cpp)
# add_executable(test2 test2.cpp)

# add_custom_target(TestFile ALL 
# 	COMMAND ${CMAKE_COMMAND} -E copy test.py ${CMAKE_BINARY_DIR}/lib/ 
# 	DEPENDS ${CMAKE_PROJECT_NAME} 
# 	BYPRODUCTS ${CMAKE_BINARY_DIR}/lib/test.py
# 	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# add_custom_target(RunTestFile ALL
# 	COMMAND python3 test.py
# 	DEPENDS TestFile
# 	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
