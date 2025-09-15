@rem インストーラ,zipを作成
@rem   test
@rem     release.batを実行、
@rem     7. exec cmd.exe を選んでから使用すると、このbatだけテストができます
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

rem ポータブル版をコピーして取っておく(署名に使用する)
%CMAKE% -E rm -rf Output/portable/teraterm-%arch%
%CMAKE% -E copy_directory Output/build/teraterm-%arch% Output/portable/teraterm-%arch%


rem (署名なし)インストーラ作成
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

rem (署名なし)ポータブル版のzipを作成
pushd Output
%CMAKE% -E rm -rf %OUTPUT%
%CMAKE% -E rm -rf %OUTPUT%_pdb
%CMAKE% -E copy_directory build\teraterm-%arch% %OUTPUT%
%CMAKE% -E copy_directory build\teraterm-%arch%_pdb %OUTPUT%_pdb
%CMAKE% -E tar cf %OUTPUT%.zip --format=zip %OUTPUT%/
%CMAKE% -E tar cf %OUTPUT%_pdb.zip --format=zip %OUTPUT%_pdb/
popd

rem ハッシュを作成
pushd Output
%CMAKE% -E sha256sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha256sum
%CMAKE% -E sha512sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha512sum
popd

endlocal
