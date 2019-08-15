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
goto patch4
:fail1
pushd ..
%folder%\patch %cmdopt1% < %folder%\ws2_32_dll_patch.txt
%folder%\patch %cmdopt2% < %folder%\ws2_32_dll_patch.txt
popd


rem CryptAcquireContextW API依存除去のため
:patch4
rem findstr /c:"add_RAND_buffer" ..\openssl\crypto\rand\rand_win.c
rem if ERRORLEVEL 1 goto fail4
rem goto patch5
rem :fail4
rem pushd ..
rem %folder%\patch %cmdopt1% < %folder%\CryptAcquireContextW.txt
rem %folder%\patch %cmdopt2% < %folder%\CryptAcquireContextW.txt
rem popd


rem WindowsMeでRAND_bytesで落ちる現象回避のため。
:patch5
findstr /c:"added if meth is NULL pointer" ..\openssl\crypto\rand\rand_lib.c
if ERRORLEVEL 1 goto fail5
goto patch6
:fail5
pushd ..
%folder%\patch %cmdopt1% < %folder%\RAND_bytes.txt
%folder%\patch %cmdopt2% < %folder%\RAND_bytes.txt
popd


rem WindowsMeでInitializeCriticalSectionAndSpinCountがエラーとなる現象回避のため。
:patch6
findstr /c:"myInitializeCriticalSectionAndSpinCount" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail6
goto patch7
:fail6
pushd ..
%folder%\patch %cmdopt1% < %folder%\atomic_api.txt
%folder%\patch %cmdopt2% < %folder%\atomic_api.txt
popd


:patch7


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


