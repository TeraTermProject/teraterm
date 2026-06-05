@rem インストーラ,zipを作成
@rem   test
@rem     release.batを実行、
@rem     7. exec cmd.exe を選んでから使用すると、このbatをテストできます
setlocal


rem arch for package
if "%arch%" == "" (set arch=x86)


rem Change the working directory to the location of this BAT file
cd /d %~dp0


if not exist Output mkdir Output
if not exist Output\portable mkdir Output\portable

SET VCSVERSION=%GITVERSION%
IF NOT "%SVNVERSION%" == "unknown" (
    SET VCSVERSION=r%SVNVERSION%
)

set SNAPSHOT_OUTPUT=teraterm-%VERSION%-%arch%-%DATE%_%TIME%-%VCSVERSION%-%USERNAME%-snapshot
set RELEASE_OUTPUT=teraterm-%VERSION%-%arch%
if "%RELEASE%" == "1" (
    set OUTPUT=%RELEASE_OUTPUT%
) else (
    set OUTPUT=%SNAPSHOT_OUTPUT%
)

rem ポータブルでコピーして使って使用(テストに使用)
%CMAKE% -E rm -rf Output/portable/teraterm-%arch%
%CMAKE% -E copy_directory Output/build/teraterm-%arch% Output/portable/teraterm-%arch%
if ERRORLEVEL 1 (
    echo ERROR copy_directory Output/build/teraterm-%arch%
    endlocal
    exit /b 1
)


rem (オプション)インストーラ作成
set INNO_SETUP_OUTPUT="/DOutputBaseFilename=%OUTPUT%"
if "%RELEASE%" == "1" (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION%"
) else (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION% %DATE%_%TIME%-%VCSVERSION%"
)
set INNO_SETUP_ARCH="/DArch=x86"
if "%arch%" == "x64" (set INNO_SETUP_ARCH=/DArch=%arch%)
if "%arch%" == "arm64" (set INNO_SETUP_ARCH=/DArch=%arch%)
%INNO_SETUP% %INNO_SETUP_APPVERSION% /OOutput %INNO_SETUP_OUTPUT% /DSrcDir=Output\build\teraterm-%arch% %INNO_SETUP_ARCH% teraterm.iss
if ERRORLEVEL 1 (
    echo ERROR %INNO_SETUP%
    endlocal
    exit /b 1
)

rem (オプション)ポータブル版zip作成
pushd Output
%CMAKE% -E rm -rf %OUTPUT%
%CMAKE% -E rm -rf %OUTPUT%_pdb
%CMAKE% -E copy_directory build\teraterm-%arch% %OUTPUT%
if ERRORLEVEL 1 (
    echo ERROR copy_directory build\teraterm-%arch%
    popd
    endlocal
    exit /b 1
)
%CMAKE% -E copy_directory build\teraterm-%arch%_pdb %OUTPUT%_pdb
if ERRORLEVEL 1 (
    echo ERROR copy_directory build\teraterm-%arch%_pdb
    popd
    endlocal
    exit /b 1
)
%CMAKE% -E tar cf %OUTPUT%.zip --format=zip %OUTPUT%/
if ERRORLEVEL 1 (
    echo ERROR tar %OUTPUT%.zip
    popd
    endlocal
    exit /b 1
)
%CMAKE% -E tar cf %OUTPUT%_pdb.zip --format=zip %OUTPUT%_pdb/
if ERRORLEVEL 1 (
    echo ERROR tar %OUTPUT%_pdb.zip
    popd
    endlocal
    exit /b 1
)
popd

rem ハッシュ作成
pushd Output
%CMAKE% -E sha256sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha256sum
if ERRORLEVEL 1 (
    echo ERROR sha256sum
    popd
    endlocal
    exit /b 1
)
%CMAKE% -E sha512sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha512sum
if ERRORLEVEL 1 (
    echo ERROR sha512sum
    popd
    endlocal
    exit /b 1
)
popd

endlocal
exit /b 0
