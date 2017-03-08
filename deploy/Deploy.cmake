# inputs must be VERSION and ZIPFILE
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

file(READ "${CMAKE_CURRENT_LIST_DIR}/messages/${VERSION}.txt" body)

find_program(GITHUBRELEASE 
    NAMES 
        githubrelease 
    HINTS
        /usr/local/bin
    DOC 
        "githubrelease executable"
    )

if(GITHUBRELEASE)
    message(STATUS "Found githubrelease: ${GITHUBRELEASE}")
else()
    message(FATAL_ERROR "Could not find githubrelease executable.")
endif()

message(STATUS "Creating release ${VERSION} on GitHub")
execute_process(
    COMMAND
        "${GITHUBRELEASE}" release rwols/Clara create ${VERSION}
    )

message(STATUS "Inserting body for release ${VERSION} on GitHub")
execute_process(
    COMMAND
        "${GITHUBRELEASE}" release rwols/Clara edit ${VERSION} --body "${body}"
    RESULT_VARIABLE
        exit_status)

if(NOT exit_status EQUAL 0)
    message(FATAL_ERROR "Failed to set the text body of release ${VERSION}")
endif()

get_filename_component(zipname "${ZIPFILE}" NAME)
message(STATUS "Deleting ${zipname} of release ${VERSION} on GitHub (if present)")
execute_process(
    COMMAND 
        "${GITHUBRELEASE}" asset rwols/Clara delete ${VERSION} "${zipname}"
    )

message(STATUS "Uploading ${ZIPFILE} for release ${VERSION} to GitHub")
execute_process(
    COMMAND 
        "${GITHUBRELEASE}" asset rwols/Clara upload ${VERSION} "${ZIPFILE}"
    RESULT_VARIABLE
        exit_status)

if(NOT exit_status EQUAL 0)
    message(FATAL_ERROR "Failed to upload ${ZIPFILE}")
endif()
