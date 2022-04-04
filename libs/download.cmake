#
# ex:
#  cmake -P download.cmake
#  cmake -DFORCE_DOWNLOAD=on -P download.cmake
# memo:
#  スクリプト中のコメントのないhash値は、
#  Tera Term Project が独自に算出しました

option(FORCE_DOWNLOAD "force download" OFF)
option(FORCE_EXTRACT "force extract" OFF)

# download_extract()
#   ファイルをダウンロードして、展開する
#
#   ファイルが展開されているかチェック
#    EXT_DIR       展開されているフォルダ
#    CHECK_FILE    チェックファイル,このファイルの有無とhashをチェックする
#                  ${EXT_DIR}相対  ${EXT_DIR}/${CHECK_FILE}
#    CHECK_HASH    チェックファイルhash値(sha256)
#   ダウンロード
#    SRC_URL       アーカイブファイルのURL
#    ARC_HASH      アーカイブファイルのhash値
#                  または、hashの入っているファイルのURL
#    DOWN_DIR      ダウンロードしたファイルの収納ディレクトリ
#   展開
#    DIR_IN_ARC    アーカイブ内のフォルダ名
#    RENAME_DIR    展開後 ${DIR_IN_ARC} をリネーム
#                  ${EXT_DIR}/${RENAME_DIR} ができる
function(download_extract SRC_URL ARC_HASH DOWN_DIR EXT_DIR DIR_IN_ARC RENAME_DIR CHECK_FILE CHECK_HASH)

  # ファイルが展開されているかチェック
  if((NOT FORCE_EXTRACT) AND (NOT FORCE_DOWNLOAD))
    if(EXISTS ${EXT_DIR}/${CHECK_FILE})
      file(SHA256 ${EXT_DIR}/${CHECK_FILE} HASH)

      if(${HASH} STREQUAL ${CHECK_HASH})
        return()
      endif()

      message("${EXT_DIR}/${CHECK_FILE}")
      message("ACTUAL_HASH=${HASH}")
      message("EXPECT_HASH=${CHECK_HASH}")
    else()
      message("not exist ${EXT_DIR}/${CHECK_FILE}")
    endif()
  endif()


  # アーカイブファイル名(フォルダ含まない)
  string(REGEX REPLACE "(.*)/([^/]*)$" "\\2" SRC_ARC ${SRC_URL})

  # ダウンロードファイルのHASH値
  if("${ARC_HASH}" MATCHES "http")
    # download hash
    string(REGEX REPLACE "(.*)/([^/]*)$" "\\2" HASH_FNAME ${ARC_HASH})
    if((NOT EXISTS ${DOWN_DIR}/${HASH_FNAME}) OR FORCE_DOWNLOAD)
      message("download ${ARC_HASH}")
      file(DOWNLOAD
        ${ARC_HASH}
        ${DOWN_DIR}/${HASH_FNAME}
        SHOW_PROGRESS
        )
    endif()
    file(STRINGS ${DOWN_DIR}/${HASH_FNAME} HASH)
    string(REGEX REPLACE "^(.+) (.+)$" "\\1" HASH ${HASH})
  else()
    # HASH値そのまま
    set(HASH ${ARC_HASH})
  endif()

  # HASHの文字長からHASHの種別を決める
  string(LENGTH ${HASH} HASH_LEN)
  if(${HASH_LEN} EQUAL 64)
    set(ARC_HASH_TYPE "SHA256")
  else()
    message(FATAL_ERROR "unknwon hash HASH=${HASH} HASH_LEN=${HASH_LEN}")
  endif()

  message("ARCHIVE=${DOWN_DIR}/${SRC_ARC}")
  message("ARCHIVE HASH ${ARC_HASH_TYPE}=${HASH}")
  if(FORCE_DOWNLOAD)
    # 常にダウンロードする
    unset(EXPECTED_HASH)
  else()
    # 必要ならダウンロードする
    set(EXPECTED_HASH EXPECTED_HASH "${ARC_HASH_TYPE}=${HASH}")
  endif()

  # アーカイブをダウンロード
  message("download ${SRC_URL}")
  file(DOWNLOAD
    ${SRC_URL}
    ${DOWN_DIR}/${SRC_ARC}
    ${EXPECTED_HASH}
    SHOW_PROGRESS
    STATUS st
    )

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
message("argon2")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "phc-winner-argon2-20190702")
set(RENAME_DIR "argon2")
set(CHECK_FILE "argon2/CHANGELOG.md")
set(CHECK_HASH "1b513eb6524f0a3ac5e182bf2713618ddd8f2616ebe6e090d647c49b3e7eb2ec")
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
  ${CHECK_HASH}
  )

