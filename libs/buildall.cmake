# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64 -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake

if("${CMAKE_GENERATOR}" STREQUAL "")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

if(NOT "${ARCHITECTURE}" STREQUAL "")
  set(ARCHITECTURE_OPTION -DARCHITECTURE=${ARCHITECTURE})
endif()

# install tool
message("jom")
execute_process(
  COMMAND ${CMAKE_COMMAND} -P jom.cmake
  )

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
message("openssl")
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P openssl.cmake
  )
message("done buildall.cmake")
