if (MSVC OR ("$ENV{MSYSTEM}" MATCHES "MINGW"))
  find_program(
    PERL perl.exe
    HINTS c:/Perl64/bin
    HINTS c:/Perl/bin
    HINTS c:/cygwin/usr/bin
    HINTS c:/cygwin64/usr/bin
    )
  file(TO_NATIVE_PATH ${PERL} PERL)
  find_program(
    HHC hhc.exe
    HINTS "C:/Program Files (x86)/HTML Help Workshop"
    HINTS "C:/Program Files/HTML Help Workshop"
    HINTS "$ENV{ProgramFiles}/HTML Help Workshop"
    HINTS "$ENV{ProgramFiles\(x86\)}/HTML Help Workshop"
    )
else()
  set(HHC "HHC-NOTFOUND")
endif()

if (NOT("${PERL}" STREQUAL "PERL-NOTFOUND") AND NOT("${HHC}" STREQUAL "HHC-NOTFOUND"))
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
    teraterm_chm ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    SOURCES ${SRC_EN}
    SOURCES htmlhelp_index_make.pl
    SOURCES htmlhelp_update_check.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/teraterm.chm"
    )

  add_custom_target(
    teratermj_chm ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    SOURCES htmlhelp_index_make.pl
    SOURCES htmlhelp_update_check.pl
    SOURCES ${SRC_JP}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/teratermj.chm"
    )

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_EN})
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_JP})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/oniguruma/COPYING   -o en/html/reference/Oniguruma-LICENSE.txt -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/oniguruma/doc/RE.ja -o en/html/reference/RE.txt      -c utf8   -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/openssl/LICENSE     -o en/html/reference/OpenSSL-LICENSE.txt   -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/putty/LICENCE       -o en/html/reference/PuTTY-LICENSE.txt     -l crlf
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/SFMT/LICENSE.txt    -o en/html/reference/SFMT-LICENSE.txt      -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../cygterm/COPYING          -o en/html/reference/CygTerm+-LICENSE.txt  -l unix
    COMMAND ${PERL} ../installer/copy-zliblicense.pl -i ../libs/zlib/README -o en/html/reference/zlib-LICENSE.txt   -l unix
    COMMAND ${PERL} -C0 -pe "s/^\\xef\\xbb\\xbf//" en/html/reference/build_with_cmake.md | ${PERL} Markdown_1.0.1/Markdown.pl > en/html/reference/build_with_cmake_utf8.html
    COMMAND ${PERL} ../installer/2sjis.pl -i en/html/reference/build_with_cmake_utf8.html -o en/html/reference/build_with_cmake.html  -c utf8
    COMMAND ${PERL} -C0 -pe "s/^\\xef\\xbb\\xbf//" en/html/reference/build_library_with_cmake.md | ${PERL} Markdown_1.0.1/Markdown.pl > en/html/reference/build_library_with_cmake_utf8.html
    COMMAND ${PERL} ../installer/2sjis.pl -i en/html/reference/build_library_with_cmake_utf8.html -o en/html/reference/build_library_with_cmake.html  -c utf8
    COMMAND ${PERL} htmlhelp_index_make.pl en html > en/Index.hhk
    COMMAND ${HHC} en/teraterm.hhp || (exit 0)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/teraterm.chm"
    )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/oniguruma/COPYING   -o ja/html/reference/Oniguruma-LICENSE.txt -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/oniguruma/doc/RE.ja -o ja/html/reference/RE.txt      -c utf8   -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/openssl/LICENSE     -o ja/html/reference/OpenSSL-LICENSE.txt   -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/putty/LICENCE       -o ja/html/reference/PuTTY-LICENSE.txt     -l crlf
    COMMAND ${PERL} ../installer/2sjis.pl -i ../libs/SFMT/LICENSE.txt    -o ja/html/reference/SFMT-LICENSE.txt      -l unix
    COMMAND ${PERL} ../installer/2sjis.pl -i ../cygterm/COPYING          -o ja/html/reference/CygTerm+-LICENSE.txt  -l unix
    COMMAND ${PERL} ../installer/copy-zliblicense.pl -i ../libs/zlib/README -o ja/html/reference/zlib-LICENSE.txt   -l unix
    COMMAND ${PERL} -C0 -pe "s/^\\xef\\xbb\\xbf//" ja/html/reference/build_with_cmake.md | ${PERL} Markdown_1.0.1/Markdown.pl > ja/html/reference/build_with_cmake_utf8.html
    COMMAND ${PERL} ../installer/2sjis.pl -i ja/html/reference/build_with_cmake_utf8.html -o ja/html/reference/build_with_cmake.html  -c utf8
    COMMAND ${PERL} -C0 -pe "s/^\\xef\\xbb\\xbf//" ja/html/reference/build_library_with_cmake.md | ${PERL} Markdown_1.0.1/Markdown.pl > ja/html/reference/build_library_with_cmake_utf8.html
    COMMAND ${PERL} ../installer/2sjis.pl -i ja/html/reference/build_library_with_cmake_utf8.html -o ja/html/reference/build_library_with_cmake.html  -c utf8
    COMMAND ${PERL} htmlhelp_index_make.pl ja html > ja/Index.hhk
    COMMAND ${HHC} ja/teraterm.hhp || (exit 0)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/teratermj.chm"
    )

  install(
    FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ja/teratermj.chm
    ${CMAKE_CURRENT_SOURCE_DIR}/en/teraterm.chm
    DESTINATION .
    )

else()

  # chmが存在したらコピーする
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ja/teraterm.chm)
    install(
      FILES ${CMAKE_CURRENT_SOURCE_DIR}/ja/teraterm.chm
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
