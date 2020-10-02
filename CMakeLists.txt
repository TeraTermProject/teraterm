
cmake_minimum_required(VERSION 3.11)

project(teraterm_all)

option(ENABLE_TTXSAMPLES "TTXSamples" ON)
option(MORE_WARNING "stronger warning" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# _WIN32_WINNT_WIN10 0x0a00
set(_WIN32_WINNT "0x0a00")
if(MSVC)
  # https://www.leeholmes.com/blog/2017/02/27/differences-between-visual-studio-2012-2013-and-2015/
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005" OR ${CMAKE_GENERATOR} MATCHES "Visual Studio 9 2008")
    set(_WIN32_WINNT "0x0501")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  if (MORE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    if(${MSVC_VERSION} GREATER_EQUAL 1910) # Visual Studio 2017以上の時
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /analyze")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /analyze")
    endif()
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GF /Gy")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GF /Gy")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
  if(${MSVC_VERSION} GREATER_EQUAL 1910)
    # Visual Studio 2017以上の時
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
  endif()
elseif(MINGW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WIN32_WINNT=${_WIN32_WINNT}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=${_WIN32_WINNT}")
  set(CMAKE_C_CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare")
  set(CMAKE_C_WARNING_FLAGS "${CMAKE_C_CXX_WARNING_FLAGS}  -Wno-pointer-sign")
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

### tool
# perl
if (MSVC OR (MINGW AND (EXISTS "c:/windows")))
  find_program(
    PERL perl.exe
    HINTS ${CMAKE_SOURCE_DIR}/buildtools/perl/perl/bin
    HINTS c:/Strawberry/perl/bin
    HINTS c:/Perl64/bin
    HINTS c:/Perl/bin
    HINTS c:/cygwin/usr/bin
    HINTS c:/cygwin64/usr/bin
    )
else()
  find_program(
    PERL perl
    )
endif()
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

if(NOT EXISTS "${CMAKE_BINARY_DIR}/build_config.cmake")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/teraterm/ttpdlg)
  execute_process(
    COMMAND ${PERL} svnrev.pl --svn \"${Subversion_SVN_EXECUTABLE}\" --git \"${GIT_EXECUTABLE}\" --root ${CMAKE_SOURCE_DIR} --header ${CMAKE_BINARY_DIR}/teraterm/ttpdlg/svnversion.h --cmake ${CMAKE_BINARY_DIR}/build_config.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/svnrev_perl
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "execute error ${PERL}")
  endif()
endif()
include(${CMAKE_BINARY_DIR}/build_config.cmake)
message("revision=\"${SVNVERSION}\"")

if(DEFINED SVNVERSION)
  set(REVISION_TIME_USER "r${SVNVERSION}-${DATE}_${TIME}-${USER}")
else()
  set(REVISION_TIME_USER "unknown-${DATE}_${TIME}-${USER}")
endif()
if(NOT DEFINED SNAPSHOT_DIR)
  set(SNAPSHOT_DIR "snapshot-${REVISION_TIME_USER}")
endif()
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/${SNAPSHOT_DIR}")
if(NOT DEFINED SETUP_EXE)
  # not need ".exe"
  if(RELEASE)
    set(SETUP_EXE "teraterm-${VERSION}")
  else()
    set(SETUP_EXE "teraterm-${VERSION}-${REVISION_TIME_USER}")
  endif()
endif()
if(NOT DEFINED SETUP_ZIP)
  if(RELEASE)
    set(SETUP_ZIP "teraterm-${VERSION}.zip")
  else()
    set(SETUP_ZIP "teraterm-${VERSION}-${REVISION_TIME_USER}.zip")
  endif()
endif()

### subdirectories
add_subdirectory(teraterm)
add_subdirectory(ttpmenu)
add_subdirectory(TTProxy)
add_subdirectory(ttssh2)
add_subdirectory(TTXKanjiMenu)
if(${ENABLE_TTXSAMPLES})
  add_subdirectory(TTXSamples)
endif()
add_subdirectory(installer)
add_subdirectory(doc)
add_subdirectory(doc_internal)
add_subdirectory(tools)

### create buildinfo.txt
configure_file(
  buildinfo.txt.in
  buildinfo.txt
  @ONLY)
