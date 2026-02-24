#
# ex:
#  cmake -P download.cmake
#  cmake -DFORCE_DOWNLOAD=on -P download.cmake
# memo:
#  スクリプト中のコメントのないhash値は、
#  TeraTerm Project が独自に算出したものです

option(FORCE_DOWNLOAD "force download" OFF)
option(FORCE_EXTRACT "force extract" OFF)

# download_extract()
#   ファイルをダウンロードして、展開する
#
#   ファイルが展開されているかチェック
#     EXT_DIR          展開されているフォルダ
#     CHECK_FILE       アーカイブの中にあり、hashをチェックするファイル（チェックファイル）
#                      このファイルの有無とhashをチェックする
#                      ${EXT_DIR}相対  ${EXT_DIR}/${CHECK_FILE}
#     CHECK_FILE_HASH  チェックファイルhash値(sha256)
#   ダウンロード
#      SRC_URL         アーカイブファイルのURL
#      ARC_HASH        アーカイブファイルのhash値
#                      または、hashの入っているファイルのURL
#      DOWN_DIR        ダウンロードしたファイルの収納ディレクトリ
#   展開
#      DIR_IN_ARC      アーカイブ内のフォルダ名
#      RENAME_DIR      展開後 ${DIR_IN_ARC} をリネーム
#                      ${EXT_DIR}/${RENAME_DIR} ができる
function(download_extract SRC_URL ARC_HASH DOWN_DIR EXT_DIR DIR_IN_ARC RENAME_DIR CHECK_FILE CHECK_FILE_HASH)

  # ファイルが展開されているかチェック
  if((NOT FORCE_EXTRACT) AND (NOT FORCE_DOWNLOAD))
    if(EXISTS ${EXT_DIR}/${CHECK_FILE})
      file(SHA256 ${EXT_DIR}/${CHECK_FILE} CHECK_FILE_HASH_ACTUAL)

      message("CHECK_FILE: ${EXT_DIR}/${CHECK_FILE}")
      message("ACTUAL_HASH: ${CHECK_FILE_HASH_ACTUAL}")
      message("EXPECT_HASH: ${CHECK_FILE_HASH}")

      # ファイルが展開されていて、チェックファイルのhashが一致したら終了
      if(${CHECK_FILE_HASH_ACTUAL} STREQUAL ${CHECK_FILE_HASH})
        message("Hash match.")
        return()
      endif()

      message("Hash mismatch.")
    else()
      message("CHECK_FILE: ${EXT_DIR}/${CHECK_FILE}")
      message("File not found.")
    endif()
  endif()


  # アーカイブファイル名(フォルダ含まない)
  string(REGEX REPLACE "(.*)/([^/]*)$" "\\2" SRC_ARC ${SRC_URL})

  # アーカイブファイルのhash値
  if("${ARC_HASH}" MATCHES "http")
    # download the `hash file`
    string(REGEX REPLACE "(.*)/([^/]*)$" "\\2" HASH_FNAME ${ARC_HASH})
    if((NOT EXISTS ${DOWN_DIR}/${HASH_FNAME}) OR FORCE_DOWNLOAD)
      message("download ${ARC_HASH}")
      file(DOWNLOAD
        ${ARC_HASH}
        ${DOWN_DIR}/${HASH_FNAME}
        SHOW_PROGRESS
        )
    endif()
    file(STRINGS ${DOWN_DIR}/${HASH_FNAME} ARC_FILE_HASH)
    string(REGEX REPLACE "^(.+) (.+)$" "\\1" ARC_FILE_HASH ${ARC_FILE_HASH})
  else()
    # 渡された値そのまま
    set(ARC_FILE_HASH ${ARC_HASH})
  endif()

  # ARC_FILE_HASHの文字長からhashの種別を決める
  string(LENGTH ${ARC_FILE_HASH} HASH_LEN)
  if(${HASH_LEN} EQUAL 64)
    set(ARC_HASH_TYPE "SHA256")
  else()
    message(FATAL_ERROR "unknwon hash HASH=${ARC_FILE_HASH} HASH_LEN=${HASH_LEN}")
  endif()

  set(DO_DOWNLOAD 1)
  if(NOT FORCE_DOWNLOAD)
    if(EXISTS ${DOWN_DIR}/${SRC_ARC})
      file(${ARC_HASH_TYPE} ${DOWN_DIR}/${SRC_ARC} ARC_FILE_HASH_ACTUAL)

      message("ARCHIVE: ${DOWN_DIR}/${SRC_ARC}")
      message("ACTUAL_HASH: ${ARC_HASH_TYPE}=${ARC_FILE_HASH_ACTUAL}")
      message("EXPECT_HASH: ${ARC_HASH_TYPE}=${ARC_FILE_HASH}")

      # ファイルがダウンロードされていて、アーカイブファイルのhashが一致したらダウンロードしない
      if(${ARC_FILE_HASH} STREQUAL ${ARC_FILE_HASH_ACTUAL})
        message("Hash match.")
        set(DO_DOWNLOAD 0)
      else()
        message("Hash mismatch.")
      endif()
    endif()
  endif()

  if(DO_DOWNLOAD)
    # アーカイブをダウンロード
    message("download ${SRC_URL}")
    set(MAX_RETRIES 3)
    set(RETRY_WAIT 30)
    set(RETRY_COUNT 0)
    set(DOWNLOAD_SUCCESS FALSE)
    while(NOT DOWNLOAD_SUCCESS AND
          (RETRY_COUNT LESS MAX_RETRIES OR RETRY_COUNT EQUAL MAX_RETRIES))
      # リトライの前に待つ
      if(RETRY_COUNT GREATER 0 AND
         (RETRY_COUNT LESS MAX_RETRIES OR RETRY_COUNT EQUAL MAX_RETRIES))
        message("Wait ${RETRY_WAIT} sec...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${RETRY_WAIT})
        message("Retrying...")
      endif()

      # ダウンロード
      file(DOWNLOAD
        ${SRC_URL}
        ${DOWN_DIR}/${SRC_ARC}
        SHOW_PROGRESS
        STATUS st
        LOG log
        )

      # ダウンロードのステータスを判
      list(GET st 0 status_code)
      if(status_code EQUAL 0)
        set(DOWNLOAD_SUCCESS TRUE)
      else()
        message("Download failed. ${st} ${log}")
        if(RETRY_COUNT EQUAL MAX_RETRIES)
          message(FATAL_ERROR "Maximum number of retries reached.")
        endif()
        math(EXPR RETRY_COUNT "${RETRY_COUNT} + 1")
      endif()
    endwhile()

    # アーカイブファイルのhashをチェクする
    file(${ARC_HASH_TYPE} ${DOWN_DIR}/${SRC_ARC} ARC_FILE_HASH_ACTUAL)
    message("ARCHIVE: ${DOWN_DIR}/${SRC_ARC}")
    message("ACTUAL_HASH: ${ARC_HASH_TYPE}=${ARC_FILE_HASH_ACTUAL}")
    message("EXPECT_HASH: ${ARC_HASH_TYPE}=${ARC_FILE_HASH}")
    if(NOT ${ARC_FILE_HASH} STREQUAL ${ARC_FILE_HASH_ACTUAL})
      message(FATAL_ERROR "Hash mismatch.")
    endif()
    message("Hash match.")
  endif()

  # アーカイブファイルを展開する
  message("expand ${EXT_DIR}/${DIR_IN_ARC}")
  file(MAKE_DIRECTORY ${EXT_DIR})
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXT_DIR}
    )

  # renameする
  message("rename ${EXT_DIR}/${RENAME_DIR}")
  file(REMOVE_RECURSE ${EXT_DIR}/${RENAME_DIR})
  file(RENAME ${EXT_DIR}/${DIR_IN_ARC} ${EXT_DIR}/${RENAME_DIR})

