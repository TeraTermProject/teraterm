# cmake -DCOPY_BMP_ICO=1 -P icon_combine.cmake

option(COPY_ICO "copy ico file to source folder" 0)
option(COPY_BMP_ICO "copy BMP ico file to source folder" 0)

#set(ICOTOOL "icotool")
find_program(
  ICOTOOL icotool
  HINTS ${CMAKE_CURRENT_LIST_DIR}/../../../../buildtools/cygwin64
  HINTS c:/cygwin64/bin
)
message("icotool=${ICOTOOL}")
message("COPY_ICO=${COPY_ICO}")
message("COPY_BMP_ICO=${COPY_BMP_ICO}")

# icoファイルを作成する
#
#   basename    icoファイルの拡張子をとったファイル名(フォルダ名)
#   PNG_ICO     0/1=pngを含まない/含んだicoファイル
#               256x256のpngファイルは png のまま icoファイルにする
#   ICO         icoファイル名 (basename フォルダ内に作成する)
#   EXIST_256   0/1 = 256x256のpngファイルが存在しなかった/した
#
function(ico_combine_raw basename PNG_ICO ICO EXIST_256)
  set(DEST ${basename})
  set(${EXIST_256} 0 PARENT_SCOPE)

  file(GLOB imgs
    RELATIVE "${CMAKE_CURRENT_LIST_DIR}/${basename}"
    "${basename}/*.png")

  unset(ARGS)
  foreach(f IN LISTS imgs)
    if(${f} MATCHES "256x256")
      set(${EXIST_256} 1 PARENT_SCOPE)
      if(${PNG_ICO})
        list(APPEND ARGS "-r")
      endif()
    endif()
    list(APPEND ARGS ${f})
  endforeach()

  #message("${ICOTOOL} -c ${ARGS}")
  execute_process(
    COMMAND ${ICOTOOL} -c ${ARGS}
    OUTPUT_FILE ${ICO}
    WORKING_DIRECTORY ${DEST}
  )

  return(${PNG})
endfunction()

# icoファイルを作成, 必要なら上書きする
#
#   basename    icoファイルの拡張子をとったファイル名
#   rel_path    icoファイルのあるフォルダ
#
function(ico_combine basename rel_path)
  set(ICO "${basename}.ico")
  ico_combine_raw(${basename} 1 ${ICO} EXIST_256)
  if(${COPY_ICO})
    file(COPY_FILE ${basename}/${ICO} ${rel_path}/${ICO})
  endif()

  set(ICO "${basename}_bmp.ico")
  ico_combine_raw(${basename} 0 ${ICO} EXIST_256)
  if(${COPY_BMP_ICO} AND ${EXIST_256})
    message("copy ${basename}/${ICO} ${rel_path}/${ICO}")
    file(COPY_FILE ${basename}/${ICO} ${rel_path}/${ICO})
  endif()
endfunction()

ico_combine("teraterm"          "../..")
ico_combine("teraterm_3d"       "../..")
ico_combine("teraterm_classic"  "../..")
ico_combine("teraterm_flat"     "../..")
ico_combine("tterm16"           "../..")
ico_combine("tek"               "../..")
ico_combine("tek16"             "../..")
ico_combine("cygterm"           "../../../../cygwin/cygterm")
ico_combine("vt"                "../..")
ico_combine("vt_3d"             "../..")
ico_combine("vt_classic"        "../..")
ico_combine("vt_flat"           "../..")
ico_combine("vt16"              "../..")
ico_combine("ttmacr16"          "../../../ttpmacro")
ico_combine("ttmacro"           "../../../ttpmacro")
ico_combine("ttmacrof"          "../../../ttpmacro")
ico_combine("ttmacro_3d"        "../../../ttpmacro")
ico_combine("ttmacro_flat"      "../../../ttpmacro")
ico_combine("ttsecure"          "../../../../ttssh2/ttxssh")
ico_combine("ttsecure_classic"  "../../../../ttssh2/ttxssh")
ico_combine("ttsecure_flat"     "../../../../ttssh2/ttxssh")
ico_combine("ttsecure_green"    "../../../../ttssh2/ttxssh")
ico_combine("ttsecure_yellow"   "../../../../ttssh2/ttxssh")
