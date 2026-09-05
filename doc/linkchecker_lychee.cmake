# cmake -P linkchecker_lychee.cmake
#
# lychee で html のリンクをチェックする
#
# lychee は UTF-8 以外のファイルを読めないため,
# html を UTF-8 に変換したコピーを作業フォルダに作ってチェックする
# 作業フォルダは残しておき, 2 回目以降は更新されたファイルだけ変換する
#
# 必要なもの
#   lychee  buildtools/install_lychee.cmake でインストールされる
#   iconv   buildtools/cygwin64 (install_cygwin.cmake) などのもの

set(DOC_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(BUILDTOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/../buildtools")

# lychee
if(CMAKE_HOST_WIN32)
  include("${BUILDTOOLS_DIR}/install_lychee.cmake")
else()
  find_program(LYCHEE_EXE lychee)
  if(NOT LYCHEE_EXE)
    message(FATAL_ERROR "lychee not found")
  endif()
endif()

# iconv
find_program(
  ICONV iconv
  HINTS
    "${BUILDTOOLS_DIR}/cygwin64/bin"
    "C:/cygwin64/bin"
    "C:/cygwin/bin"
)
if(NOT ICONV)
  message(FATAL_ERROR "iconv not found")
endif()

# 作業フォルダ
set(WORK "./linkchecker_utf8_tmp")
# IS_NEWER_THAN は絶対パスでないと動作が保証されない
get_filename_component(WORK "${WORK}" ABSOLUTE)

# SRC_DIR のファイルを DST_DIR へコピーする
# html は UTF-8 に変換する, html 以外はそのままコピーする
# DST_DIR のファイルが SRC_DIR のファイルより新しければ何もしない
# SRC_DIR に無いファイルが DST_DIR にあれば削除する
function(copy_html_as_utf8 SRC_DIR DST_DIR)
  # SRC_DIR に無いファイルを削除
  file(GLOB_RECURSE DST_FILES RELATIVE "${DST_DIR}" "${DST_DIR}/*")
  foreach(F IN LISTS DST_FILES)
    if(NOT EXISTS "${SRC_DIR}/${F}")
      file(REMOVE "${DST_DIR}/${F}")
    endif()
  endforeach()

  file(GLOB_RECURSE SRC_FILES RELATIVE "${SRC_DIR}" "${SRC_DIR}/*")
  foreach(F IN LISTS SRC_FILES)
    set(SRC "${SRC_DIR}/${F}")
    set(DST "${DST_DIR}/${F}")

    # 変換先が存在して新しければ何もしない
    if(NOT "${SRC}" IS_NEWER_THAN "${DST}")
      continue()
    endif()

    get_filename_component(DIR "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${DIR}")

    # html 以外, ASCII だけの html は変換不要
    set(CONVERT FALSE)
    if(F MATCHES "\\.html?$")
      file(READ "${SRC}" CONTENT)
      if(CONTENT MATCHES "[^\t\r\n -~]")
        set(CONVERT TRUE)
      endif()
    endif()
    if(NOT CONVERT)
      configure_file("${SRC}" "${DST}" COPYONLY)
      continue()
    endif()

    # -c: 変換できない文字は捨てる
    #     en の html は CP932 でないもの(UTF-8, ISO-8859-1)が混在している
    #     リンクチェックには影響しないので, UTF-8 として正しい出力になればよい
    execute_process(
      COMMAND ${ICONV} -c -f CP932 -t UTF-8 "${SRC}"
      OUTPUT_FILE "${DST}"
      ERROR_VARIABLE ERR
      RESULT_VARIABLE RC
    )
    # 1 は -c で文字を捨てたとき
    if(NOT RC EQUAL 0 AND NOT RC EQUAL 1)
      message(FATAL_ERROR "iconv failed: ${F}\n${ERR}")
    endif()
  endforeach()
endfunction()

set(RESULT 0)
foreach(LANG ja en)
  message("${LANG}")
  copy_html_as_utf8("${DOC_DIR}/${LANG}/html" "${WORK}/${LANG}")
  set(LYCHEE_OPTION --include-fragments --no-progress)
  set(LYCHEE_OPTION --offline ${LYCHEE_OPTION})  # コメントアウトすると外部リンクもチェックする
  execute_process(
    COMMAND ${LYCHEE_EXE} ${LYCHEE_OPTION} "**/*.html"
    WORKING_DIRECTORY "${WORK}/${LANG}"
    RESULT_VARIABLE RC
    ENCODING UTF-8
  )
  if(NOT RC EQUAL 0)
    set(RESULT 1)
  endif()
endforeach()

if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR "link check failed")
endif()
