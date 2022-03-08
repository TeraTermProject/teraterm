# Makefile for CygTerm

BINDIR = $(HOME)/bin

CC = gcc
CXX = g++
RC = windres
UNAME_A = $(shell uname -a)

ifneq (,$(findstring x86_64, "$(UNAME_A)"))
ifneq (,$(findstring x86_64, $(RC)))
# x86_64-pc-cygwin-windres is not exists
# use windres, when x86_64 env
override RC= windres
endif
else
ifneq (,$(findstring i686, $(RC)))
# i686-pc-cygwin-windres is not exsit
# use windres, when i686 env
override RC= windres
endif
endif

CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions -DUNICODE -D_UNICODE
#CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions
CXXFLAGS = $(CFLAGS)
LDFLAGS = -mwindows

EXE = cygterm.exe
SRC = \
	cygterm.cc \
	cygterm_cfg.cc \
	cygterm_cfg.h \
	sub.cpp \
	sub.h
CFG = cygterm.cfg
RES = cygterm.res
ICO = cygterm.ico
SRC_RC  = cygterm.rc
ARCHIVE = cygterm+.tar.gz

.PHONY: all clean install uninstall cygterm_x86_64 cygterm_i686

all : $(EXE)

archive : $(ARCHIVE)

$(EXE) : cygterm.o cygterm_cfg.o sub.o $(RES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXE) $^ -lole32
	strip $(EXE)

$(RES): $(SRC_RC) $(ICO)
	$(RC) -O coff -o $(RES) $(SRC_RC)

$(SRC_RC):
	echo 'icon ICON $(ICO)' > $(SRC_RC)

clean :
	rm -f $(EXE) $(SRC_RC) $(RES) $(ARCHIVE) *.o *.obj

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

# call sub make
cygterm_x86_64:
	make CC=x86_64-pc-cygwin-gcc CXX=x86_64-pc-cygwin-g++ RC=x86_64-pc-cygwin-windres EXE=cygterm_x86_64.exe

cygterm_i686:
	make CC=i686-pc-cygwin-gcc CXX=i686-pc-cygwin-g++ RC=i686-pc-cygwin-windres clean all EXE=cygterm_i686.exe

