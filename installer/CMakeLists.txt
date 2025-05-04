
add_custom_target(
  teraterm_ini ALL
  DEPENDS ${BINARY_DIR}/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  )

set_target_properties(teraterm_ini PROPERTIES FOLDER installer)

add_custom_command(
  OUTPUT ${BINARY_DIR}/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI > "${BINARY_DIR}/TERATERM.INI"
  )

install(
  FILES ${BINARY_DIR}/TERATERM.INI
  DESTINATION .
  )

add_custom_target(
  special_font ALL
  DEPENDS ${BINARY_DIR}/TSPECIAL1.TTF
)

add_custom_command(
  OUTPUT ${BINARY_DIR}/TSPECIAL1.TTF
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/release/TSPECIAL1.TTF ${BINARY_DIR}
)

set_target_properties(special_font PROPERTIES FOLDER installer)

install(
  DIRECTORY release/
  DESTINATION .
  PATTERN *~ EXCLUDE
  PATTERN TERATERM.INI EXCLUDE
  PATTERN lang* EXCLUDE
  )

install(
  FILES release/IBMKEYB.CNF
  DESTINATION .
  RENAME KEYBOARD.CNF
  )

install(
  FILES ../ttpmenu/readme.txt
  DESTINATION .
  RENAME ttmenu_readme-j.txt
)

# Inno Setup
#  Create setup.exe
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_program(
    ISCC ISCC.exe
    HINTS "${CMAKE_SOURCE_DIR}/buildtools/innosetup6"
    HINTS "C:/Program Files (x86)/Inno Setup 6/"
    HINTS "C:/Program Files (x86)/Inno Setup 5/"
    HINTS "C:/Program Files/Inno Setup 6/"
    HINTS "C:/Program Files/Inno Setup 5/"
    HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
    HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 5"
  )
  message("ISCC=${ISCC}")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  add_custom_target(
    inno_setup
    COMMAND "${CMAKE_COMMAND}" -E echo "inno setup do not run"
  )
elseif("${ISCC}" STREQUAL "ISCC-NOTFOUND")
  add_custom_target(
    inno_setup
    COMMAND "${CMAKE_COMMAND}" -E echo "inno setup not found"
  )
elseif(NOT TARGET ttxssh)
  add_custom_target(
    inno_setup
    COMMAND "${CMAKE_COMMAND}" -E echo "ttxssh was not build"
  )
else()
  configure_file(
    teraterm_cmake.iss.in teraterm_cmake.iss
    NEWLINE_STYLE CRLF
    )

  if (RELEASE)
    set(ISCC_OPTION "-DSETUP_RELEASE=1")
  endif()
  if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "arm64")
    list(APPEND ISCC_OPTION "-DM_ARM64=1")
  elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    list(APPEND ISCC_OPTION "-DM_X64=1")
  endif()
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${SETUP_EXE}.exe
    COMMAND ${ISCC} ${ISCC_OPTION} teraterm_cmake.iss
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha256sum
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha512sum
    WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
  )
  add_custom_target(
    inno_setup
    DEPENDS cygtool
    DEPENDS ${CMAKE_BINARY_DIR}/${SETUP_EXE}.exe
    )
endif()
set_target_properties(inno_setup PROPERTIES FOLDER installer)

# zip
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/${SETUP_ZIP}
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/TTXFixedWinSize.dll ${CMAKE_INSTALL_PREFIX}/_TTXFixedWinSize.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/TTXOutputBuffering.dll ${CMAKE_INSTALL_PREFIX}/_TTXOutputBuffering.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/TTXResizeWin.dll ${CMAKE_INSTALL_PREFIX}/_TTXResizeWin.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/TTXShowCommandLine.dll ${CMAKE_INSTALL_PREFIX}/_TTXShowCommandLine.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/TTXtest.dll ${CMAKE_INSTALL_PREFIX}/_TTXtest.dll
  COMMAND "${CMAKE_COMMAND}" -E tar cvf ${SETUP_ZIP} --format=zip ${CMAKE_INSTALL_PREFIX}
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/_TTXFixedWinSize.dll ${CMAKE_INSTALL_PREFIX}/TTXFixedWinSize.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/_TTXOutputBuffering.dll ${CMAKE_INSTALL_PREFIX}/TTXOutputBuffering.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/_TTXResizeWin.dll ${CMAKE_INSTALL_PREFIX}/TTXResizeWin.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/_TTXShowCommandLine.dll ${CMAKE_INSTALL_PREFIX}/TTXShowCommandLine.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_INSTALL_PREFIX}/_TTXtest.dll ${CMAKE_INSTALL_PREFIX}/TTXtest.dll
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha256sum
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha512sum
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

add_custom_target(
  zip
  DEPENDS ${CMAKE_BINARY_DIR}/${SETUP_ZIP}
)
set_target_properties(zip PROPERTIES FOLDER installer)

add_subdirectory(release/lang)
set_target_properties(lang PROPERTIES FOLDER installer)
add_subdirectory(release/lang_utf16le)
set_target_properties(lang_utf16le PROPERTIES FOLDER installer)

# sha256sum
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha256sum
  COMMAND ${CMAKE_COMMAND} -DOUTPUT=${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha256sum -P ${CMAKE_CURRENT_LIST_DIR}/sha256sum.cmake ${SETUP_EXE}.exe ${SETUP_ZIP}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_custom_target(
  sha256sum
  DEPENDS ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha256sum
)
set_target_properties(sha256sum PROPERTIES FOLDER installer)

# sha512sum
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha512sum
  COMMAND ${CMAKE_COMMAND} -DOUTPUT=${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha512sum -P ${CMAKE_CURRENT_LIST_DIR}/sha512sum.cmake ${SETUP_EXE}.exe ${SETUP_ZIP}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_custom_target(
  sha512sum
  DEPENDS ${CMAKE_BINARY_DIR}/${SETUP_EXE}.sha512sum
)
set_target_properties(sha512sum PROPERTIES FOLDER installer)

# calc_sha
add_custom_target(
  calc_hash
  DEPENDS sha256sum
  DEPENDS sha512sum
)
set_target_properties(calc_hash PROPERTIES FOLDER installer)
