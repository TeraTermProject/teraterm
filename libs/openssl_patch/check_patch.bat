@echo off

set folder=openssl_patch
set cmdopt2=--binary --backup -p0
set cmdopt1=--dry-run %cmdopt2%

rem
echo OpenSSL 1.1.1にパッチが適用されているかを確認します...
echo.
rem

rem パッチコマンドの存在チェック
set patchcmd="patch.exe"
if exist %patchcmd% (goto cmd_true) else goto cmd_false

:cmd_true


rem パッチの適用有無をチェック

rem freeaddrinfo/getnameinfo/getaddrinfo API依存除去のため
:patch1
findstr /c:"# undef AI_PASSIVE" ..\openssl\crypto\bio\bio_lcl.h
if ERRORLEVEL 1 goto fail1
goto patch3
:fail1
pushd ..
%folder%\patch %cmdopt1% < %folder%\ws2_32_dll_patch.txt
%folder%\patch %cmdopt2% < %folder%\ws2_32_dll_patch.txt
popd

rem InitializeCriticalSectionAndSpinCount API依存除去のため
rem 以下は不要
:patch2
findstr /c:"running on Windows95" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail2
goto patch3
:fail2
pushd ..
%folder%\patch %cmdopt1% < %folder%\InitializeCriticalSectionAndSpinCount_patch.txt
%folder%\patch %cmdopt2% < %folder%\InitializeCriticalSectionAndSpinCount_patch.txt
popd

rem InitializeCriticalSectionAndSpinCount/InterlockedCompareExchange/InterlockedExchangeAdd API依存除去のため
:patch3
findstr /c:"myInitializeCriticalSectionAndSpinCount" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail3
goto patch4
:fail3
pushd ..
%folder%\patch %cmdopt1% < %folder%\thread_win.txt
%folder%\patch %cmdopt2% < %folder%\thread_win.txt
popd


rem CryptAcquireContextW API依存除去のため
:patch4
findstr /c:"CryptAcquireContextA" ..\openssl\crypto\rand\rand_win.c
if ERRORLEVEL 1 goto fail4
goto patch5
:fail4
pushd ..
%folder%\patch %cmdopt1% < %folder%\CryptAcquireContextW.txt
%folder%\patch %cmdopt2% < %folder%\CryptAcquireContextW.txt
popd


:patch5


:patch_end
echo "パッチは適用されています"
timeout 5
goto end

:patchfail
echo "パッチが適用されていないようです"
set /P ANS="続行しますか？(y/n)"
if "%ANS%"=="y" (
  goto end
) else if "%ANS%"=="n" (
  echo "バッチファイルを終了します"
  exit /b
) else (
  goto fail
)

goto end

:cmd_false
echo パッチコマンド %patchcmd% が見つかりません
echo 下記サイトからダウンロードしてください
echo http://geoffair.net/projects/patch.htm
echo.
goto patchfail

:end
@echo on


