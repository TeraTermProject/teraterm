
cmake_minimum_required(VERSION 3.6)

project(teraterm_all)

option(ENABLE_TTXSAMPLES "TTXSamples" ON)
option(MORE_WARNING "stronger warning" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

string(TIMESTAMP SNAPSHOT_DIR "snapshot-%Y%m%d_%H%M%S")
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/${SNAPSHOT_DIR}")

# _WIN32_WINNT_WIN10 0x0a00
set(_WIN32_WINNT "0x0a00")
if(MSVC)
  # https://www.leeholmes.com/blog/2017/02/27/differences-between-visual-studio-2012-2013-and-2015/
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005")
    set(_WIN32_WINNT "0x0501")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_WIN32_WINNT=${_WIN32_WINNT}")
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_DEPRECATE")
  if (MORE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    if(${MSVC_VERSION} GREATER_EQUAL 1910) # Visual Studio 2017以上の時
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /analyze")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /analyze")
    endif()
  else()
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
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -static -ffunction-sections -fdata-sections ${C_ONLY_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -static -ffunction-sections -fdata-sections")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  endif()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -s -Wl,--gc-sections")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -s -Wl,--gc-sections")
endif()

### create version_info.h
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  # git-svn
  find_package(Git)
  if(Git_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} svn log --oneline -1
      RESULT_VARIABLE rv
      OUTPUT_VARIABLE ov
      ERROR_QUIET)
    if(${rv})
      message("git svn error? please check git svn")
    else()
      string(REGEX MATCH "[0-9]+" SVNVERSION "${ov}")
    endif()
  endif()
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.svn")
  # svn
  find_package(Subversion)
  if(SUBVERSION_FOUND)
    Subversion_WC_INFO(${CMAKE_SOURCE_DIR} TT)
    set(SVNVERSION "${TT_WC_REVISION}")
  endif()
else()
  # build without svn,git?
  #set(SVNVERSION "0000")
endif()
configure_file(
  version_info.h.in
  version_info.h
  @ONLY)
message("revision=\"${SVNVERSION}\"")

####
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

####
# ソースツリー内に生成ファイルが存在していたら削除する
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/teraterm/ttpdlg/svnversion.h")
  # r7517より前
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/teraterm/ttpdlg/svnversion.h")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/teraterm/teraterm/svnversion.h")
  # r7517以後
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/teraterm/teraterm/svnversion.h")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ttssh2/ttxssh/puttyversion.h")
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/ttssh2/ttxssh/puttyversion.h")
endif()
