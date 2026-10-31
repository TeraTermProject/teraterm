find_program(DOXYGEN NAMES doxygen)
message("DOXYGEN=${DOXYGEN}")

if(NOT DOXYGEN)
  message(FATAL_ERROR "doxygen not found")
endif()

execute_process(
  COMMAND ${DOXYGEN} Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doxygen
  RESULT_VARIABLE DOXYGEN_RESULT
)

if(NOT DOXYGEN_RESULT EQUAL 0)
  message(FATAL_ERROR "doxygen failed (${DOXYGEN_RESULT})")
endif()
