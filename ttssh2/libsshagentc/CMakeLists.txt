cmake_minimum_required(VERSION 3.11)
project(libsshagentc)

# どの libsshagentc を使用するか
option(SSHAGENTC_SKELTON "use skelton" OFF)

option(SSHAGENTC_TOOL "sshagentc test/debug tool" OFF)

if(SSHAGENTC_SKELTON)
  add_subdirectory(skelton)
else()
  add_subdirectory(sshagentc)
endif()

# for debug/test
if(SSHAGENTC_TOOL)
  add_subdirectory(sshagentc_tool)
endif()
