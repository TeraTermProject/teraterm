@rem インストーラ,zipを作成
@rem   test
@rem     release.batを実行、
@rem     7. exec cmd.exe を選んでから使用すると、このbatだけテストができます
setlocal
cd /d %~dp0

if not exist Output mkdir Output
if not exist Output\portable mkdir Output\portable
SET VCSVERSION=%GITVERSION%
IF NOT "%SVNVERSION%" == "unknown" (
    SET VCSVERSION=r%SVNVERSION%
)

set SNAPSHOT_OUTPUT="teraterm-%VERSION%-%DATE%_%TIME%-%VCSVERSION%-%USERNAME%-snapshot"
set RELEASE_OUTPUT="teraterm-%VERSION%"
if "%RELEASE%" == "1" (
    set OUTPUT=%RELEASE_OUTPUT%
) else (
    set OUTPUT=%SNAPSHOT_OUTPUT%
)

rem ポータブル版をコピーして取っておく(署名に使用する)
%CMAKE% -E rm -rf Output/portable/teraterm
%CMAKE% -E copy_directory teraterm Output/portable/teraterm


rem (署名なし)インストーラ作成
set INNO_SETUP_OUTPUT="/DOutputBaseFilename=%OUTPUT%"
if "%RELEASE%" == "1" (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION%"
) else (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION% %DATE%_%TIME%-%VCSVERSION%"
)
%INNO_SETUP% %INNO_SETUP_APPVERSION% /OOutput %INNO_SETUP_OUTPUT% /DSrcDir=teraterm teraterm.iss

rem (署名なし)ポータブル版のzipを作成
%CMAKE% -E rm -rf %OUTPUT%
%CMAKE% -E rm -rf %OUTPUT%_pdb
%CMAKE% -E copy_directory teraterm %OUTPUT%
%CMAKE% -E copy_directory teraterm_pdb %OUTPUT%_pdb
%CMAKE% -E tar cf Output/%OUTPUT%.zip --format=zip %OUTPUT%/
%CMAKE% -E tar cf Output/%OUTPUT%_pdb.zip --format=zip %OUTPUT%_pdb/

rem hash
pushd Output
%CMAKE% -E sha256sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha256sum
%CMAKE% -E sha512sum %OUTPUT%.exe %OUTPUT%.zip %OUTPUT%_pdb.zip > %OUTPUT%.sha512sum
popd

endlocal
