
cmake_minimum_required(VERSION 3.11)

project(teraterm_all)

if(MSVC)
  if((NOT DEFINED CMAKE_GENERATOR_PLATFORM) OR (CMAKE_GENERATOR_PLATFORM STREQUAL ""))
    set(CMAKE_GENERATOR_PLATFORM "win32")
  endif()
  set(ARCHITECTURE ${CMAKE_VS_PLATFORM_NAME})
else()
  if(NOT DEFINED ARCHITECTURE)
    message(FATAL_ERROR "ARCHITECTURE not defined")
  endif()
endif()

set(SUPPORT_OLD_WINDOWS_DEFAULT OFF)
set(BMP_ICO_DEFAULT OFF)
if(MSVC AND (MSVC_VERSION LESS_EQUAL 1400))
  # Visual Studio 2005 (1400=VS2005)
  set(SUPPORT_OLD_WINDOWS_DEFAULT ON)
  set(BMP_ICO_DEFAULT ON)
endif()
if(CMAKE_HOST_UNIX AND (NOT DEFINED CMAKE_TOOLCHAIN_FILE))
  message(FATAL_ERROR "add option -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/mingw.toolchain.cmake")
endif()

option(SUPPORT_OLD_WINDOWS "windows 95 support" ${SUPPORT_OLD_WINDOWS_DEFAULT})
option(BMP_ICO "icon(*.ico) file include only bmp(not png)" ${BMP_ICO_DEFAULT})
option(ENABLE_TTXSAMPLES "TTXSamples" ON)
option(MORE_WARNING "stronger warning" OFF)
option(TTXSSH "ttxssh" ON)
option(DYNAMIC_CODE_ANALYSIS "Dynamic code analysis" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 出力フォルダ
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    set(BINARY_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
  else()
    # 3.11のときはとりあえず debug ビルドと仮定する
    set(BINARY_DIR "${CMAKE_BINARY_DIR}/debug")
  endif()
else()
  set(BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

#set(_WIN32_WINNT "0x0a00") # 0x0a00=_WIN32_WINNT_WIN10
#set(_WIN32_WINNT "0x0501") # Tera Term がビルドできる最小値
if(MSVC AND (MSVC_VERSION LESS_EQUAL 1500))
  # Visual Studio 2005, 2008  (1500=VS2008)
  set(_WIN32_WINNT "0x0501")
  if(1)
    set(TTXSSH OFF)
  else()
    # この組み合わせはlibputtyがビルドできないので今のところ実現しない
    set(TTXSSH ON)
    set(TTXSSH_LIBRESSL OFF)
    set(TTXSSH_OPENSSL3 ON)
  endif()
endif()
if(${BMP_ICO})
  # cmake 3.11でRC_FLAGSが反映されない?,C_FLAGSも設定するとok
  # bmp形式icoについて teraterm/teraterm/common/ico/ico_scripts/readme.md 参照
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /DBMP_ICO")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DBMP_ICO")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DBMP_ICO")
endif()

if(MSVC)
  # https://www.leeholmes.com/blog/2017/02/27/differences-between-visual-studio-2012-2013-and-2015/
  if(_WIN32_WINNT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
  endif()
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  if (MORE_WARNING)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    if(${MSVC_VERSION} GREATER_EQUAL 1910) # Visual Studio 2017以上の時
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /analyze")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /analyze")
    endif()
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  endif()
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GF /Gy /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GF /Gy /Zi")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8")
  if(${MSVC_VERSION} GREATER_EQUAL 1910)
    # Visual Studio 2017以上の時
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
  endif()
  if(DYNAMIC_CODE_ANALYSIS)
    if(${MSVC_VERSION} GREATER_EQUAL 1920)
      # Visual Studio 2019以上の時
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /fsanitize=address")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /fsanitize=address")
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /fsanitize=address")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fsanitize=address")
    endif()
  endif()
elseif(MINGW)
  if(_WIN32_WINNT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=${_WIN32_WINNT}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=${_WIN32_WINNT}")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_MINGW_ANSI_STDIO=0")
  set(CMAKE_C_CXX_WARNING_FLAGS "-Wall -Wextra")
  if (NOT MORE_WARNING)
    set(CMAKE_C_CXX_WARNING_FLAGS "${CMAKE_C_CXX_WARNING_FLAGS} -Wno-unused-parameter -Wno-sign-compare")
    set(CMAKE_C_WARNING_FLAGS "${CMAKE_C_CXX_WARNING_FLAGS} -Wno-pointer-sign")
  else()
    set(CMAKE_C_CXX_WARNING_FLAGS "${CMAKE_C_CXX_WARNING_FLAGS}")
    set(CMAKE_C_WARNING_FLAGS "${CMAKE_C_CXX_WARNING_FLAGS}")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_WARNING_FLAGS} -static -ffunction-sections -fdata-sections ${C_ONLY_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_CXX_WARNING_FLAGS} -static -ffunction-sections -fdata-sections")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  endif()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -Wl,--gc-sections")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s -Wl,--gc-sections")
