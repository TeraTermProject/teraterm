
rem パッチ適用チェック
pushd openssl_patch
call check_patch.bat
popd

rem OpenSSLのビルドへ移行

cd openssl

if exist "out32.dbg\libcrypto.lib" goto build_dbg_end

perl -e "open(IN,'Configurations/10-main.conf');while(<IN>){s|/W3|/W1|;s|/WX||;print $_;}close(IN);" > conf.tmp
move conf.tmp Configurations/10-main.conf

perl Configure no-asm no-async shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501 --debug
perl -e "open(IN,'makefile');while(<IN>){s| /MDd| /MTd|;print $_;}close(IN);" > makefile.tmp
if exist "makefile.dbg" del makefile.dbg
ren makefile.tmp makefile.dbg
nmake -f makefile.dbg clean
nmake -f makefile.dbg
mkdir out32.dbg
move libcrypto* out32.dbg
move libssl* out32.dbg
move apps\openssl.exe out32.dbg
:build_dbg_end

if exist "out32\libcrypto.lib" goto build_end
perl Configure no-asm no-async shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501
perl -e "open(IN,'makefile');while(<IN>){s| /MD| /MT|;print $_;}close(IN);" > makefile.tmp
if exist "makefile" del makefile
ren makefile.tmp makefile
nmake clean
nmake
mkdir out32
move libcrypto* out32
move libssl* out32
move apps\openssl.exe out32
:build_end

cd ..
