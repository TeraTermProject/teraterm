
install(
  DIRECTORY release/
  DESTINATION .
  PATTERN *~ EXCLUDE
  PATTERN TERATERM.INI EXCLUDE
  PATTERN lang_utf8 EXCLUDE
  )

install(
  FILES release/EDITOR.CNF
  DESTINATION .
  RENAME KEYBOARD.CNF
  )

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  find_program(
    PERL perl
    )
else()
  # CYGWIN / Active Perl
  find_program(
    PERL perl.exe
    HINTS c:/Perl64/bin
    HINTS c:/Perl/bin
    HINTS c:/cygwin/usr/bin
    HINTS c:/cygwin64/usr/bin
    )
endif()

add_custom_target(
  teraterm_ini ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl
  COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/setini.pl ${CMAKE_CURRENT_SOURCE_DIR}/release/TERATERM.INI > "${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI"
  )

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/TERATERM.INI
  DESTINATION .
  )

set(RELEASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release)

add_custom_target(
  lang_conv ALL
  DEPENDS
    ${RELEASE_DIR}/lang_utf16le/Default.lng
    ${RELEASE_DIR}/lang_utf16le/English.lng
    ${RELEASE_DIR}/lang_utf16le/French.lng
    ${RELEASE_DIR}/lang_utf16le/German.lng
    ${RELEASE_DIR}/lang_utf16le/Japanese.lng
    ${RELEASE_DIR}/lang_utf16le/Korean.lng
    ${RELEASE_DIR}/lang_utf16le/Russian.lng
    "${RELEASE_DIR}/lang_utf16le/Simplified Chinese.lng"
    "${RELEASE_DIR}/lang_utf16le/Traditional Chinese.lng"
    ${RELEASE_DIR}/lang/Default.lng
    ${RELEASE_DIR}/lang/English.lng
    ${RELEASE_DIR}/lang/French.lng
    ${RELEASE_DIR}/lang/German.lng
    ${RELEASE_DIR}/lang/Japanese.lng
    ${RELEASE_DIR}/lang/Korean.lng
    ${RELEASE_DIR}/lang/Russian.lng
    "${RELEASE_DIR}/lang/Simplified Chinese.lng"
    "${RELEASE_DIR}/lang/Traditional Chinese.lng"
  SOURCES
    ${RELEASE_DIR}/lang_utf8/Default.lng
    ${RELEASE_DIR}/lang_utf8/English.lng
    ${RELEASE_DIR}/lang_utf8/French.lng
    ${RELEASE_DIR}/lang_utf8/German.lng
    ${RELEASE_DIR}/lang_utf8/Japanese.lng
    ${RELEASE_DIR}/lang_utf8/Korean.lng
    ${RELEASE_DIR}/lang_utf8/Russian.lng
    "${RELEASE_DIR}/lang_utf8/Simplified Chinese.lng"
    "${RELEASE_DIR}/lang_utf8/Traditional Chinese.lng"
  )

add_custom_command(
  OUTPUT
    ${RELEASE_DIR}/lang_utf16le/Default.lng
    ${RELEASE_DIR}/lang_utf16le/English.lng
    ${RELEASE_DIR}/lang_utf16le/French.lng
    ${RELEASE_DIR}/lang_utf16le/German.lng
    ${RELEASE_DIR}/lang_utf16le/Japanese.lng
    ${RELEASE_DIR}/lang_utf16le/Korean.lng
    ${RELEASE_DIR}/lang_utf16le/Russian.lng
    "${RELEASE_DIR}/lang_utf16le/Simplified Chinese.lng"
    "${RELEASE_DIR}/lang_utf16le/Traditional Chinese.lng"
    ${RELEASE_DIR}/lang/Default.lng
    ${RELEASE_DIR}/lang/English.lng
    ${RELEASE_DIR}/lang/French.lng
    ${RELEASE_DIR}/lang/German.lng
    ${RELEASE_DIR}/lang/Japanese.lng
    ${RELEASE_DIR}/lang/Korean.lng
    ${RELEASE_DIR}/lang/Russian.lng
    "${RELEASE_DIR}/lang/Simplified Chinese.lng"
    "${RELEASE_DIR}/lang/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_DIR}/lang_utf16le
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/Default.lng ${RELEASE_DIR}/lang_utf16le/Default.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/English.lng ${RELEASE_DIR}/lang_utf16le/English.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/French.lng ${RELEASE_DIR}/lang_utf16le/French.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/German.lng ${RELEASE_DIR}/lang_utf16le/German.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/Japanese.lng ${RELEASE_DIR}/lang_utf16le/Japanese.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/Korean.lng ${RELEASE_DIR}/lang_utf16le/Korean.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le ${RELEASE_DIR}/lang_utf8/Russian.lng ${RELEASE_DIR}/lang_utf16le/Russian.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le "${RELEASE_DIR}/lang_utf8/Simplified Chinese.lng" "${RELEASE_DIR}/lang_utf16le/Simplified Chinese.lng"
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl utf16le "${RELEASE_DIR}/lang_utf8/Traditional Chinese.lng" "${RELEASE_DIR}/lang_utf16le/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_DIR}/lang
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp1252 ${RELEASE_DIR}/lang_utf8/Default.lng ${RELEASE_DIR}/lang/Default.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp1252 ${RELEASE_DIR}/lang_utf8/English.lng ${RELEASE_DIR}/lang/English.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp1252 ${RELEASE_DIR}/lang_utf8/French.lng ${RELEASE_DIR}/lang/French.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp1252 ${RELEASE_DIR}/lang_utf8/German.lng ${RELEASE_DIR}/lang/German.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp932 ${RELEASE_DIR}/lang_utf8/Japanese.lng ${RELEASE_DIR}/lang/Japanese.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp949 ${RELEASE_DIR}/lang_utf8/Korean.lng ${RELEASE_DIR}/lang/Korean.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl windows-1251 ${RELEASE_DIR}/lang_utf8/Russian.lng ${RELEASE_DIR}/lang/Russian.lng
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp936 "${RELEASE_DIR}/lang_utf8/Simplified Chinese.lng" "${RELEASE_DIR}/lang/Simplified Chinese.lng"
    COMMAND ${PERL} ${CMAKE_CURRENT_SOURCE_DIR}/utf8_to.pl cp950 "${RELEASE_DIR}/lang_utf8/Traditional Chinese.lng" "${RELEASE_DIR}/lang/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang ${CMAKE_BINARY_DIR}/$<CONFIG>/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang_utf16le ${CMAKE_BINARY_DIR}/$<CONFIG>/lang_utf16le
  )


