# cmake -P icon_extract.cmake

#set(ICOTOOL "icotool")
find_program(
  ICOTOOL icotool
  HINTS ${CMAKE_CURRENT_LIST_DIR}/../../../../buildtools/cygwin64
  HINTS c:/cygwin64/bin
)
message("icotool=${ICOTOOL}")

# icoファイルを分解する
#
#   basename    icoファイルの拡張子をとったファイル名
#   rel_path    このファイルのあるフォルダからの相対パス
#
function(ico_extract basename rel_path)
  set(DEST ${basename})
  set(ICO "${rel_path}/${basename}.ico")
  if(NOT EXISTS ${ICO})
    message(FATAL_ERROR "ico not found ${ICO}")
  endif()
  if(EXISTS ${DEST})
    file(REMOVE_RECURSE ${DEST})
  endif()
  file(MAKE_DIRECTORY ${DEST})

  execute_process(
    COMMAND ${ICOTOOL} -x "../${ICO}"
    WORKING_DIRECTORY ${DEST}
  )
endfunction()

ico_extract("teraterm"          "../..")
ico_extract("teraterm_3d"       "../..")
ico_extract("teraterm_classic"  "../..")
ico_extract("teraterm_flat"     "../..")
ico_extract("tterm16"           "../..")
ico_extract("tek"               "../..")
ico_extract("tek16"             "../..")
ico_extract("cygterm"           "../../../../cygwin/cygterm")
ico_extract("vt"                "../..")
ico_extract("vt_3d"             "../..")
ico_extract("vt_classic"        "../..")
ico_extract("vt_flat"           "../..")
ico_extract("vt16"              "../..")
ico_extract("ttmacr16"          "../../../ttpmacro")
ico_extract("ttmacro"           "../../../ttpmacro")
ico_extract("ttmacrof"          "../../../ttpmacro")
ico_extract("ttmacro_3d"        "../../../ttpmacro")
ico_extract("ttmacro_flat"      "../../../ttpmacro")
ico_extract("ttsecure"          "../../../../ttssh2/ttxssh")
ico_extract("ttsecure_classic"  "../../../../ttssh2/ttxssh")
ico_extract("ttsecure_flat"     "../../../../ttssh2/ttxssh")
ico_extract("ttsecure_green"    "../../../../ttssh2/ttxssh")
ico_extract("ttsecure_yellow"   "../../../../ttssh2/ttxssh")
