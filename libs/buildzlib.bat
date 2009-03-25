cd zlib

if exist "win32\Makefile.msc.rel" goto build

mkdir debug
mkdir release

perl -e "open(IN,'win32\Makefile.msc');while(<IN>){s/ -MD/ -MT/;print $_;}close(IN);" > win32\Makefile.tmp
copy /Y win32\Makefile.tmp win32\Makefile.msc.rel
del win32\Makefile.tmp

perl -e "open(IN,'win32\Makefile.msc');while(<IN>){s/ -MD -O2/ -MTd -Od/;s/ -release/ -debug/;s/ zlib.lib/ zlibd.lib/;s/ zlib.lib/ zlibd.lib/;print $_;}close(IN);" > win32\Makefile.tmp
copy /Y win32\Makefile.tmp win32\Makefile.msc.deb
del win32\Makefile.tmp

:build
nmake -f win32\Makefile.msc.rel clean all
copy /Y zlib.lib release\
nmake -f win32\Makefile.msc.deb clean all
copy /Y zlibd.lib debug\

cd ..
