rem OpenSSLのビルド

cd openssl


rem Visual Studio 2005 の場合はパッチを適用
set CL_VER=
for /f "delims=" %%o in ('cl 2^>^&1') do set CL_VER=%%o & goto end_clver_chk
:end_clver_chk

echo %CL_VER% | find "Compiler Version 14" >nul
if ERRORLEVEL 1 goto patch_end
pushd ..\openssl_patch
call apply_patch.bat
if ERRORLEVEL 1 (
    popd
    goto fail
)
popd

:patch_end


rem ossl_static.pdb は *.pdb なので nmake clean すると削除されてしまう。
rem debug のときのほうが必要だと思われるので、
rem release を先にビルドして debug の ossl_static.pdb が残るようにする。

if exist "out32\libcrypto.lib" goto build_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501
perl -e "open(IN,'makefile');while(<IN>){s| /MD| /MT|;print $_;}close(IN);" > makefile.tmp
if exist "makefile" del makefile
ren makefile.tmp makefile
nmake clean
nmake
mkdir out32
move libcrypto* out32
move libssl* out32
move apps\openssl.exe out32
move ossl_static.pdb out32
:build_end

if exist "out32.dbg\libcrypto.lib" goto build_dbg_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501 --debug
perl -e "open(IN,'makefile');while(<IN>){s| /MDd| /MTd|;print $_;}close(IN);" > makefile.tmp
if exist "makefile.dbg" del makefile.dbg
ren makefile.tmp makefile.dbg
nmake -f makefile.dbg clean
nmake -f makefile.dbg
mkdir out32.dbg
move libcrypto* out32.dbg
move libssl* out32.dbg
move apps\openssl.exe out32.dbg
move ossl_static.pdb out32.dbg
:build_dbg_end


rem Visual Studio 2005 の場合は 2003 R2 Platform SDK の導入を確認する
echo %CL_VER% | find "Compiler Version 14" >nul
if ERRORLEVEL 1 goto end
@echo off
if exist out32\openssl.exe (
    echo OpenSSLのビルドが正常終了しました。
    goto end
)
echo crypt32.lib がリンクできずにバイナリが作成できていません。
echo Platform SDKが導入されていない可能性があります。
set /P ANS2003SDK="続行しますか？(y/n)"
if "%ANS2003SDK%"=="y" (
    goto end
) else (
    goto fail
)

:end
cd ..
@echo on
exit /b 0


:fail
cd ..
echo "buildopenssl11.bat を終了します"
@echo on
exit /b 1
