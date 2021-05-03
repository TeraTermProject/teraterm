set(COMPILER VS_142)
set(COMPILER_FRIENDLY vs2019_x64)
list(APPEND GENERATE_OPTIONS "-G" "Visual Studio 16 2019" "-A" "x64")
#set(CMAKE_OPTION_LIBS -DARCHITECTURE=Win32)
#list(APPEND BUILD_OPTIONS "--config" "Release")
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../build_${COMPILER_FRIENDLY}_appveyor")

include(${CMAKE_CURRENT_LIST_DIR}/build_appveyor.cmake)
