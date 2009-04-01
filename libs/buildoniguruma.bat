cd oniguruma

if exist "Makefile" goto build

copy win32\config.h config.h
perl -e "open(IN,'win32\Makefile');while(<IN>){s|CFLAGS =|CFLAGS = /MT|;print $_;}close(IN);" > Makefile
perl -e "open(IN,'win32\Makefile');while(<IN>){s|CFLAGS = -O2|CFLAGS = /MTd -Od|;s|_s.lib|_sd.lib|;print $_;}close(IN);" > Makefile.debug

:build
if exist onig_sd.lib goto build_release
nmake -f Makefile.debug clean
nmake -f Makefile.debug
move onig_sd.lib sample\
nmake clean
move sample\onig_sd.lib .\

:build_release
nmake

cd ..
