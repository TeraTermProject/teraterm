# cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=Win32 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake
# .\cmake-3.11.4-win32-x86\bin\cmake.exe -DCMAKE_GENERATOR="Visual Studio 8 2005" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -P buildall.cmake

if("${CMAKE_GENERATOR}" STREQUAL "")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

if(NOT "${ARCHITECTURE}" STREQUAL "")
  set(ARCHITECTURE_OPTION -DARCHITECTURE=${ARCHITECTURE})
endif()

if(NOT DEFINED BUILD_SSL_LIBRARY)
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005" OR ${CMAKE_GENERATOR} MATCHES "Visual Studio 9 2008")
    set(BUILD_SSL_LIBRARY OFF)
  else()
    set(BUILD_SSL_LIBRARY ON)
  endif()
endif()

message("BUILD_SSL_LIBRARY=${BUILD_SSL_LIBRARY}")

# install tools
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  if(NOT EXISTS c:/Strawberry/perl/bin/perl.exe)
    message("perl")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -P perl.cmake
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../buildtools
      )
  endif()
endif()

# build
message("oniguruma")
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P oniguruma.cmake
  )
message("zlib")
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P zlib.cmake
  )
message("putty")
execute_process(
  COMMAND ${CMAKE_COMMAND} -P putty.cmake
  )
message("SFMT")
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P SFMT.cmake
  )
if(BUILD_SSL_LIBRARY)
  message("openssl 1.1")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P openssl11.cmake
    )
endif(BUILD_SSL_LIBRARY)
message("cJSON")
execute_process(
  COMMAND ${CMAKE_COMMAND} -P cJSON.cmake
  )
message("argon2")
execute_process(
  COMMAND ${CMAKE_COMMAND} -P argon2.cmake
  )
if(BUILD_SSL_LIBRARY)
  message("libressl")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P buildlibressl.cmake
    )
endif(BUILD_SSL_LIBRARY)

message("done buildall.cmake")
