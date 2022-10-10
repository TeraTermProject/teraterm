if (MSVC OR (MINGW AND (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")))
  find_program(
    PERL perl.exe
    HINTS ${CMAKE_CURRENT_LIST_DIR}/perl/perl/bin
    HINTS c:/Strawberry/perl/bin
    HINTS c:/Perl64/bin
    HINTS c:/Perl/bin
    HINTS c:/cygwin/usr/bin
    HINTS c:/cygwin64/usr/bin
  )
else()
  # (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  find_program(
    PERL perl
  )
endif()

if ("${PERL}" STREQUAL "PERL-NOTFOUND")
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -P getperl.cmake
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildtools
    )
    set(PERL ${CMAKE_CURRENT_LIST_DIR}/perl/perl/bin)
  else()
    message("perl not installed")
  endif()
endif()
