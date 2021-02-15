# cmake -P innosetup.cmake

# unrar
set(UNRAR_ZIP "unrar-3.4.3-bin.zip")
set(UNRAR_URL "https://sourceforge.net/projects/gnuwin32/files/unrar/3.4.3/${UNRAR_ZIP}/download")
set(UNRAR_HASH "b7c3cc0865bfc32cb9e7e2017c91c5d3601eb97b3bcd37f42c36d9c62f1ae46c")
file(MAKE_DIRECTORY "download/unrar")
file(DOWNLOAD
  ${UNRAR_URL}
  download/unrar/${UNRAR_ZIP}
  EXPECTED_HASH SHA256=${UNRAR_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "unrar")
    file(REMOVE_RECURSE "unrar")
endif()
file(MAKE_DIRECTORY "unrar")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xvf ../download/unrar/${UNRAR_ZIP}
  WORKING_DIRECTORY "unrar"
  )

# innounp
#set(INNOUNP_RAR "innounp050.rar")
#set(INNOUNP_URL "https://sourceforge.net/projects/innounp/files/innounp/innounp%200.50/${INNOUNP_RAR}/download")
#set(INNOUNP_HASH "1d8837540ccc15d98245a1c73fd08f404b2a7bdfe7dc9bed2fdece818ff6df67")
set(INNOUNP_RAR "innounp049.rar")
set(INNOUNP_URL "https://sourceforge.net/projects/innounp/files/innounp/innounp%200.49/${INNOUNP_RAR}/download")
set(INNOUNP_HASH "4a636ff9213928a919717d0fab28e525fb992d9bb5f2d4a9ab2fd97132690741")
file(MAKE_DIRECTORY "download/innounp")
file(DOWNLOAD
  ${INNOUNP_URL}
  download/innounp/${INNOUNP_RAR}
  EXPECTED_HASH SHA256=${INNOUNP_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "innounp")
    file(REMOVE_RECURSE "innounp")
endif()
file(MAKE_DIRECTORY "innounp")
execute_process(
  COMMAND ../unrar/bin/unrar.exe x ../download/innounp/${INNOUNP_RAR}
  WORKING_DIRECTORY "innounp"
  )

# innosetup 6
#set(INNOSETUP_EXE "innosetup-6.1.2.exe")
#set(INNOSETUP_HASH "a3ce1c40ef9c71a92691aaff0f413f530c8c9e3c766be481bc63ca7cc74e35e7")
set(INNOSETUP_EXE "innosetup-6.0.5.exe")
set(INNOSETUP_HASH "ae6823b523df87e9441789e51845434e4e0e70aac0b88afe80f94f20f4b98acb")
file(MAKE_DIRECTORY "download/innosetup6")
file(DOWNLOAD
  https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}
  download/innosetup6/${INNOSETUP_EXE}
  EXPECTED_HASH SHA256=${INNOSETUP_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "innosetup6")
    file(REMOVE_RECURSE "innosetup6")
endif()
file(MAKE_DIRECTORY "innosetup6")
execute_process(
  COMMAND ../innounp/innounp.exe -x ../download/innosetup6/${INNOSETUP_EXE}
  WORKING_DIRECTORY "innosetup6"
  )
file(RENAME "innosetup6/{app}" innosetup6/bin)
file(RENAME "innosetup6/{tmp}" innosetup6/tmp)
