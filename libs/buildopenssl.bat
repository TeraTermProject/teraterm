cd openssl

if exist "Makefile.bak" goto build

perl Configure no-asm VC-WIN32

copy ms\do_ms.bat ms\do_ms.bat.orig
perl -e "open(IN,'ms\do_ms.bat');while(<IN>){print $_;}print 'perl util\mk1mf.pl no-asm debug VC-WIN32 >ms\ntd.mak';print \"\n\";close(IN);" > ms\do_ms.bat.tmp
copy /Y ms\do_ms.bat.tmp ms\do_ms.bat

call ms\do_ms.bat

:build
nmake -f ms\nt.mak
nmake -f ms\ntd.mak

cd ..
