cmake_minimum_required(VERSION 3.11)
project(libsshagentc)

# どの libsshagentc を使用するか
#  すべて OFF の時、従来と同じ putty を使用する
option(SSHAGENTC_SKELTON "use skelton" OFF)
option(SSHAGENTC_NEXT "use next sshagent" OFF)

option(SSHAGENTC_TOOL "sshagentc test/debug tool" OFF)

if(SSHAGENTC_NEXT)
  add_subdirectory(sshagentc)
elseif(SSHAGENTC_SKELTON)
  add_subdirectory(skelton)
else()
  add_subdirectory(putty)
endif()

# for debug/test
if(SSHAGENTC_TOOL)
  add_subdirectory(sshagentc_tool)
endif()
