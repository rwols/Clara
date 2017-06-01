
if (CMAKE_VERSION VERSION_LESS 3.6)
  set(GIT_EXECUTABLE "git")
else()
  find_package(git REQUIRED)
endif()

function(get_git_branch in_repository out_branch)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY "${in_repository}"
      OUTPUT_VARIABLE branch
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(branch STREQUAL "")
        message(FATAL_ERROR "Could not determine the git branch of ${in_repository}")
    else()
        set(${out_branch} ${branch} PARENT_SCOPE)
    endif()
endfunction(get_git_branch)

function(get_git_commit in_repository out_commit)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      WORKING_DIRECTORY "${in_repository}"
      OUTPUT_VARIABLE commit
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(commit STREQUAL "")
        message(FATAL_ERROR "Could not determine the git commit of ${in_repository}")
    else()
        set(${out_commit} ${commit} PARENT_SCOPE)
    endif()
endfunction(get_git_commit)
