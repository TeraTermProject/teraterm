#
# cmake -DOUTPUT=outputbase -P sha512sum.cmake <file1> <file2> ...

if(NOT DEFINED OUTPUT)
  set(OUTPUT "sha512.sha512sum")
endif()

function(CALC_SHA512 FILE_NAME OUTPUT_FILE)
  file(SHA512 "${FILE_NAME}" HASH)
  file(APPEND "${OUTPUT_FILE}" "${HASH} ${FILE_NAME}\n")
endfunction()


foreach(i RANGE 4 ${CMAKE_ARGC})
  set(FILE_NAME "${CMAKE_ARGV${i}}")
  if(EXISTS "${FILE_NAME}")
    CALC_SHA512("${FILE_NAME}" ${OUTPUT})
  endif()
endforeach()
