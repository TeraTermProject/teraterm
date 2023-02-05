@rem インストーラ,zipを作成
@rem   test
@rem     release.batを実行、
@rem     7. exec cmd.exe を選んでから使用すると、このbatだけテストができます
setlocal
cd /d %~dp0

if not exist Output mkdir Output
set TT_VERSION=%VERSION%
set SNAPSHOT_PORTABLE_OUTPUT="teraterm-%TT_VERSION%-r%SVNVERSION%-%DATE%_%TIME%-%USERNAME%-snapshot"
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
    set INNO_SETUP_OPT_VERSION="/DVerSubStr=r%SVNVERSION%-%DATE%_%TIME%"
    set INNO_SETUP_OPT_OUTPUT="/DOutputSubStr=r%SVNVERSION%-%DATE%_%TIME%-%USERNAME%-snapshot"
)
%INNO_SETUP% %INNO_SETUP_OPT_VERSION% /DAppVer=%VERSION% %INNO_SETUP_OPT_OUTPUT% teraterm.iss

endlocal
