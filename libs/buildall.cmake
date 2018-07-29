# cmake -DGENERATOR="Vsual Studio 15 2017" -P buildall.cmake 

if("${GENERATOR}" STREQUAL "")
  #set(GENERATOR "Visual Studio 15 2017")
  message(FATAL_ERROR "set GENERATOR!")
endif()

# utf-8
execute_process(
  COMMAND cmd.exe /c chcp 65001
  )

# build
execute_process(
  COMMAND ${CMAKE_COMMAND} -DGENERATOR=${GENERATOR} -P oniguruma.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DGENERATOR=${GENERATOR} -P zlib.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -P putty.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DGENERATOR=${GENERATOR} -P openssl.cmake
  )
execute_process(
  COMMAND ${CMAKE_COMMAND} -DGENERATOR=${GENERATOR} -P SFMT.cmake
  )
