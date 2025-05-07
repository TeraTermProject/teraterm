@rem インストーラ,zipを作成
@rem   test
@rem     release.batを実行、
@rem     7. exec cmd.exe を選んでから使用すると、このbatだけテストができます
setlocal
cd /d %~dp0

if not exist Output mkdir Output
set TT_VERSION=%VERSION%
SET VCSVERSION=%GITVERSION%
IF NOT "%SVNVERSION%" == "unknown" (
    SET VCSVERSION=r%SVNVERSION%
)
set SNAPSHOT_PORTABLE_OUTPUT="teraterm-%TT_VERSION%-%DATE%_%TIME%-%VCSVERSION%-%USERNAME%-snapshot"
if "%RELEASE%" == "1" (
    pushd Output
    %CMAKE% -E tar cf teraterm-%TT_VERSION%.zip --format=zip teraterm-%TT_VERSION%/
    %CMAKE% -E tar cf teraterm-%TT_VERSION%_pdb.zip --format=zip teraterm-%TT_VERSION%_pdb/
    popd
    set INNO_SETUP_OPT_VERSION=
    set INNO_SETUP_OPT_OUTPUT=
) else (
    %CMAKE% -E rename snapshot-%DATE%_%TIME% %SNAPSHOT_PORTABLE_OUTPUT%
    %CMAKE% -E rename snapshot-%DATE%_%TIME%_pdb %SNAPSHOT_PORTABLE_OUTPUT%_pdb
    %CMAKE% -E tar cf Output/%SNAPSHOT_PORTABLE_OUTPUT%.zip --format=zip %SNAPSHOT_PORTABLE_OUTPUT%
    %CMAKE% -E tar cf Output/%SNAPSHOT_PORTABLE_OUTPUT%_pdb.zip --format=zip %SNAPSHOT_PORTABLE_OUTPUT%_pdb
    %CMAKE% -E rename %SNAPSHOT_PORTABLE_OUTPUT% snapshot-%DATE%_%TIME%
    %CMAKE% -E rename %SNAPSHOT_PORTABLE_OUTPUT%_pdb snapshot-%DATE%_%TIME%_pdb
    set INNO_SETUP_OPT_VERSION="/DVerSubStr=%DATE%_%TIME%"-%VCSVERSION%
    set INNO_SETUP_OPT_OUTPUT="/DOutputSubStr=%DATE%_%TIME%-%VCSVERSION%-%USERNAME%-snapshot"
)
%INNO_SETUP% %INNO_SETUP_OPT_VERSION% /DAppVer=%VERSION% %INNO_SETUP_OPT_OUTPUT% teraterm.iss

if "%RELEASE%" == "1" (
    pushd Output
    %CMAKE% -DOUTPUT=teraterm-%TT_VERSION%.sha256sum -P ../sha256sum.cmake teraterm-%TT_VERSION%.exe teraterm-%TT_VERSION%.zip teraterm-%TT_VERSION%_pdb.zip
    %CMAKE% -DOUTPUT=teraterm-%TT_VERSION%.sha512sum -P ../sha512sum.cmake teraterm-%TT_VERSION%.exe teraterm-%TT_VERSION%.zip teraterm-%TT_VERSION%_pdb.zip
    popd
) else (
    pushd Output
    %CMAKE% -DOUTPUT=%SNAPSHOT_PORTABLE_OUTPUT%.sha256sum -P ../sha256sum.cmake %SNAPSHOT_PORTABLE_OUTPUT%.exe %SNAPSHOT_PORTABLE_OUTPUT%.zip %SNAPSHOT_PORTABLE_OUTPUT%_pdb.zip
    %CMAKE% -DOUTPUT=%SNAPSHOT_PORTABLE_OUTPUT%.sha512sum -P ../sha512sum.cmake %SNAPSHOT_PORTABLE_OUTPUT%.exe %SNAPSHOT_PORTABLE_OUTPUT%.zip %SNAPSHOT_PORTABLE_OUTPUT%_pdb.zip
    popd
)

endlocal
