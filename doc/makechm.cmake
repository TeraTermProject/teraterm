# cmake -P makechm.cmake

if(CMAKE_HOST_WIN32)
  if("${CMAKE_COMMAND}" MATCHES "msys")
    # msys
    find_program(PERL perl.exe)
  else()
    find_program(
      PERL perl.exe
      HINTS ${CMAKE_CURRENT_LIST_DIR}/../buildtools/perl/perl/bin
      HINTS c:/Strawberry/perl/bin
      HINTS c:/Perl64/bin
      HINTS c:/Perl/bin
      HINTS c:/cygwin/usr/bin
      HINTS c:/cygwin64/usr/bin
      )
  endif()
  find_program(
    HHC hhc.exe
    HINTS "C:/Program Files (x86)/HTML Help Workshop"
    HINTS "C:/Program Files/HTML Help Workshop"
    HINTS "$ENV{ProgramFiles}/HTML Help Workshop"
#    HINTS "$ENV{ProgramFiles\(x86\)}/HTML Help Workshop"
    )
  find_program(
    CHMCMD chmcmd
    HINTS ${CMAKE_CURRENT_LIST_DIR}/../buildtools/chmcmd/
    )
else(CMAKE_HOST_WIN32)
  find_program(
    PERL perl
    )
  set(HHC "HHC-NOTFOUND")
  find_program(
    CHMCMD chmcmd
    )
endif(CMAKE_HOST_WIN32)

message("perl=${PERL}")
message("hhc=${HHC}")
message("chmcmd=${CHMCMD}")


function(ConvertHTML CMD_OPTION)
  set(CONV_CMD ${PERL} "2sjis.pl")
  set(CONV_CMD_OPTION ${CMD_OPTION})
  separate_arguments(CONV_CMD_OPTION)
  set(CMD ${CONV_CMD} ${CONV_CMD_OPTION})

  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    string(JOIN " " CMD_PRINT ${CMD})
  else()
    string(REPLACE ";" " " CMD_PRINT "${CMD}")
  endif()
  #message(STATUS ${CMD_PRINT})

  execute_process(
    COMMAND ${CMD}
    RESULT_VARIABLE rv
    )
  if(NOT "${rv}" STREQUAL "0")
    message(${CMD_PRINT})
    message(FATAL_ERROR "rv=${rv}")
  endif()
endfunction()


if (NOT("${PERL}" STREQUAL "PERL-NOTFOUND"))
  set(REF_E "en/html/reference")
  set(REF_J "ja/html/reference")
  ConvertHTML("-i ../libs/doc_help/Oniguruma-LICENSE.txt -o ${REF_E}/Oniguruma-LICENSE.txt -l unix")
  ConvertHTML("-i ../libs/doc_help/Oniguruma-LICENSE.txt -o ${REF_J}/Oniguruma-LICENSE.txt -l unix")
  ConvertHTML("-i ../libs/doc_help/en/RE                 -o ${REF_E}/RE.txt                -l unix -c utf8")
  ConvertHTML("-i ../libs/doc_help/ja/RE                 -o ${REF_J}/RE.txt                -l unix -c utf8")
  ConvertHTML("-i ../libs/doc_help/LibreSSL-LICENSE.txt  -o ${REF_E}/LibreSSL-LICENSE.txt  -l unix")
  ConvertHTML("-i ../libs/doc_help/LibreSSL-LICENSE.txt  -o ${REF_J}/LibreSSL-LICENSE.txt  -l unix")
  ConvertHTML("-i ../libs/doc_help/PuTTY-LICENSE.txt     -o ${REF_E}/PuTTY-LICENSE.txt     -l crlf")
  ConvertHTML("-i ../libs/doc_help/PuTTY-LICENSE.txt     -o ${REF_J}/PuTTY-LICENSE.txt     -l crlf")
  ConvertHTML("-i ../libs/doc_help/SFMT-LICENSE.txt      -o ${REF_E}/SFMT-LICENSE.txt      -l unix")
  ConvertHTML("-i ../libs/doc_help/SFMT-LICENSE.txt      -o ${REF_J}/SFMT-LICENSE.txt      -l unix")
  ConvertHTML("-i ../libs/doc_help/argon2-LICENSE.txt    -o ${REF_E}/argon2-LICENSE.txt    -l unix")
  ConvertHTML("-i ../libs/doc_help/argon2-LICENSE.txt    -o ${REF_J}/argon2-LICENSE.txt    -l unix")
  ConvertHTML("-i ../libs/doc_help/zlib-LICENSE.txt      -o ${REF_E}/zlib-LICENSE.txt      -l unix --zlib_special")
  ConvertHTML("-i ../libs/doc_help/zlib-LICENSE.txt      -o ${REF_J}/zlib-LICENSE.txt      -l unix --zlib_special")
  ConvertHTML("-i ../libs/doc_help/cJSON-LICENSE.txt     -o ${REF_E}/cJSON-LICENSE.txt     -l unix")
  ConvertHTML("-i ../libs/doc_help/cJSON-LICENSE.txt     -o ${REF_J}/cJSON-LICENSE.txt     -l unix")
  ConvertHTML("-i ../cygwin/cygterm/COPYING              -o ${REF_E}/CygTerm+-LICENSE.txt  -l unix")
  ConvertHTML("-i ../cygwin/cygterm/COPYING              -o ${REF_J}/CygTerm+-LICENSE.txt  -l unix")
  ConvertHTML("-i ${REF_E}/build_with_cmake.md           -o ${REF_E}/build_with_cmake.html")
  ConvertHTML("-i ${REF_J}/build_with_cmake.md           -o ${REF_J}/build_with_cmake.html")
  ConvertHTML("-i ${REF_E}/build_library_with_cmake.md   -o ${REF_E}/build_library_with_cmake.html")
  ConvertHTML("-i ${REF_J}/build_library_with_cmake.md   -o ${REF_J}/build_library_with_cmake.html")
  ConvertHTML("-i ${REF_E}/menu_id.md                    -o ${REF_E}/menu_id.html")
  ConvertHTML("-i ${REF_J}/menu_id.md                    -o ${REF_J}/menu_id.html")

  ConvertHTML("-i en/html/setup/folder.md                -o en/html/setup/folder.html")
  ConvertHTML("-i ja/html/setup/folder.md                -o ja/html/setup/folder.html")

  execute_process(
    COMMAND ${PERL} htmlhelp_index_make.pl en html -o en/Index.hhk
    COMMAND ${PERL} htmlhelp_index_make.pl ja html -o ja/Index.hhk
    )
endif()

if(NOT("${HHC}" STREQUAL "HHC-NOTFOUND"))
  execute_process(
    COMMAND ${HHC} en/teraterm.hhp
    )
  execute_process(
    COMMAND ${HHC} ja/teraterm.hhp
    )
elseif(NOT("${CHMCMD}" STREQUAL "CHMCMD-NOTFOUND"))
  execute_process(
    COMMAND ${CHMCMD} teraterm.hhp
    WORKING_DIRECTORY en
    )
  execute_process(
    COMMAND ${CHMCMD} teraterm.hhp
    WORKING_DIRECTORY ja
    )
endif()
