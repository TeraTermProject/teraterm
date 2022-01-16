rem OpenSSLのビルド

cd openssl


rem nmmake clean すると ossl_static.pdb は *.pdb なので削除されてしまう。
rem ossl_static.pdb は *.pdb なので nmake clean すると削除されてしまう。
rem debug のときのほうが必要だと思われるので、
rem release を先にビルドして debug の ossl_static.pdb が残るようにする。

if exist "out32\libcrypto.lib" goto build_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32
perl -e "open(IN,'makefile');while(<IN>){s| /MD| /MT|;print $_;}close(IN);" > makefile.tmp
move /y makefile.tmp makefile
nmake -f makefile clean
nmake -f makefile build_libs
mkdir out32
move /y libcrypto.lib out32\
move /y ossl_static.pdb out32\
:build_end

if exist "out32.dbg\libcrypto.lib" goto build_dbg_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32 --debug
perl -e "open(IN,'makefile');while(<IN>){s| /MDd| /MTd|;print $_;}close(IN);" > makefile.tmp
move /y makefile.tmp makefile.dbg
nmake -f makefile.dbg clean
nmake -f makefile.dbg build_libs
mkdir out32.dbg
move /y libcrypto.lib out32.dbg\
move /y ossl_static.pdb out32.dbg\
:build_dbg_end


cd ..
exit /b 0
