#!/usr/bin/env cmake -P
find_program(FIND find
  HINTS C:/cygwin64/bin
  HINTS C:/cygwin/bin
  HINTS C:/msys64/usr/bin
  )
find_program(GREP grep
  HINTS C:/cygwin64/bin
  HINTS C:/cygwin/bin
  HINTS C:/msys64/usr/bin
  )
find_program(GTAGS gtags)
message("FIND=${FIND}")
message("GREP=${GREP}")
message("GTAGS=${GTAGS}")

execute_process(
  COMMAND ${FIND} teraterm TTProxy TTX* ttssh2 -type f -name "*.c" -o -name "*.cpp" -o -name "*.h"
  COMMAND ${GREP} -v Release
  COMMAND ${GREP} -v Debug
  COMMAND ${GREP} -v .vs
  COMMAND ${GREP} -v build
  OUTPUT_FILE gtags.files
  WORKING_DIRECTORY "."
  )
execute_process(
  COMMAND ${GTAGS} -f gtags.files
  )
# see doc_internal/readme.md

