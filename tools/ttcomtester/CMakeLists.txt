set(PACKAGE_NAME "ttcomtester")

project(${PACKAGE_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PACKAGE_NAME}
  buildcommdcbw.cpp
  buildcommdcbw.h
  device_com.cpp
  deviceope.h
  main.cpp
)

if(0)
  target_sources(${PACKAGE_NAME}
    PRIVATE
    ../libs/getopt_mb_uni_src/getopt.c
    ../libs/getopt_mb_uni_src/getopt.h
  )
  target_include_directories(${PACKAGE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/getopt_mb_uni_src
  )
endif()
if(1)
  target_sources(${PACKAGE_NAME}
    PRIVATE
    ../libs/getoptW/getoptW.c
    ../libs/getoptW/getoptW.h
  )
  target_include_directories(${PACKAGE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/getoptW
  )
endif()

target_compile_options(
  ${PACKAGE_NAME}
  PRIVATE
  /W4
)

target_compile_options(
  ${PACKAGE_NAME}
  PRIVATE
  -W4
  )

target_compile_definitions(
  ${PACKAGE_NAME}
  PRIVATE
  STATIC_GETOPT
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  winmm.lib
)

if(MINGW)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -municode
    )
endif()
