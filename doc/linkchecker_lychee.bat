@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set NOPAUSE=1
call ..\buildtools\install_lychee.bat
set LYCHEE=%CUR%..\buildtools\lychee\lychee.exe

rem lychee は UTF-8 以外のファイルを読めないため、
rem html を UTF-8 に変換したコピーを作ってチェックする
set WORK=%TEMP%\linkchecker_lychee

set RESULT=0
call :check ja
call :check en

rmdir /s /q "%WORK%" 2> nul
pause
exit /b %RESULT%

:check
echo %1
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "$src = '%CUR%%1\html'; $dst = '%WORK%\%1';" ^
 "if (Test-Path $dst) { Remove-Item -Recurse -Force $dst };" ^
 "Get-ChildItem -Recurse -File $src | ForEach-Object {" ^
 "  $rel = $_.FullName.Substring($src.Length + 1); $out = Join-Path $dst $rel;" ^
 "  New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null;" ^
 "  if ($_.Extension -match '^\.html?$') {" ^
 "    $text = [IO.File]::ReadAllText($_.FullName, [Text.Encoding]::GetEncoding(932));" ^
 "    [IO.File]::WriteAllText($out, $text, (New-Object Text.UTF8Encoding $false))" ^
 "  } else { Copy-Item $_.FullName $out }" ^
 "}"
pushd "%WORK%\%1"
"%LYCHEE%" --offline --include-fragments --no-progress "**/*.html"
if not %errorlevel% == 0 set RESULT=1
popd
exit /b
