
cmake_minimum_required(VERSION 3.11.4)

project(comportinfo)

add_executable(
  comportinfo
  main.cpp
  ../comportinfo.cpp
  ../comportinfo.h
  )

target_include_directories(
  comportinfo
  PUBLIC
  ..
  )

if(MSVC)
  target_compile_options(
	comportinfo
	PRIVATE
	/W4
	$<$<CONFIG:Release>:/MT>
	$<$<CONFIG:Debug>:/MTd /ZI /D_DEBUG>
	)

  if(${CMAKE_VERSION} VERSION_GREATER "3.11.4")
	target_link_options(
	  comportinfo
	  PRIVATE
	  $<$<CONFIG:Debug>:/SAFESEH:NO>
	)
  endif()

elseif(MINGW)

endif()

target_link_libraries(
  comportinfo
  PRIVATE
  Setupapi.lib
  )
