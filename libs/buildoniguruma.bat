cd oniguruma

if exist "Makefile" goto build

copy win32\config.h config.h
perl -e "open(IN,'win32\Makefile');while(<IN>){s|CFLAGS =|CFLAGS = /MT|;print $_;}close(IN);" > Makefile
perl -e "open(IN,'win32\Makefile');while(<IN>){s|CFLAGS =|CFLAGS = /MTd|;s|libname   = |libname   = debug/|;print $_;}close(IN);" > Makefile.debug
mkdir debug

:build
nmake clean
nmake -f Makefile.debug
nmake clean
nmake

cd ..
