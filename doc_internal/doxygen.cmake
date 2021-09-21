find_program(DOXYGEN Doxygen)
message("DOXYGEN=${DOXYGEN}")

execute_process(
  COMMAND ${DOXYGEN} Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doxygen
  )
