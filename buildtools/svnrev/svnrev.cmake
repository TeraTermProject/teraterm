# input
#   PERL
#   GIT_EXECUTABLE
#   SVN_EXECUTABLE
#   SOURCE_DIR (CMAKE_SOURCE_DIR), ソースツリーのルート
#   BINARY_DIR (CMAKE_BINARY_DIR), バイナリツリーのルート

set(CMAKE_SOURCE_DIR ${SOURCE_DIR})
set(CMAKE_BINARY_DIR ${BINARY_DIR})
set(SVNREV_PL ${CMAKE_CURRENT_LIST_DIR}/svnrev.pl)
set(SVNVERSION_H ${CMAKE_BINARY_DIR}/teraterm/common/svnversion.h)
set(BUILD_CONFIG ${CMAKE_BINARY_DIR}/build_config.cmake)
set(SOURCETREEINFO ${CMAKE_CURRENT_LIST_DIR}/sourcetree_info.bat)
set(BUILD_CONFIG_ISL ${SOURCE_DIR}/installer/build_config.isl)
if(NOT (DEFINED ARCHITECTURE))
  set(ARCHITECTURE "Win32")
endif()

unset(ARGS)
if((DEFINED SVN_EXECUTABLE) AND (DEFINED ${SVN_EXECUTABLE}))
  list(APPEND ARGS "--svn" "${SVN_EXECUTABLE}")
endif()
if((DEFINED GIT_EXECUTABLE) AND (DEFINED ${GIT_EXECUTABLE}))
  list(APPEND ARGS "--git" "${GIT_EXECUTABLE}")
endif()
if((${SVNREV_PL} IS_NEWER_THAN ${SVNVERSION_H}) OR
    (${SVNREV_PL} IS_NEWER_THAN ${BUILD_CONFIG}))
  # 出力ファイルが古い(or存在しない)時は上書き指定
  list(APPEND ARGS "--overwrite")
endif()

if(0)
  message("PERL=${PERL}")
  message("GIT_EXECUTABLE=${GIT_EXECUTABLE}")
  message("SVN_EXECUTABLE=${SVN_EXECUTABLE}")
  message("SOURCE_DIR=${SOURCE_DIR}")
  message("BINARY_DIR=${BINARY_DIR}")
  message("CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
  message("CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
  message("ARGS=${ARGS}")
  message("SVNREV_PL=${SVNREV_PL}")
  message("SVNVERSION_H=${SVNVERSION_H}")
  message("BUILD_CONFIG=${BUILD_CONFIG}")
  message("SOURCETREEINFO=${SOURCETREEINFO}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E echo ${PERL} ${SVNREV_PL} ${ARGS}
  --root ${CMAKE_SOURCE_DIR}
  --header ${SVNVERSION_H}
  --cmake ${BUILD_CONFIG}
  --bat ${SOURCETREEINFO}
  --isl ${BUILD_CONFIG_ISL}
  --architecture ${ARCHITECTURE}
  #--verbose
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  RESULT_VARIABLE rv
)
execute_process(
  COMMAND ${PERL} ${SVNREV_PL} ${ARGS}
  --root ${CMAKE_SOURCE_DIR}
  --header ${SVNVERSION_H}
  --cmake ${BUILD_CONFIG}
  --bat ${SOURCETREEINFO}
  --isl ${BUILD_CONFIG_ISL}
  --architecture ${ARCHITECTURE}
  #--verbose
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  RESULT_VARIABLE rv
)
