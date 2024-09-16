# install msys2 in this folder
#  cmake -P install_msys2.cmake
set(MSYS_ROOT "${CMAKE_CURRENT_LIST_DIR}/msys64")

# msys2 root
if(EXISTS "${MSYS_ROOT}")
  file(MAKE_DIRECTORY "${MSYS_ROOT}")
endif()

set(SETUP_EXE "msys2-x86_64-20240727.exe")
set(SETUP_URL "https://github.com/msys2/msys2-installer/releases/download/2024-07-27/${SETUP_EXE}")
set(SETUP_HASH_SHA256 "20d452e66cc95f975b2a8c5d814ba02e92481071580e80a3e3502a391fff6d2a")
set(SETUP "${MSYS_ROOT}/${SETUP_EXE}")
set(PACMAN "${MSYS_ROOT}/usr/bin/pacman.exe")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/msys2_package")

# メッセージを英語で表示
set(ENV{LANG} "C")

# msys2のセットアップを準備
file(DOWNLOAD
  ${SETUP_URL}
  ${PACKAGE}/${SETUP_EXE}
  EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
  SHOW_PROGRESS
)
file(COPY ${PACKAGE}/${SETUP_EXE} DESTINATION ${MSYS_ROOT})

# install base
if(NOT EXISTS "${PACMAN}")
  execute_process(
    COMMAND ${SETUP}
    in --confirm-command --accept-messages --root ${MSYS_ROOT} --cache-path ${PACKAGE}
    WORKING_DIRECTORY ${MSYS_ROOT}
  )
endif()

# update
execute_process(
  COMMAND ${PACMAN} -Syuu --noconfirm --noprogressbar
  --cachedir ${PACKAGE}
  WORKING_DIRECTORY ${MSYS_ROOT}
)

# install
#  for msys2term
execute_process(
  COMMAND ${PACMAN} -S --noconfirm --needed --noprogressbar
  --cachedir ${PACKAGE}
  msys/make msys/cmake msys/gcc
  WORKING_DIRECTORY ${MSYS_ROOT}
  ENCODING none
)

# Tera Term
#  mingw-w64-i686-cmake mingw-w64-i686-gcc mingw-w64-i686-clang
#  mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-clang
