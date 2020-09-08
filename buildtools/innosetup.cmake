# cmake -P innosetup.cmake

# unrar
file(MAKE_DIRECTORY "download/unrar")
file(DOWNLOAD
  http://gnuwin32.sourceforge.net/downlinks/unrar-bin-zip.php
  download/unrar/unrar-3.4.3-bin.zip
  EXPECTED_HASH SHA256=b7c3cc0865bfc32cb9e7e2017c91c5d3601eb97b3bcd37f42c36d9c62f1ae46c
  SHOW_PROGRESS
  )
file(MAKE_DIRECTORY "unrar")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xvf ../download/unrar/unrar-3.4.3-bin.zip
  WORKING_DIRECTORY "unrar"
  )

# innounp
file(MAKE_DIRECTORY "download/innounp")
file(DOWNLOAD
  https://sourceforge.net/projects/innounp/files/innounp/innounp%200.49/innounp049.rar/download
  download/innounp/innounp049.rar
  EXPECTED_HASH SHA256=4a636ff9213928a919717d0fab28e525fb992d9bb5f2d4a9ab2fd97132690741
  SHOW_PROGRESS
  )
file(MAKE_DIRECTORY "innounp")
execute_process(
  COMMAND ../unrar/bin/unrar.exe x ../download/innounp/innounp049.rar
  WORKING_DIRECTORY "innounp"
  )

# innosetup 6
file(MAKE_DIRECTORY "download/innosetup6")
file(DOWNLOAD
  https://jrsoftware.org/download.php/is.exe
  download/innosetup6/innosetup-6.0.5.exe
  EXPECTED_HASH SHA256=ae6823b523df87e9441789e51845434e4e0e70aac0b88afe80f94f20f4b98acb
  SHOW_PROGRESS
  )
file(MAKE_DIRECTORY "innosetup6")
execute_process(
  COMMAND ../innounp/innounp.exe -x ../download/innosetup6/innosetup-6.0.5.exe
  WORKING_DIRECTORY "innosetup6"
  )
file(RENAME "innosetup6/{app}" innosetup6/bin)
file(RENAME "innosetup6/{tmp}" innosetup6/tmp)
