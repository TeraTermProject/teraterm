
file(REMOVE_RECURSE global)

execute_process(
  COMMAND ${CMAKE_COMMAND} -P gtags_update.cmake
  WORKING_DIRECTORY ".."
  )

find_program(HTAGS htags)
message("HTAGS=${HTAGS}")

execute_process(
  COMMAND ${HTAGS} -ans --tabs 4 -F
  WORKING_DIRECTORY ".."
  )

file(MAKE_DIRECTORY global)
file(RENAME ../HTML global/HTML)
