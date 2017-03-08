# inputs must be VERSION, SUBLIME_PLATFORM_EXT and ZIPFILE
if(DEFINED VERSION)
    message(STATUS "tag: ${VERSION}")
else()
    message(FATAL_ERROR "VERSION variable is required. Pass it via -D")
endif()
if(DEFINED ZIPFILE)
    message(STATUS "zipfile: ${ZIPFILE}")
else()
    message(FATAL_ERROR "ZIPFILE variable is required. Pass it via -D")
endif()
if(DEFINED SUBLIME_PLATFORM_EXT)
    message(STATUS "platform: ${SUBLIME_PLATFORM_EXT}")
else()
    MESSAGE(FATAL_ERROR "SUBLIME_PLATFORM_EXT variable is required. Pass it via -D")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/messages/${VERSION}.txt" body)

find_package(PythonInterp REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include(GitHubRelease)
include(GitUtilities)

get_git_branch("${CMAKE_CURRENT_LIST_DIR}/.." GIT_BRANCH)
get_git_commit("${CMAKE_CURRENT_LIST_DIR}/.." GIT_COMMIT)

message(STATUS "git branch: ${GIT_BRANCH}")
message(STATUS "git commit: ${GIT_COMMIT}")

if(NOT GIT_BRANCH STREQUAL master)
    message(FATAL_ERROR "You are on branch ${GIT_BRANCH}; you can only deploy from the master branch.")
endif()

githubrelease(rwols/Clara
    TAG ${VERSION}
    BODY ${body}
    ASSETS "${ZIPFILE}")

message(STATUS "Updating repository.json")
execute_process(
    COMMAND
        "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/update-repository.py" --tag ${VERSION} --platform ${SUBLIME_PLATFORM_EXT} --zipfile "${ZIPFILE}"
    RESULT_VARIABLE
        exit_status
    )
if(NOT exit_status EQUAL 0)
    message(FATAL_ERROR "Failed to update repository.json")
endif()
