find_package(Doxygen)
message("doxygen=${DOXYGEN_EXECUTABLE}")

if(NOT DOXYGEN_FOUND)
  message(warning "doxygen not found")
else()
  add_custom_target(
    doxygen
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/doxygen.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

  set_target_properties(
    doxygen
    PROPERTIES FOLDER doc_internal)
endif()


find_program(GTAGS gtags)
find_program(HTAGS htags)
message("GTAGS=${GTAGS}")
message("HTAGS=${HTAGS}")

if(("${GTAGS}" STREQUAL "GTAGS-NOTFOUND") OR ("${HTAGS}" STREQUAL "HTAGS-NOTFOUND"))
  message(warning "global not found")
else()
  add_custom_target(
    global
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/global.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

  set_target_properties(
    global
    PROPERTIES FOLDER doc_internal)
endif()
