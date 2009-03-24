cd openssl

if exist "Makefile.bak" goto build

perl Configure VC-WIN32

copy ms\do_ms.bat ms\do_ms.bat.orig
perl -e "open(IN,'ms\do_ms.bat');while(<IN>){print $_;}print 'perl util\mk1mf.pl no-asm debug VC-WIN32 >ms\ntd.mak';print \"\n\";close(IN);" > ms\do_ms.bat.tmp
copy /Y ms\do_ms.bat.tmp ms\do_ms.bat

call ms\do_ms.bat

copy ms\nt.mak ms\nt.mak.orig
perl -e "open(IN,'ms\nt.mak');while(<IN>){s/\/MD/\/MT/;print $_;}close(IN);" > ms\nt.mak.tmp
copy /Y ms\nt.mak.tmp ms\nt.mak
del ms\nt.mak.tmp

copy ms\ntd.mak ms\ntd.mak.orig
perl -e "open(IN,'ms\ntd.mak');while(<IN>){s/\/MD/\/MT/;print $_;}close(IN);" > ms\ntd.mak.tmp
copy /Y ms\ntd.mak.tmp ms\ntd.mak
del ms\ntd.mak.tmp

:build
nmake -f ms\nt.mak
nmake -f ms\ntd.mak

cd ..
