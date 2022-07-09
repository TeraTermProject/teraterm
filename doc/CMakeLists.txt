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
    HINTS "$ENV{ProgramFiles\(x86\)}/HTML Help Workshop"
    )
  find_program(
    CHMCMD chmcmd
    HINTS ${CMAKE_CURRENT_LIST_DIR}/../../buildtools/chmcmd/
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

if ("${PERL}" STREQUAL "PERL-NOTFOUND")
  return()
endif()
if (("${HHC}" STREQUAL "HHC-NOTFOUND") AND ("${CHMCMD}" STREQUAL "CHMCMD-NOTFOUND"))
  return()
endif()

add_subdirectory(en)
set_target_properties(chm_en PROPERTIES FOLDER doc)
add_subdirectory(ja)
set_target_properties(chm_ja PROPERTIES FOLDER doc)
