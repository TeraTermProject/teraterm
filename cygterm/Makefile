# Makefile for CygTerm

BINDIR = $(HOME)/bin

CC = gcc
CFLAGS = -O2 -fno-exceptions
#CFLAGS = -g -fno-exceptions
LDFLAGS = -mwindows

LAUNCH = cyglaunch.exe
LAUNCH_SRC = cyglaunch.c
EXE = cygterm.exe
SRC = $(EXE:.exe=.cc)
CFG = $(EXE:.exe=.cfg)
RES = $(EXE:.exe=.res)
ICO = $(EXE:.exe=.ico)
RC  = $(EXE:.exe=.rc)
ARCHIVE = cygterm+.tar.gz

.PHONY: all clean install uninstall

all : $(EXE) $(LAUNCH) $(ARCHIVE)

$(EXE) : $(SRC) $(RES)
  ifeq (0, $(shell nm /usr/lib/crt0.o | grep -c WinMainCRTStartup))
	$(CC) $(CFLAGS) $(LDFLAGS) -DNO_WIN_MAIN -o $(EXE) $(SRC) $(RES)
  else
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXE) $(SRC) $(RES)
  endif
	strip $(EXE)

$(LAUNCH) : $(LAUNCH_SRC) $(RES)
	@# gcc 4.3.2? or later doesn't support "-mno-cygwin" flag.
	@#
	@# $(CC)                   gcc less than 4.3.2 (supports "-mno-cygwin")
	@# gcc-3                   gcc3 (supports "-mno-cygwin")
	@# x86_64-w64-mingw32-gcc  mingw32 gcc  host:64bit target:64bit (64bit cygwin)
	@# i686-w64-mingw32-gcc    mingw32 gcc  host:32bit target:64bit (32bit cygwin on 64bit Windows)
	@# i686-pc-mingw32-gcc     mingw32 gcc  host:32bit target:32bit (32bit cygwin)
	( $(CC) $(CFLAGS) $(LDFLAGS) -mno-cygwin -o $(LAUNCH) $(LAUNCH_SRC) $(RES) ) || \
	( gcc-3 $(CFLAGS) $(LDFLAGS) -mno-cygwin -o $(LAUNCH) $(LAUNCH_SRC) $(RES) ) || \
	( x86_64-w64-mingw32-gcc $(CFLAGS) $(LDFLAGS) -o $(LAUNCH) $(LAUNCH_SRC) $(RES) ) || \
	( i686-w64-mingw32-gcc $(CFLAGS) $(LDFLAGS) -o $(LAUNCH) $(LAUNCH_SRC) $(RES) ) || \
	( i686-pc-mingw32-gcc $(CFLAGS) $(LDFLAGS) -o $(LAUNCH) $(LAUNCH_SRC) $(RES) ) 
	strip $(LAUNCH)

$(RC):
	echo 'icon ICON $(ICO)' > $(RC)

$(RES): $(ICO) $(RC)
	windres -O coff -o $(RES) $(RC)

clean :
	rm -f $(EXE) $(RC) $(RES) $(LAUNCH) $(ARCHIVE)

install : $(EXE)
	@ install -v $(EXE) $(BINDIR)/$(EXE)
	@ if [ ! -f $(BINDIR)/$(CFG) ]; then \
	    install -v $(CFG) $(BINDIR)/$(CFG) \
	; fi

uninstall :
	rm -f $(BINDIR)/$(EXE)
	rm -f $(BINDIR)/$(CFG)

$(ARCHIVE) : $(SRC) $(ICO) $(CFG) $(LAUNCH_SRC) README README-j Makefile
	tar cf - $(SRC) $(ICO) $(CFG) $(LAUNCH_SRC) README README-j Makefile | gzip > $(ARCHIVE)