endfunction()


# argon2
function(download_argon2)
  message("argon2")
  set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
  set(DIR_IN_ARC "phc-winner-argon2-20190702")
  set(RENAME_DIR "argon2")
  set(CHECK_FILE "argon2/CHANGELOG.md")
  set(CHECK_FILE_HASH "1b513eb6524f0a3ac5e182bf2713618ddd8f2616ebe6e090d647c49b3e7eb2ec")
  set(SRC_URL "https://github.com/P-H-C/phc-winner-argon2/archive/refs/tags/20190702.tar.gz")
  set(ARC_HASH "daf972a89577f8772602bf2eb38b6a3dd3d922bf5724d45e7f9589b5e830442c")
  #   ARC_HASH by TeraTerm Project
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/argon2")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
endfunction()

# cJSON
function(download_cjson)
  message("cJSON")
  set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
  set(DIR_IN_ARC "cJSON-1.7.14")
  set(RENAME_DIR "cJSON")
  set(CHECK_FILE "cJSON/CHANGELOG.md")
  set(CHECK_FILE_HASH "4ff95e0060ea2dbc13720079399e77d404d89e514b569fcc8d741f3272c98e53")
  set(SRC_URL "https://github.com/DaveGamble/cJSON/archive/v1.7.14.zip")
  set(ARC_HASH "d797b4440c91a19fa9c721d1f8bab21078624aa9555fc64c5c82e24aa2a08221")
  #   ARC_HASH by TeraTerm Project
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/cJSON")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
endfunction()

