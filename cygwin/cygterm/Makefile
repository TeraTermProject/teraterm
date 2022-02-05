# Makefile for CygTerm

BINDIR = $(HOME)/bin

CC = gcc
CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions -DUNICODE -D_UNICODE
#CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions
CXXFLAGS = $(CFLAGS)
LDFLAGS = -mwindows

EXE = cygterm.exe
SRC = \
	$(EXE:.exe=.cc) \
	cygterm_cfg.cc \
	cygterm_cfg.h \
	sub.cpp \
	sub.h
CFG = $(EXE:.exe=.cfg)
RES = $(EXE:.exe=.res)
ICO = $(EXE:.exe=.ico)
RC  = $(EXE:.exe=.rc)
ARCHIVE = cygterm+.tar.gz

.PHONY: all clean install uninstall

all : $(EXE) $(ARCHIVE)

$(EXE) : cygterm.o cygterm_cfg.o sub.o $(RES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXE) $^ -lole32
	strip $(EXE)

$(RES): $(RC)
	windres -O coff -o $(RES) $(RC)

$(RC):
	echo 'icon ICON $(ICO)' > $(RC)

clean :
	rm -f $(EXE) $(RC) $(RES) $(ARCHIVE) *.o *.obj

install : $(EXE)
	@ install -v $(EXE) $(BINDIR)/$(EXE)
	@ if [ ! -f $(BINDIR)/$(CFG) ]; then \
	    install -v $(CFG) $(BINDIR)/$(CFG) \
	; fi

uninstall :
	rm -f $(BINDIR)/$(EXE)
	rm -f $(BINDIR)/$(CFG)

$(ARCHIVE) : $(SRC) $(ICO) $(CFG) README README-j Makefile CMakeLists.txt
	tar cf - $(SRC) $(ICO) $(CFG) COPYING README README-j Makefile CMakeLists.txt msys2term.cfg | gzip > $(ARCHIVE)

cygterm.o:
  ifeq (0, $(shell nm /usr/lib/crt0.o | grep -c WinMainCRTStartup))
	$(CXX) $(CXXFLAGS) -DNO_WIN_MAIN cygterm.cc -c -o $@
  else
	$(CXX) $(CXXFLAGS) cygterm.cc -c -o $@
  endif

# cc -M *.cc *.cpp
cygterm.o: cygterm.cc sub.h cygterm_cfg.h
cygterm_cfg.o: cygterm_cfg.cc cygterm_cfg.h
sub.o: sub.cpp sub.h
