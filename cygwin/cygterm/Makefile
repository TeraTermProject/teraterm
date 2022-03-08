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

CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions -DUNICODE -D_UNICODE -MMD
#CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions
CXXFLAGS = $(CFLAGS)
LDFLAGS = -mwindows

EXE_BASE = cygterm.exe
SRC = \
	cygterm.cpp \
	cygterm_cfg.cpp \
	sub.cpp
H = \
	cygterm_cfg.h \
	sub.h

#BINARY_DIR = cygterm_x86_64

ifeq (,$(BINARY_DIR))
# output to current dir
EXE = $(EXE_BASE)
OBJ = $(SRC:.cpp=.o)
RES = cygterm.res
else
# output to $(BINARY_DIR)
EXE = $(BINARY_DIR)/$(EXE_BASE)
OBJ = $(addprefix $(BINARY_DIR)/,$(SRC:.cpp=.o))
RES = $(BINARY_DIR)/cygterm.res
endif
SRC_RC = cygterm.rc
DEP = $(OBJ:.o=.d)

ICO = cygterm.ico
CFG = cygterm.cfg
ARCHIVE = cygterm+.tar.gz

.PHONY: all clean install uninstall archive
.PHONY: cygterm+-x86_64 cygterm+-i686
.PHONY: cygterm+-x86_64-clean cygterm+-i686-clean

all : $(EXE)
ifneq (,$(wildcard Makefile.cyglaunch))
	make -f Makefile.cyglaunch
endif

-include $(DEP)

archive : $(ARCHIVE)

$(EXE) : $(OBJ) $(RES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lole32
	strip $@

$(RES): $(SRC_RC) $(ICO)
	@mkdir -p $(dir $@)
	$(RC) -O coff -o $@ $(SRC_RC)

$(SRC_RC):
	echo 'icon ICON $(ICO)' > $(SRC_RC)

clean :
	rm -f $(EXE) $(SRC_RC) $(RES) $(ARCHIVE)
	rm -f $(DEP)
	rm -f $(OBJ) *.o *.obj
	rm -f $(ARCHIVE)
ifneq (,$(BINARY_DIR))
	rm -rf $(BINARY_DIR)
endif

install : $(EXE)
	@ install -v $(EXE) $(BINDIR)/$(EXE)
	@ if [ ! -f $(BINDIR)/$(CFG) ]; then \
	    install -v $(CFG) $(BINDIR)/$(CFG) \
	; fi

uninstall :
	rm -f $(BINDIR)/$(EXE)
	rm -f $(BINDIR)/$(CFG)

$(ARCHIVE) : $(SRC) $(H) $(ICO) $(CFG) COPYING README README-j Makefile CMakeLists.txt
	rm -f $(ARCHIVE)
	rm -rf tar_tmp/
	mkdir tar_tmp
	cp $^ tar_tmp/
	cd ../cyglaunch/; make copyfile COPY_DEST=../cygterm/tar_tmp
	(cd tar_tmp; tar cf ../$(ARCHIVE:.gz=) *)
	gzip $(ARCHIVE:.gz=)
	rm -rf tar_tmp/

cygterm.o:
  ifeq (0, $(shell nm /usr/lib/crt0.o | grep -c WinMainCRTStartup))
	$(CXX) $(CXXFLAGS) -DNO_WIN_MAIN cygterm.cpp -c -o $@
  else
	$(CXX) $(CXXFLAGS) cygterm.cpp -c -o $@
  endif

# call sub make
cygterm+-x86_64:
	make CC=x86_64-pc-cygwin-gcc CXX=x86_64-pc-cygwin-g++ RC=x86_64-pc-cygwin-windres EXE_BASE=cygterm.exe BINARY_DIR=cygterm+-x86_64 all

cygterm+-x86_64-clean:
	make BINARY_DIR=cygterm+-x86_64 clean

cygterm+-i686:
	make CC=i686-pc-cygwin-gcc CXX=i686-pc-cygwin-g++ RC=i686-pc-cygwin-windres EXE_BASE=cygterm.exe BINARY_DIR=cygterm+-i686 all

cygterm+-i686-clean:
	make BINARY_DIR=cygterm+-i686 clean

$(BINARY_DIR)/%.o: %.cpp
	@mkdir -p $(BINARY_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
