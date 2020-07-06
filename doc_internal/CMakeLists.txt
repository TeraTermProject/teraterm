find_package(Doxygen)
message("doxygen=${DOXYGEN_EXECUTABLE}")

if(NOT DOXYGEN_FOUND)
  message(warning "doxygen not found")
else()
  add_custom_target(
    doxygen
    ${DOXYGEN_EXECUTABLE} Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doxygen
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
    COMMAND ${GTAGS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/global
    COMMAND ${HTAGS} -ans --tabs 4 -F -d ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/global
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

  set_target_properties(
    global
    PROPERTIES FOLDER doc_internal)
endif()
