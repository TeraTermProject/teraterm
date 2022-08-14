# cygterm, msys2termのビルド
# - 生成可能な cygterm, msys2term をビルド
# - CMAKE_INSTALL_PREFIX にコピーする
# 例
#  mkdir build_all && cd build_all && cmake -DCMAKE_INSTALL_PREFIX=c:/tmp/cygterm -P ../build_cygterm.cmake

message("CMAKE_COMMAND=${CMAKE_COMMAND}")
message("CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX not defined")
endif()
if((NOT (${CMAKE_INSTALL_PREFIX} MATCHES "^[A-z]:[/\\]")) AND (NOT(${CMAKE_INSTALL_PREFIX} MATCHES "^/")))
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX(${CMAKE_INSTALL_PREFIX}) must absolute")
endif()

function(ToCygwinPath in_path out_var)
  if("${in_path}" MATCHES "^[A-z]:[/\\]")
    # windows native absolute path
    string(REGEX REPLACE "([A-z]):[/\\]" "/cygdrive/\\1/" new_path "${in_path}")
    set(${out_var} ${new_path} PARENT_SCOPE)
  else()
    set(${out_var} ${in_path} PARENT_SCOPE)
  endif()
endfunction()

function(ToMsys2Path in_path out_var)
  if("${in_path}" MATCHES "^[A-z]:[/\\]")
    # windows native absolute path
    string(REGEX REPLACE "([A-z]):[/\\]" "/\\1/" new_str "${in_path}")
    set(${out_var} ${new_str} PARENT_SCOPE)
  else()
    set(${out_var} ${in_path} PARENT_SCOPE)
  endif()
endfunction()

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
    if (EXISTS "c:/msys64/usr/bin/msys-2.0.dll")
      set(${path} ${PATH} PARENT_SCOPE)
    endif()
  elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    # msys2/mingw64 or msys2/mingw32 or Windows のcmake
    set(PATH "c:/msys64/usr/bin")   # msys2インストールフォルダ
    if (EXISTS ${PATH})
      set(${path} ${PATH} PARENT_SCOPE)
    endif()
  elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "CYGWIN")
    # cygwin の cmake ,未実装
    unset(${path} PARENT_SCOPE)
  else()
    message("?")
  endif()
endfunction()

function(build TARGET_CMAKE_COMMAND SRC_DIR BUILD_DIR INSTALL_DIR GENERATE_OPTIONS)
  file(REMOVE_RECURSE ${BUILD_DIR})
  file(MAKE_DIRECTORY ${BUILD_DIR})
  if((NOT EXISTS ${TARGET_CMAKE_COMMAND}) AND (NOT EXISTS "${TARGET_CMAKE_COMMAND}.exe"))
    message("${TARGET_CMAKE_COMMAND} not found")
    return()
  endif()
  list(APPEND GENERATE_OPTIONS -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR})
  execute_process(
    COMMAND ${TARGET_CMAKE_COMMAND} ${SRC_DIR} -G "Unix Makefiles" ${GENERATE_OPTIONS}
    WORKING_DIRECTORY ${BUILD_DIR}
    RESULT_VARIABLE rv
  )
  #message("rv=${rv}")
  execute_process(
    COMMAND ${TARGET_CMAKE_COMMAND} --build . --config release --target install -- -j
    WORKING_DIRECTORY ${BUILD_DIR}
  )
endfunction()

## cygwin
GetCygwinPath(PATH)
set(ENV{PATH} ${PATH})
message("cygwin ENV{PATH}=$ENV{PATH}")

if (EXISTS ${PATH}/g++.exe)
  # gcc-core, gcc-g++ package (cygwin 64bit)
  ToCygwinPath(${CMAKE_CURRENT_LIST_DIR} CURRENT_LIST_DIR)
  set(GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CURRENT_LIST_DIR}/toolchain_x86_64-cygwin.cmake")
  ToCygwinPath(${CMAKE_INSTALL_PREFIX} INSTALL_PREFIX)
  build("${PATH}/cmake" ${CURRENT_LIST_DIR} "build_cygterm_x86_64" "${INSTALL_PREFIX}/cygterm+-x86_64" "${GENERATE_OPTIONS}")
  file(COPY "${CMAKE_INSTALL_PREFIX}/cygterm+-x86_64/cygterm.exe" DESTINATION ${CMAKE_INSTALL_PREFIX})
  file(COPY "${CMAKE_INSTALL_PREFIX}/cygterm+-x86_64/cygterm.cfg" DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (EXISTS ${PATH}/i686-pc-cygwin-g++.exe)
  # cygwin32-gcc-core, cygwin32-gcc-g++ package (cygwin 32bit on cygwin 64bit)
  ToCygwinPath(${CMAKE_CURRENT_LIST_DIR} CURRENT_LIST_DIR)
  set(GENERATE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CURRENT_LIST_DIR}/toolchain_i686-cygwin.cmake")
  ToCygwinPath(${CMAKE_INSTALL_PREFIX} INSTALL_PREFIX)
  build("${PATH}/cmake" ${CURRENT_LIST_DIR} "build_cygterm_i686" "${INSTALL_PREFIX}/cygterm+-i686" "${GENERATE_OPTIONS}")
endif()

## msys2
GetMsys2Path(PATH)
if(DEFINED PATH)
  set(ENV{PATH} "/usr/bin") # msys2 の cmake を使うので決め打ち
  set(ENV{MSYSTEM} "MSYS")
  message("PATH=${PATH}")
  message("msys2 ENV{PATH}=$ENV{PATH}")

  ToMsys2Path(${CMAKE_CURRENT_LIST_DIR} CURRENT_LIST_DIR)
  set(GENERATE_OPTIONS "")
  ToMsys2Path(${CMAKE_INSTALL_PREFIX} INSTALL_PREFIX)
  build("${PATH}/cmake" ${CURRENT_LIST_DIR} "build_msys2term_x86_64" "${INSTALL_PREFIX}" "${GENERATE_OPTIONS}")

endif()
