function(get_git_version DATE COMMIT_HASH)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
            COMMAND git log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH_STRING
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${COMMIT_HASH} ${GIT_COMMIT_HASH_STRING} PARENT_SCOPE)

    execute_process(
            COMMAND git log -1 --format=%ad --date=short
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DATE_STRING
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${DATE} ${GIT_DATE_STRING} PARENT_SCOPE)
  else(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(${DATE} "" PARENT_SCOPE)
    set(${COMMIT_HASH} "" PARENT_SCOPE)
  endif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
endfunction()

function(get_file_version DATE COMMIT_HASH)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LINES)
  string(REPLACE "\n" ";" LINES "${LINES}")
  list(GET LINES 1 FILE_DATE_STRING)
  list(GET LINES 3 FILE_COMMIT_HASH_STRING)
  set(${DATE} ${FILE_DATE_STRING} PARENT_SCOPE)
  set(${COMMIT_HASH} ${FILE_COMMIT_HASH_STRING} PARENT_SCOPE)
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION") 
  get_file_version(GIT_DATE GIT_COMMIT_SHA)
else()
  get_git_version(GIT_DATE GIT_COMMIT_SHA)
endif()
