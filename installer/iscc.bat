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
if "%RELEASE%" == "1" (
    set OUTPUT=%RELEASE_OUTPUT%
) else (
    set OUTPUT=%SNAPSHOT_OUTPUT%
)

rem �|�[�^�u���ł��R�s�[���Ď���Ă���(�����Ɏg�p����)
%CMAKE% -E rm -rf Output/portable/teraterm
%CMAKE% -E copy_directory teraterm Output/portable/teraterm


rem (�����Ȃ�)�C���X�g�[���쐬
set INNO_SETUP_OUTPUT="/DOutputBaseFilename=%OUTPUT%"
if "%RELEASE%" == "1" (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION%"
) else (
    set INNO_SETUP_APPVERSION="/DAppVersion=%VERSION% %DATE%_%TIME%-%VCSVERSION%"
)
%INNO_SETUP% %INNO_SETUP_APPVERSION% /OOutput %INNO_SETUP_OUTPUT% /DSrcDir=teraterm teraterm.iss

rem (�����Ȃ�)�|�[�^�u���ł�zip���쐬
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
