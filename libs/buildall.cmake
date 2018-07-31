# cmake -DCMAKE_GENERATOR="Vsual Studio 15 2017" -P buildall.cmake 

if("${CMAKE_GENERATOR}" STREQUAL "")
  #set(CMAKE_GENERATOR "Visual Studio 15 2017")
  message(FATAL_ERROR "set CMAKE_GENERATOR!")
endif()

# utf-8
execute_process(
  COMMAND cmd.exe /c chcp 65001
  )

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
