
cmake_minimum_required(VERSION 3.6)

project(teraterm_all)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(ENABLE_TTXSAMPLES "TTXSamples" ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_DEPRECATE /W3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_DEPRECATE /W3")
  # _WIN32_WINNT_WIN10 0x0A00
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0a00")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_WIN32_WINNT=0x0a00")
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 15 2017")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
  endif()
endif()

add_subdirectory(teraterm)
add_subdirectory(ttpmenu)
add_subdirectory(TTProxy)
add_subdirectory(ttssh2)
add_subdirectory(TTXKanjiMenu)
if(${ENABLE_TTXSAMPLES})
  add_subdirectory(TTXSamples)
endif()
