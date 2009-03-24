# Makefile for CygTerm

BINDIR = $(HOME)/bin

CC = gcc
CFLAGS = -O2 -fno-exceptions
#CFLAGS = -g -fno-exceptions
LIBS = -mwindows

LAUNCH = cyglaunch.exe
EXE = cygterm.exe
SRC = $(EXE:.exe=.cc)
CFG = $(EXE:.exe=.cfg)
RES = $(EXE:.exe=.res)
ICO = $(EXE:.exe=.ico)
RC  = $(EXE:.exe=.rc)

all : $(EXE) $(LAUNCH) src

$(EXE) : $(SRC) $(RES)
  ifeq (0, $(shell nm /usr/lib/crt0.o | grep -c WinMainCRTStartup))
	$(CC) $(CFLAGS) -DNO_WIN_MAIN -o $(EXE) $(SRC) $(RES) $(LIBS)
  else
	$(CC) $(CFLAGS) -o $(EXE) $(SRC) $(RES) $(LIBS)
  endif
	strip $(EXE)

$(LAUNCH) : cyglaunch.c $(RES)
	cc -mno-cygwin -mwindows -o cyglaunch cyglaunch.c $(RES)
	strip $(LAUNCH)

$(RC):
	echo 'icon ICON $(ICO)' > $(RC)

$(RES):	$(ICO) $(RC)
	windres -O coff -o $(RES) $(RC)

clean :
	rm -f $(EXE) $(RC) $(RES) $(LAUNCH) cygterm+.tar.gz

install : $(EXE)
	@ install -v $(EXE) $(BINDIR)/$(EXE)
	@ if [ ! -f $(BINDIR)/$(CFG) ]; then \
	    install -v $(CFG) $(BINDIR)/$(CFG) \
	; fi

uninstall :
	rm -f $(BINDIR)/$(EXE)
	rm -f $(BINDIR)/$(CFG)

src :
	tar cf - $(SRC) $(ICO) $(CFG) cyglaunch.c README README-j Makefile | gzip > cygterm+.tar.gz
