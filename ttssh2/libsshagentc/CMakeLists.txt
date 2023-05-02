cmake_minimum_required(VERSION 3.11)
project(libsshagentc)

option(SSHAGENTC_SKELTON "use skelton" OFF)

if(SSHAGENTC_SKELTON)
  add_subdirectory(skelton)
else()
  add_subdirectory(putty)
endif()

# for debug/test
#add_subdirectory(sshagentc_tool)
