rem Build SFMT

pushd SFMT


SET filename=SFMT_version_for_teraterm.h


rem Generate Makefile
SET SFMT_MAKEFILE=Makefile.msc.Win32.debug
SET SFMT_OUTDIR=build\Win32\Debug

if exist "%SFMT_MAKEFILE%" goto end_Win32_debug_gen
echo CFLAGS = /MTd /Od /nologo> %SFMT_MAKEFILE%
echo;>> %SFMT_MAKEFILE%
echo %SFMT_OUTDIR%\SFMTd.lib: SFMT.c>> %SFMT_MAKEFILE%
echo 	cl $(CFLAGS) /Fo%SFMT_OUTDIR%\SFMT.obj /c SFMT.c>> %SFMT_MAKEFILE%
echo 	lib /out:%SFMT_OUTDIR%\SFMTd.lib %SFMT_OUTDIR%\SFMT.obj>> %SFMT_MAKEFILE%

:end_Win32_debug_gen

IF NOT EXIST %SFMT_OUTDIR% MKDIR %SFMT_OUTDIR%
nmake /f Makefile.msc.Win32.debug


SET SFMT_MAKEFILE=Makefile.msc.Win32.release
SET SFMT_OUTDIR=build\Win32\Release

if exist "%SFMT_MAKEFILE%" goto end_Win32_release_gen
echo CFLAGS = /MT /O2 /nologo> %SFMT_MAKEFILE%
echo;>> %SFMT_MAKEFILE%
echo %SFMT_OUTDIR%\SFMT.lib: SFMT.c>> %SFMT_MAKEFILE%
echo 	cl $(CFLAGS) /Fo%SFMT_OUTDIR%\SFMT.obj /c SFMT.c>> %SFMT_MAKEFILE%
echo 	lib /out:%SFMT_OUTDIR%\SFMT.lib %SFMT_OUTDIR%\SFMT.obj>> %SFMT_MAKEFILE%

:end_Win32_release_gen

IF NOT EXIST %SFMT_OUTDIR% MKDIR %SFMT_OUTDIR%
nmake /f Makefile.msc.Win32.release


rem Create version file
IF EXIST %filename% (GOTO FILE_TRUE) ELSE GOTO FILE_FALSE
:FILE_TRUE
ECHO "Found file: %filename%"
GOTO END

:FILE_FALSE
ECHO "%filename% file was not fould. Generating it."
echo #define SFMT_VERSION "Unknown"> %filename%
GOTO END

:END
popd
