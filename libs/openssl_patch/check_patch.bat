@echo off

rem
rem OpenSSL 1.1.1にパッチが適用されているかを確認する
rem

findstr /c:"# undef AI_PASSIVE" ..\openssl\crypto\bio\bio_lcl.h
if ERRORLEVEL 1 goto fail

findstr /c:"running on Windows95" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail

echo "パッチは適用されています"
timeout 5
goto end

:fail
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

:end
@echo on


