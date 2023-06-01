setlocal
cd /d %~dp0
call ci_scripts\find_cmake.bat
"%CMAKE_COMMAND%" -P gtags_update.cmake
rem see doc_internal/readme.md
