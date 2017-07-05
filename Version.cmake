EXECUTE_PROCESS(
    COMMAND git describe --dirty 
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE git_version
    RESULT_VARIABLE git_result
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

message(STATUS "get version[${git_result}]: ${git_version}")

STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\1.\\2.\\3" VERSION "${git_version}" )
STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\1" VERSION_MAJOR "${git_version}")
STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\2" VERSION_MINOR "${git_version}")
STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\3" VERSION_PATCH "${git_version}")