# libressl
function(download_libressl)
  message("libressl")
  set(DIR_IN_ARC "libressl-4.2.1")
  set(RENAME_DIR "libressl")
  set(CHECK_FILE "libressl/ChangeLog")
  set(CHECK_FILE_HASH "f99c885d5318dc6357b4bb3563c0fb7ea536ff1734c67a436aa2532ebb4b5bd7")
  set(SRC_URL "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-4.2.1.tar.gz")
  set(ARC_HASH "6d5c2f58583588ea791f4c8645004071d00dfa554a5bf788a006ca1eb5abd70b")
  #   ARC_HASH was picked from https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/SHA256
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/libressl")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
endfunction()

# oniguruma
function(download_oniguruma)
  message("oniguruma")
  set(DIR_IN_ARC "onig-6.9.10")
  set(RENAME_DIR "oniguruma")
  set(CHECK_FILE "oniguruma/HISTORY")
  set(CHECK_FILE_HASH "bdf00b251fa9dfb2aea3e1c007a0994bd40203a56706402c7cebb976c41d0cae")
  set(SRC_URL "https://github.com/kkos/oniguruma/releases/download/v6.9.10/onig-6.9.10.tar.gz")
  set(ARC_HASH "https://github.com/kkos/oniguruma/releases/download/v6.9.10/onig-6.9.10.tar.gz.sha256")
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/oniguruma")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
endfunction()

# SFMT
function(download_sfmt)
  message("SFMT")
  set(DIR_IN_ARC "SFMT-src-1.5.1")
  set(RENAME_DIR "SFMT")
  set(CHECK_FILE "SFMT/CHANGE-LOG.txt")
  set(CHECK_FILE_HASH "ac65302c740579c7dccc99b2fcd735af3027957680f2ce227042755646abb1db")
  set(SRC_URL "https://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/SFMT-src-1.5.1.zip")
  set(ARC_HASH "630d1dfa6b690c30472f75fa97ca90ba62f9c13c5add6c264fdac2c1d3a878f4")
  #   ARC_HASH by TeraTerm Project
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/SFMT")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
  set(SFMT_VERSION_H "${EXT_DIR}/${RENAME_DIR}/SFMT_version_for_teraterm.h")
  if(NOT EXISTS ${SFMT_VERSION_H})
    file(WRITE ${SFMT_VERSION_H}
      "// created by download.cmake\n"
      "#pragma once\n"
      "#ifndef SFMT_VERSION_H\n"
      "#define SFMT_VERSION_H\n"
      "#define SFMT_VERSION \"1.5.1\"\n"
      "#endif\n"
    )
  endif()
endfunction()

# zlib
function(download_zlib)
  message("zlib")
  set(DIR_IN_ARC "zlib-1.3.2")
  set(RENAME_DIR "zlib")
  set(CHECK_FILE "zlib/ChangeLog")
  set(CHECK_FILE_HASH "c6f42df0f27486db922d633e7a70757b56c6cd9f742f21af419d232649efbede")
  set(SRC_URL "https://zlib.net/zlib-1.3.2.tar.xz")
  set(ARC_HASH "d7a0654783a4da529d1bb793b7ad9c3318020af77667bcae35f95d0e42a792f3")
  #   ARC_HASH was picked from https://www.zlib.net
  set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/zlib")
  download_extract(
    ${SRC_URL}
    ${ARC_HASH}
    ${DOWN_DIR}
    ${EXT_DIR}
    ${DIR_IN_ARC}
    ${RENAME_DIR}
    ${CHECK_FILE}
    ${CHECK_FILE_HASH}
  )
endfunction()

##########
if(NOT DEFINED TARGET)
  set(TARGET "all")
endif()
if(NOT DEFINED EXT_DIR)
  set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()
#message("TARGET=${TARGET}")
#message("EXT_DIR=${EXT_DIR}")

if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "cjson"))
  download_cjson()
endif()
if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "argon2"))
  download_argon2()
endif()
if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "libressl"))
  download_libressl()
endif()
if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "oniguruma"))
  download_oniguruma()
endif()
if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "sfmt"))
  download_sfmt()
endif()
if ((${TARGET} STREQUAL "all") OR (${TARGET} STREQUAL "zlib"))
  download_zlib()
endif()