endif()

if(SUPPORT_OLD_WINDOWS)
  message("SUPPORT_OLD_WINDOWS=ON")
endif()

### tool
# perl
include(${CMAKE_CURRENT_LIST_DIR}/buildtools/checkperl.cmake)
message("perl=${PERL}")

# svn / git-svn
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  # git (git-svn)
  find_package(Git)
  message("git=${GIT_EXECUTABLE}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.svn")
  # svn
  find_package(Subversion)
  message("svn=${Subversion_SVN_EXECUTABLE}")
endif()

if(NOT $ENV{USER})
  set(USER $ENV{USER})
elseif(NOT $ENV{USERNAME})
  set(USER $ENV{USERNAME})
else()
  set(USER "NONAME")
endif()

# ソースツリー内に生成ファイルが存在していたら削除する
if(EXISTS "${CMAKE_SOURCE_DIR}/teraterm/ttpdlg/svnversion.h")
  file(REMOVE "${CMAKE_SOURCE_DIR}/teraterm/ttpdlg/svnversion.h")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/teraterm/common/svnversion.h")
  file(REMOVE "${CMAKE_SOURCE_DIR}/teraterm/common/svnversion.h")
endif()

if((NOT EXISTS "${CMAKE_BINARY_DIR}/build_config.cmake") OR
    (NOT EXISTS "${CMAKE_BINARY_DIR}/teraterm/common/svnversion.h") OR
    (NOT EXISTS "${CMAKE_SOURCE_DIR}/buildtools/svnrev/sourcetreeinfo.bat") OR
    (NOT EXISTS "${CMAKE_SOURCE_DIR}/installer/build_config.isl"))
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/teraterm/common)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -DPERL=${PERL} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DBINARY_DIR=${CMAKE_BINARY_DIR} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DSVN_EXECUTABLE=${Subversion_SVN_EXECUTABLE} -DARCHITECTURE=${ARCHITECTURE} -P svnrev.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/buildtools/svnrev
    RESULT_VARIABLE rv
  )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "execute error ${PERL}")
  endif()
endif()
include(${CMAKE_BINARY_DIR}/build_config.cmake)

if(DEFINED GITVERSION)
  message("git hash=\"${GITVERSION}\"")
  set(REVISION_TIME_USER "${DATE}_${TIME}-${GITVERSION}-${USER}")
elseif(DEFINED SVNVERSION)
  message("svn revision=\"${SVNVERSION}\"")
  set(REVISION_TIME_USER "r${SVNVERSION}-${DATE}_${TIME}-${USER}")
else()
  set(REVISION_TIME_USER "${DATE}_${TIME}-unknown-${USER}")
endif()
if(NOT DEFINED SNAPSHOT_DIR)
  set(SNAPSHOT_DIR "teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}")
endif()
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/${SNAPSHOT_DIR}")
if(NOT DEFINED SETUP_EXE)
  # not need ".exe"
  if(RELEASE)
    set(SETUP_EXE "teraterm-${VERSION}-${ARCHITECTURE}")
  else()
    set(SETUP_EXE "teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}")
  endif()
endif()
if(NOT DEFINED SETUP_ZIP)
  if(RELEASE)
    set(SETUP_ZIP "teraterm-${VERSION}-${ARCHITECTURE}.zip")
  else()
    set(SETUP_ZIP "teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}.zip")
  endif()
endif()

### subdirectories
add_subdirectory(teraterm)
add_subdirectory(ttpmenu)
add_subdirectory(TTProxy)
if(TTXSSH)
  add_subdirectory(ttssh2)
endif(TTXSSH)
add_subdirectory(TTXKanjiMenu)
if(${ENABLE_TTXSAMPLES})
  add_subdirectory(TTXSamples)
endif()
add_subdirectory(installer)
add_subdirectory(doc)
add_subdirectory(doc_internal)
add_subdirectory(tools)
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/TTXAdditional/CMakeLists.txt)
  add_subdirectory(TTXAdditional)
endif()
add_subdirectory(cygwin)

### create buildinfo.txt
configure_file(
  buildinfo.txt.in
  buildinfo.txt
  @ONLY)

### make_installer_cmake.bat/sh
if(CMAKE_HOST_WIN32)
  file(WRITE "${CMAKE_BINARY_DIR}/make_installer_cmake.bat"
    "pushd %~dp0\n"
    "\"${CMAKE_COMMAND}\" -P \"${CMAKE_SOURCE_DIR}/installer/install.cmake\"\n"
    "pause\n")
else()
  file(WRITE "${CMAKE_BINARY_DIR}/make_installer_cmake.sh"
    "\"${CMAKE_COMMAND}\" -P \"${CMAKE_SOURCE_DIR}/installer/install.cmake\"\n")
endif()
