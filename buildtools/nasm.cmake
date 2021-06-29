# cmake -P nasm.cmake

set(NASM_BASE "nasm-2.15.05")
set(NASM_ZIP "${NASM_BASE}-win32.zip")
set(NASM_URL "https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win32/${NASM_ZIP}")
set(NASM_HASH "258c7d1076e435511cf2fdf94e2281eadbdb9e3003fd57f356f446e2bce3119e")
file(MAKE_DIRECTORY "download/nasm")
file(DOWNLOAD
  ${NASM_URL}
  download/nasm/${NASM_ZIP}
  EXPECTED_HASH SHA256=${NASM_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "nasm")
    file(REMOVE_RECURSE "nasm")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xvf download/nasm/${NASM_ZIP}
  )
file(RENAME "${NASM_BASE}" "nasm")
