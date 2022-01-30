if (MSVC OR MINGW)
  find_program(
    HHC hhc.exe
    HINTS "C:/Program Files (x86)/HTML Help Workshop"
    HINTS "C:/Program Files/HTML Help Workshop"
    HINTS "$ENV{ProgramFiles}/HTML Help Workshop"
    HINTS "$ENV{ProgramFiles\(x86\)}/HTML Help Workshop"
    )
  find_program(
    CHMCMD chmcmd
    HINTS ${CMAKE_CURRENT_LIST_DIR}/../buildtools/chmcmd/
    )
else()
  set(HHC "HHC-NOTFOUND")
  find_program(
    CHMCMD chmcmd
    )
endif()
message("hhc=${HHC}")

if (NOT("${PERL}" STREQUAL "PERL-NOTFOUND")
    AND (NOT(("${HHC}" STREQUAL "HHC-NOTFOUND") AND "${CHMCMD}" STREQUAL "CHMCMD-NOTFOUND")))
  file(
    GLOB_RECURSE
    SRC_EN
    LIST_DIRECTORIES false
    en/*.html en/*.md en/*.png en/*.hhc en/*.hhp
    )

  file(
    GLOB_RECURSE
    SRC_JP
    LIST_DIRECTORIES false
    ja/*.html ja/*.md ja/*.png ja/*.hhc ja/*.hhp
    )

  add_custom_target(
    chm ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    SOURCES ${SRC_EN}
    SOURCES ${SRC_JP}
    SOURCES htmlhelp_index_make.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_EN})
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_JP})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    COMMAND ${CMAKE_COMMAND} -P makechm.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/teraterm.chm"
    )

  set_target_properties(
    chm
    PROPERTIES FOLDER doc)

  install(
    FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    DESTINATION .
    )

else()

  # chmが存在したらコピーする
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm)
    install(
      FILES ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
      DESTINATION .
      )
  endif()
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm)
    install(
      FILES ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
      DESTINATION .
      )
  endif()

endif()
