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

:patch1
rem freeaddrinfo/getnameinfo/getaddrinfo API(WindowsXP以降)依存除去のため
findstr /c:"# undef AI_PASSIVE" ..\openssl\crypto\bio\bio_lcl.h
if ERRORLEVEL 1 goto fail1
goto patch2
:fail1
pushd ..
%folder%\patch %cmdopt1% < %folder%\ws2_32_dll_patch.txt
%folder%\patch %cmdopt2% < %folder%\ws2_32_dll_patch.txt
popd

:patch2
:patch3
:patch4


:patch5
rem WindowsMeでRAND_bytesで落ちる現象回避のため。
rem OpenSSL 1.0.2ではmethのNULLチェックがあったが、OpenSSL 1.1.1でなくなっている。
rem このNULLチェックはなくても問題はなく、本質はInitializeCriticalSectionAndSpinCountにあるため、
rem デフォルトでは適用しないものとする。
rem findstr /c:"added if meth is NULL pointer" ..\openssl\crypto\rand\rand_lib.c
rem if ERRORLEVEL 1 goto fail5
rem goto patch6
rem :fail5
rem pushd ..
rem %folder%\patch %cmdopt1% < %folder%\RAND_bytes.txt
rem %folder%\patch %cmdopt2% < %folder%\RAND_bytes.txt
rem popd


:patch6
rem WindowsMeでInitializeCriticalSectionAndSpinCountがエラーとなる現象回避のため。
findstr /c:"myInitializeCriticalSectionAndSpinCount" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail6
goto patch7
:fail6
pushd ..
%folder%\patch %cmdopt1% < %folder%\atomic_api.txt
%folder%\patch %cmdopt2% < %folder%\atomic_api.txt
popd


:patch7
rem Windows98/Me/NT4.0ではCryptAcquireContextWによるエントロピー取得が
rem できないため、新しく処理を追加する。CryptAcquireContextWの利用は残す。
findstr /c:"CryptAcquireContextA" ..\openssl\crypto\rand\rand_win.c
if ERRORLEVEL 1 goto fail7
goto patch8
:fail7
pushd ..
%folder%\patch %cmdopt1% < %folder%\CryptAcquireContextW2.txt
%folder%\patch %cmdopt2% < %folder%\CryptAcquireContextW2.txt
popd


:patch8
rem Windows95では InterlockedCompareExchange と InterlockedCompareExchange が
rem 未サポートのため、別の処理で置き換える。
rem InitializeCriticalSectionAndSpinCount も未サポートだが、WindowsMe向けの
rem 処置に含まれる。
findstr /c:"INTERLOCKEDCOMPAREEXCHANGE" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail8
goto patch9
:fail8
pushd ..
copy /b openssl\crypto\threads_win.c.orig openssl\crypto\threads_win.c.orig2
%folder%\patch %cmdopt1% < %folder%\atomic_api_win95.txt
%folder%\patch %cmdopt2% < %folder%\atomic_api_win95.txt
popd


rem Windows95では CryptAcquireContextW が未サポートのため、エラーで返すようにする。
rem エラー後は CryptAcquireContextA を使う。
:patch9
findstr /c:"myCryptAcquireContextW" ..\openssl\crypto\rand\rand_win.c
if ERRORLEVEL 1 goto fail9
goto patch10
:fail9
pushd ..
copy /b openssl\crypto\rand\rand_win.c.orig openssl\crypto\rand\rand_win.c.orig2
%folder%\patch %cmdopt1% < %folder%\CryptAcquireContextW_win95.txt
%folder%\patch %cmdopt2% < %folder%\CryptAcquireContextW_win95.txt
popd



:patch10


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


