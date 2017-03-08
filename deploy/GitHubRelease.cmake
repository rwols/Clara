include(CMakeParseArguments)

find_program(GITHUBRELEASE 
  NAMES githubrelease 
  HINTS /usr/local/bin
  DOC "githubrelease executable"
  )

if(GITHUBRELEASE)
  message(STATUS "Found githubrelease: ${GITHUBRELEASE}")
else()
  message(FATAL_ERROR "Could not find githubrelease executable.")
endif()


function(githubrelease repository)
  set(functionArguments TAG BODY)
  set(multiValueArgs ASSETS)
  cmake_parse_arguments(args "" "${functionArguments}" "${multiValueArgs}" ${ARGN})
  message(STATUS "Creating release ${args_TAG} for repository ${repository} on GitHub")
  execute_process(
    COMMAND
      "${GITHUBRELEASE}" release ${repository} create ${args_TAG}
    )

  message(STATUS "Inserting body for release ${args_TAG} on GitHub")
  execute_process(
    COMMAND
      "${GITHUBRELEASE}" release ${repository} edit ${args_TAG} --body "${body}"
    RESULT_VARIABLE
      exit_status)

  if(NOT exit_status EQUAL 0)
    message(FATAL_ERROR "Failed to set the text body of release ${args_TAG}")
  endif()

  foreach(asset ${args_ASSETS})
    get_filename_component(assetname "${asset}" NAME)
    message(STATUS "Deleting ${assetname} of release ${args_TAG} on GitHub (if present)")
    execute_process(
      COMMAND 
        "${GITHUBRELEASE}" asset ${repository} delete ${args_TAG} "${assetname}"
      )

    message(STATUS "Uploading ${asset} for release ${args_TAG} to GitHub")
    execute_process(
      COMMAND 
        "${GITHUBRELEASE}" asset ${repository} upload ${args_TAG} "${asset}"
      RESULT_VARIABLE
        exit_status)

    if(NOT exit_status EQUAL 0)
      message(FATAL_ERROR "Failed to upload ${asset}")
    endif()
  endforeach(asset)
endfunction(githubrelease)

