# cygterm, msys2termのビルド
# - 生成可能な cygterm, msys2term をビルド
# - CMAKE_INSTALL_PREFIX にコピーする
# 例
#  mkdir build_all && cd build_all && cmake -P ../build_cygterm.cmake

message("CMAKE_COMMAND=${CMAKE_COMMAND}")
message("CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

# cygwinのインストールパスを返す
# 見つからない場合は ""
function(GetCygwinPath path)
  set(${path} "" PARENT_SCOPE)
  if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "CYGWIN")
    # cygwinのcmake
    set(PATH "/usr/bin")
    if (EXISTS ${PATH})
      set(${path} ${PATH} PARENT_SCOPE)
    endif()
  elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    # windowsのcmake
    set(PATH "C:\\cygwin64\\bin")
    if (NOT EXISTS ${PATH})
      set(PATH "C:\\cygwin\\bin")
    endif()
    if (EXISTS ${PATH})
      set(${path} ${PATH} PARENT_SCOPE)
    endif()
  else()
    # msys2?
  endif()
endfunction()

# msys2のインストールパスを返す
# 見つからない場合は ""
function(GetMsys2Path path)
  set(${path} "" PARENT_SCOPE)
  if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "MSYS")
    # msys2のcmake
    set(PATH "/c/msys64/usr/bin")
    if (EXISTS ${PATH})
      set(${path} ${PATH} PARENT_SCOPE)
    endif()
  elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    if(${CMAKE_COMMAND} MATCHES "msys")
      # msys2/mingw64(32) のcmake
      set(PATH "c:/msys64/usr/bin")
      if (EXISTS ${PATH})
        #なぜかうまくいかない
        #set(${path} ${PATH} PARENT_SCOPE)
      endif()
    else()
      # windowsのcmake
      set(PATH "c:\\msys64\\usr\\bin")
      if (EXISTS ${PATH})
        set(${path} ${PATH} PARENT_SCOPE)
      endif()
    endif()
  else()
  endif()
endfunction()

function(build TARGET_CMAKE_COMMAND DIST_DIR GENERATE_OPTION)
  file(REMOVE_RECURSE ${DIST_DIR})
  file(MAKE_DIRECTORY ${DIST_DIR})
  if((NOT EXISTS ${TARGET_CMAKE_COMMAND}) AND (NOT EXISTS "${TARGET_CMAKE_COMMAND}.exe"))
    message("${TARGET_CMAKE_COMMAND} not found")
    return()
  endif()
  if(${TARGET_CMAKE_COMMAND} MATCHES "msys")
    # msys2のときは、c:/path -> /c/path に書き換える
    string(REGEX REPLACE "([A-z]):[/\\]" "/\\1/" CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")
    string(REGEX REPLACE "([A-z]):[/\\]" "/\\1/" GENERATE_OPTION "${GENERATE_OPTION}")
  endif()
  message("${TARGET_CMAKE_COMMAND} ${CMAKE_CURRENT_LIST_DIR} -G \"Unix Makefiles\" ${GENERATE_OPTION} ${MSYS2_ADD}")
  execute_process(
    COMMAND ${TARGET_CMAKE_COMMAND} ${CMAKE_CURRENT_LIST_DIR} -G "Unix Makefiles" ${GENERATE_OPTION} ${MSYS2_ADD}
    WORKING_DIRECTORY ${DIST_DIR}
    RESULT_VARIABLE rv
    )
  #message("rv=${rv}")
  execute_process(
    COMMAND ${TARGET_CMAKE_COMMAND} --build . --config release -- -j
    WORKING_DIRECTORY ${DIST_DIR}
    )
  if (DEFINED CMAKE_INSTALL_PREFIX)
    execute_process(
      COMMAND ${TARGET_CMAKE_COMMAND} --build . --config release --target install
      WORKING_DIRECTORY ${DIST_DIR}
      )
  endif()
endfunction()

## cygwin
GetCygwinPath(PATH)
set(ENV{PATH} ${PATH})
message("cygwin ENV{PATH}=$ENV{PATH}")

if (EXISTS ${PATH}/g++.exe)
  # gcc-core, gcc-g++ package (cygwin 64bit)
  set(GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/toolchain_x86_64-cygwin.cmake")
  if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/cygterm_x86_64")
  endif()
  build("${PATH}/cmake" "build_cygterm_x86_64" "${GENERATE_OPTIONS}")
  if(DEFINED CMAKE_INSTALL_PREFIX)
    file(COPY "${CMAKE_INSTALL_PREFIX}/cygterm_x86_64/cygterm.exe" DESTINATION ${CMAKE_INSTALL_PREFIX})
    file(COPY "${CMAKE_INSTALL_PREFIX}/cygterm_x86_64/cygterm.cfg" DESTINATION ${CMAKE_INSTALL_PREFIX})
  endif()
endif()

if (EXISTS ${PATH}/i686-pc-cygwin-g++.exe)
  # cygwin32-gcc-core, cygwin32-gcc-g++ package (cygwin 32bit on cygwin 64bit)
  set(GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/toolchain_i686-cygwin.cmake")
  if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/cygterm_i686")
  endif()
  build("${PATH}/cmake" "build_cygterm_i686" "${GENERATE_OPTIONS}")
endif()

## msys2
GetMsys2Path(PATH)
set(ENV{PATH} ${PATH})
message("msys2 ENV{PATH}=$ENV{PATH}")

if (EXISTS ${PATH}/g++.exe)
  unset(GENERATE_OPTIONS)
  if(DEFINED CMAKE_INSTALL_PREFIX)
    set(GENERATE_OPTIONS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
  endif()
  build("${PATH}/cmake" "build_msys2term_x86_64" "${GENERATE_OPTIONS}")
endif()
