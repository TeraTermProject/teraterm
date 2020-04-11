
add_custom_target(
  svnversion_h ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../teraterm/ttpdlg/svnversion.h
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/svnrev.pl
  COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/../teraterm/ttpdlg/svnversion.h"
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../teraterm/ttpdlg/svnversion.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/../teraterm/ttpdlg/
  COMMAND ${PERL} svnrev.pl --svn ${Subversion_SVN_EXECUTABLE} --git ${GIT_EXECUTABLE} --root ${CMAKE_SOURCE_DIR} --header ${CMAKE_CURRENT_BINARY_DIR}/../teraterm/ttpdlg/svnversion.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

####
# ソースツリー内に生成ファイルが存在していたら削除する
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../teraterm/ttpdlg/svnversion.h")
  file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/../teraterm/ttpdlg/svnversion.h")
endif()