# cJSON
message("cJSON")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "cJSON-1.7.14")
set(RENAME_DIR "cJSON")
set(CHECK_FILE "cJSON/CHANGELOG.md")
set(CHECK_HASH "4ff95e0060ea2dbc13720079399e77d404d89e514b569fcc8d741f3272c98e53")
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
  ${CHECK_HASH}
  )

# libressl
message("libressl")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "libressl-3.4.3")
set(RENAME_DIR "libressl")
set(CHECK_FILE "libressl/ChangeLog")
set(CHECK_HASH "96b2032157a92af8ae02ac57cdc9b96a96a3e91b89602936cf6403c9bf7e88ca")
set(SRC_URL "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.4.3.tar.gz")
set(ARC_HASH "ff88bffe354818b3ccf545e3cafe454c5031c7a77217074f533271d63c37f08d")
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
  ${CHECK_HASH}
  )

# oniguruma
message("oniguruma")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "onig-6.9.7")
set(RENAME_DIR "oniguruma")
set(CHECK_FILE "oniguruma/HISTORY")
set(CHECK_HASH "5ad81979c1e3beb313bf94e5a57c0772810b5b159cdfa629e2daf7b0643e1a3d")
set(SRC_URL "https://github.com/kkos/oniguruma/releases/download/v6.9.7.1/onig-6.9.7.1.tar.gz")
set(ARC_HASH "https://github.com/kkos/oniguruma/releases/download/v6.9.7.1/onig-6.9.7.1.tar.gz.sha256")
set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/oniguruma")
download_extract(
  ${SRC_URL}
  ${ARC_HASH}
  ${DOWN_DIR}
  ${EXT_DIR}
  ${DIR_IN_ARC}
  ${RENAME_DIR}
  ${CHECK_FILE}
  ${CHECK_HASH}
  )

# putty
message("putty")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "putty-0.76")
set(RENAME_DIR "putty")
set(CHECK_FILE "putty/version.h")
set(CHECK_HASH "c6975607ae6fb8510d56fae403fdd83bbafa3efe32397e5956375db49d0fadda")
set(SRC_URL "https://the.earth.li/~sgtatham/putty/0.76/putty-0.76.tar.gz")
set(ARC_HASH "547cd97a8daa87ef71037fab0773bceb54a8abccb2f825a49ef8eba5e045713f")
#   ARC_HASH was picked from https://the.earth.li/~sgtatham/putty/0.76/sha256sums
set(DOWN_DIR "${CMAKE_CURRENT_LIST_DIR}/download/putty")
download_extract(
  ${SRC_URL}
  ${ARC_HASH}
  ${DOWN_DIR}
  ${EXT_DIR}
  ${DIR_IN_ARC}
  ${RENAME_DIR}
  ${CHECK_FILE}
  ${CHECK_HASH}
  )

# SFMT
message("SFMT")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "SFMT-src-1.5.1")
set(RENAME_DIR "SFMT")
set(CHECK_FILE "SFMT/CHANGE-LOG.txt")
set(CHECK_HASH "ac65302c740579c7dccc99b2fcd735af3027957680f2ce227042755646abb1db")
set(SRC_URL "http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/SFMT-src-1.5.1.zip")
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
  ${CHECK_HASH}
  )
set(SFMT_VERSION_H "${EXT_DIR}/${RENAME_DIR}/SFMT_version_for_teraterm.h")
if(NOT EXISTS ${SFMT_VERSION_H})
  file(WRITE ${SFMT_VERSION_H}
    "#define SFMT_VERSION \"1.5.1\"\n"
    )
endif()

# zlib
message("zlib")
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DIR_IN_ARC "zlib-1.2.12")
set(RENAME_DIR "zlib")
set(CHECK_FILE "zlib/ChangeLog")
set(CHECK_HASH "66a955bd457c93d490c13b82f283b5507972fe50bcd8d9d6dd821ab57f187c2d")
set(SRC_URL "https://zlib.net/zlib-1.2.12.tar.xz")
set(ARC_HASH "7db46b8d7726232a621befaab4a1c870f00a90805511c0e0090441dac57def18")
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
  ${CHECK_HASH}
  )