# copy .lng files
if(MSVC AND ("${MSVC_VERSION}" GREATER_EQUAL "1910"))
  # Visual Studio 2017以上の時
  add_custom_target(
    lang_copy ALL
    DEPENDS
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Traditional Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Traditional Chinese.lng"
    SOURCES
      ${RELEASE_DIR}/lang_utf16le/Default.lng
      ${RELEASE_DIR}/lang_utf16le/English.lng
      ${RELEASE_DIR}/lang_utf16le/French.lng
      ${RELEASE_DIR}/lang_utf16le/German.lng
      ${RELEASE_DIR}/lang_utf16le/Japanese.lng
      ${RELEASE_DIR}/lang_utf16le/Korean.lng
      ${RELEASE_DIR}/lang_utf16le/Russian.lng
      "${RELEASE_DIR}/lang_utf16le/Simplified Chinese.lng"
      "${RELEASE_DIR}/lang_utf16le/Traditional Chinese.lng"
      ${RELEASE_DIR}/lang/Default.lng
      ${RELEASE_DIR}/lang/English.lng
      ${RELEASE_DIR}/lang/French.lng
      ${RELEASE_DIR}/lang/German.lng
      ${RELEASE_DIR}/lang/Japanese.lng
      ${RELEASE_DIR}/lang/Korean.lng
      ${RELEASE_DIR}/lang/Russian.lng
      "${RELEASE_DIR}/lang/Simplified Chinese.lng"
      "${RELEASE_DIR}/lang/Traditional Chinese.lng"
    )
  add_custom_command(
    OUTPUT
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang/Traditional Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)/lang_utf16le/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang ${CMAKE_BINARY_DIR}/$<CONFIG>/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang_utf16le ${CMAKE_BINARY_DIR}/$<CONFIG>/lang_utf16le
    )
elseif(MINGW)
  # MinGW
  add_custom_target(
    lang_copy ALL
    DEPENDS
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Traditional Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Traditional Chinese.lng"
    SOURCES
      ${RELEASE_DIR}/lang_utf16le/Default.lng
      ${RELEASE_DIR}/lang_utf16le/English.lng
      ${RELEASE_DIR}/lang_utf16le/French.lng
      ${RELEASE_DIR}/lang_utf16le/German.lng
      ${RELEASE_DIR}/lang_utf16le/Japanese.lng
      ${RELEASE_DIR}/lang_utf16le/Korean.lng
      ${RELEASE_DIR}/lang_utf16le/Russian.lng
      "${RELEASE_DIR}/lang_utf16le/Simplified Chinese.lng"
      "${RELEASE_DIR}/lang_utf16le/Traditional Chinese.lng"
      ${RELEASE_DIR}/lang/Default.lng
      ${RELEASE_DIR}/lang/English.lng
      ${RELEASE_DIR}/lang/French.lng
      ${RELEASE_DIR}/lang/German.lng
      ${RELEASE_DIR}/lang/Japanese.lng
      ${RELEASE_DIR}/lang/Korean.lng
      ${RELEASE_DIR}/lang/Russian.lng
      "${RELEASE_DIR}/lang/Simplified Chinese.lng"
      "${RELEASE_DIR}/lang/Traditional Chinese.lng"
    )
  add_custom_command(
    OUTPUT
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang/Traditional Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Default.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/English.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/French.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/German.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Japanese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Korean.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Russian.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Simplified Chinese.lng"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lang_utf16le/Traditional Chinese.lng"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang ${CMAKE_BINARY_DIR}/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/lang_utf16le ${CMAKE_BINARY_DIR}/lang_utf16le
    )
endif()
