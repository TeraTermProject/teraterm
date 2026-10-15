# install msys2 in this folder
#  cmake -P install_msys2.cmake
#
# - ホストの環境を壊さない
# - msys2-base の tar.xz を展開
# - インストーラは使わない
#   - スタートメニューやアンインストール登録を行わない
#   - msys64/ と download/msys2_package/ に閉じる
set(MSYS_ROOT "${CMAKE_CURRENT_LIST_DIR}/msys64")

set(BASE_TAG "2024-07-27")
set(BASE_TAR "msys2-base-x86_64-20240727.tar.xz")
set(BASE_URL "https://github.com/msys2/msys2-installer/releases/download/${BASE_TAG}/${BASE_TAR}")
set(BASE_HASH_SHA256 "da19afcc9b635967b3bfc0db119bf848ed6c28fdde8e4678d9f838d678939060")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/msys2_package")
set(BASH "${MSYS_ROOT}/usr/bin/bash.exe")
set(PACMAN "${MSYS_ROOT}/usr/bin/pacman.exe")

# メッセージを英語で表示
set(ENV{LANG} "C")
# ホストの msys2/cygwin 環境の影響を受けないようにする
set(ENV{MSYSTEM} "MSYS")
set(ENV{CHERE_INVOKED} "1")

if(NOT EXISTS "${PACMAN}")
  # msys2-base をダウンロード
  file(DOWNLOAD
    ${BASE_URL}
    ${PACKAGE}/${BASE_TAR}
    EXPECTED_HASH SHA256=${BASE_HASH_SHA256}
    SHOW_PROGRESS
  )

  # 展開 (tar はトップに msys64/ を含む)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${PACKAGE}/${BASE_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    RESULT_VARIABLE rv
  )
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "extract failed (${rv})")
  endif()

  # 初回起動 (キーリング等の初期化が行われる)
  execute_process(
    COMMAND ${BASH} -l -c "true"
    WORKING_DIRECTORY ${MSYS_ROOT}
    ENCODING none
  )
endif()

# update
#  コア (msys2-runtime 等) の更新でプロセスが終了することがあるため 2 回実行する
execute_process(
  COMMAND ${PACMAN} -Syuu --noconfirm --noprogressbar
  --cachedir ${PACKAGE}
  WORKING_DIRECTORY ${MSYS_ROOT}
  ENCODING none
)
execute_process(
  COMMAND ${PACMAN} -Syuu --noconfirm --noprogressbar
  --cachedir ${PACKAGE}
  WORKING_DIRECTORY ${MSYS_ROOT}
  ENCODING none
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
