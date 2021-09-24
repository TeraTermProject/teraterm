#
if(NOT DEFINED CMAKE_GENERATOR)
  if(${CMAKE_COMMAND} MATCHES "mingw")
    # meybe mingw
    set(CMAKE_GENERATOR "Unix Makefiles")
  elseif(CMAKE_HOST_WIN32)
    set(CMAKE_GENERATOR "Visual Studio 16 2019")
  else()
    set(CMAKE_GENERATOR "Unix Makefiles")
  endif()
endif()
if((${CMAKE_GENERATOR} MATCHES "Visual Studio 16 2019") OR
    (${CMAKE_GENERATOR} MATCHES "Visual Studio 17 2022"))
  # 32bit build for inno setup
  set(GENERATE_OPTION "-A;Win32")
elseif(${CMAKE_COMMAND} MATCHES "msys64/mingw64/bin")
  message("switch msys 32bit env")
  set(ENV{PATH} "c:\\msys64\\mingw32\\bin;c:\\msys64\\usr\\bin")
  set(CMAKE_COMMAND "C:/msys64/mingw32/bin/cmake.exe")
endif()

message("CMAKE_GENERATOR=${CMAKE_GENERATOR}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
message("CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

execute_process(
  COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_LIST_DIR} -G ${CMAKE_GENERATOR} ${GENERATE_OPTION} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
  )

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config release
  )

if(DEFINED CMAKE_INSTALL_PREFIX)
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config release --target install
    )
endif()
