# cmake -DCMAKE_GENERATOR="Vsual Studio 15 2017" -P buildall.cmake
# cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake

if("${CMAKE_GENERATOR}" STREQUAL "")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

# build
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -P oniguruma.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -P zlib.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -P putty.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -P SFMT.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -P openssl.cmake
  )
