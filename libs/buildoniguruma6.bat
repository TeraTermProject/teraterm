pushd oniguruma\src

SET ONIG_DIR=.
SET BUILD_DIR=.

if not exist "%ONIG_DIR%\Makefile" goto mkmf
for %%F in (Makefile) do set mftime=%%~tF
for %%F in (..\..\buildoniguruma6.bat) do set battime=%%~tF
if "%battime%" leq "%mftime%" goto build

del %ONIG_DIR%\onig_sd.lib
nmake -f %ONIG_DIR%\Makefile clean

:mkmf
copy %ONIG_DIR%\config.h.win32 %ONIG_DIR%\config.h
perl -e "open(IN,'%ONIG_DIR%\Makefile.windows');while(<IN>){s|CFLAGS =|CFLAGS = /MT|;print $_;}close(IN);" > %ONIG_DIR%\Makefile
perl -e "open(IN,'%ONIG_DIR%\Makefile.windows');while(<IN>){s|CFLAGS = -O2|CFLAGS = /MTd -Od|;s|_s.lib|_sd.lib|;print $_;}close(IN);" > %ONIG_DIR%\Makefile.debug

:build
if exist "%ONIG_DIR%\onig_sd.lib" goto build_release
nmake -f %ONIG_DIR%\Makefile.debug clean
nmake -f %ONIG_DIR%\Makefile.debug
move %ONIG_DIR%\onig_sd.lib %ONIG_DIR%\..\sample\
nmake -f %ONIG_DIR%\Makefile.debug clean
move %ONIG_DIR%\..\sample\onig_sd.lib %ONIG_DIR%

:build_release
if exist "%ONIG_DIR%\onig_s.lib" goto end
nmake -f %ONIG_DIR%\Makefile

:end
popd
