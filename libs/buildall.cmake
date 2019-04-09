# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake

if("${CMAKE_GENERATOR}" STREQUAL "")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

if(NOT "${ARCHITECTURE}" STREQUAL "")
  set(ARCHITECTURE_OPTION -DARCHITECTURE=${ARCHITECTURE})
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
message("openssl")
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} ${ARCHITECTURE_OPTION} -P openssl.cmake
  )
