cd openssl

if exist "ms\ntd.mak" goto build

perl Configure no-asm VC-WIN32

copy ms\do_ms.bat ms\do_ms.bat.orig
echo perl util\mk1mf.pl no-asm debug VC-WIN32 ^>ms\ntd.mak>>ms\do_ms.bat

call ms\do_ms.bat

:build
nmake -f ms\nt.mak
nmake -f ms\ntd.mak

cd ..
