@rem �C���X�g�[��,zip���쐬
@rem   test
@rem     release.bat�����s�A
@rem     7. exec cmd.exe ��I��ł���g�p����ƁA����bat�����e�X�g���ł��܂�
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


rem �|�[�^�u���ł��R�s�[���Ď���Ă���(�����Ɏg�p����)
%CMAKE% -E rm -rf Output/portable/teraterm
%CMAKE% -E copy_directory teraterm Output/portable/teraterm


rem (�����Ȃ�)�C���X�g�[���쐬
if "%RELEASE%" == "1" (
    set INNO_SETUP_OUTPUT="/DOutputBaseFilename=%RELEASE_OUTPUT%"
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION%"
) else (
    set INNO_SETUP_OUTPUT="/DOutputBaseFilename=%SNAPSHOT_OUTPUT%"
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION% %DATE%_%TIME%-%VCSVERSION%"
)
%INNO_SETUP% %INNO_SETUP_APPVERSION% /OOutput %INNO_SETUP_OUTPUT% /DSrcDir=teraterm teraterm.iss


rem (�����Ȃ�)�|�[�^�u���ł�zip���쐬
if "%RELEASE%" == "1" (
    %CMAKE% -E rm -rf %RELEASE_OUTPUT%
    %CMAKE% -E rm -rf %RELEASE_OUTPUT%_pdb
    %CMAKE% -E copy_directory teraterm %RELEASE_OUTPUT%
    %CMAKE% -E copy_directory teraterm_pdb %RELEASE_OUTPUT%_pdb
    %CMAKE% -E tar cf Output/%RELEASE_OUTPUT%.zip --format=zip %RELEASE_OUTPUT%/
    %CMAKE% -E tar cf Output/%RELEASE_OUTPUT%_pdb.zip --format=zip %RELEASE_OUTPUT%_pdb/
) else (
    %CMAKE% -E rm -rf %SNAPSHOT_OUTPUT%
    %CMAKE% -E rm -rf %SNAPSHOT_OUTPUT%_pdb
    %CMAKE% -E copy_directory teraterm %SNAPSHOT_OUTPUT%
    %CMAKE% -E copy_directory teraterm_pdb %SNAPSHOT_OUTPUT%_pdb
    %CMAKE% -E tar cf Output/%SNAPSHOT_OUTPUT%.zip --format=zip %SNAPSHOT_OUTPUT%
    %CMAKE% -E tar cf Output/%SNAPSHOT_OUTPUT%_pdb.zip --format=zip %SNAPSHOT_OUTPUT%_pdb
)

rem hash
if "%RELEASE%" == "1" (
    pushd Output
    %CMAKE% -DOUTPUT=%RELEASE_OUTPUT%.sha256sum -P ../sha256sum.cmake %RELEASE_OUTPUT%.exe %RELEASE_OUTPUT%.zip %RELEASE_OUTPUT%_pdb.zip
    %CMAKE% -DOUTPUT=%RELEASE_OUTPUT%.sha512sum -P ../sha512sum.cmake %RELEASE_OUTPUT%.exe %RELEASE_OUTPUT%.zip %RELEASE_OUTPUT%_pdb.zip
    popd
) else (
    pushd Output
    %CMAKE% -DOUTPUT=%SNAPSHOT_OUTPUT%.sha256sum -P ../sha256sum.cmake %SNAPSHOT_OUTPUT%.exe %SNAPSHOT_OUTPUT%.zip %SNAPSHOT_OUTPUT%_pdb.zip
    %CMAKE% -DOUTPUT=%SNAPSHOT_OUTPUT%.sha512sum -P ../sha512sum.cmake %SNAPSHOT_OUTPUT%.exe %SNAPSHOT_OUTPUT%.zip %SNAPSHOT_OUTPUT%_pdb.zip
    popd
)

endlocal
