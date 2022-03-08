# Makefile for cyglaunch

BINDIR = $(HOME)/bin

UNAME_A = $(shell uname -a)
UNAME_M = $(shell uname -m)
CURRENT_MAKEFILE_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

ifeq (i686, $(UNAME_M))

# mingw 32bit
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
STRIP = i686-w64-mingw32-strip
RC = i686-w64-mingw32-windres

# gcc3 (supports "-mno-cygwin")
#CC = gcc-3 -mno-cygwin
#CXX = g++-3 -mno-cygwin
#RC = windres
#STRIP = strip

# gcc less than 4.3.2 (supports "-mno-cygwin")
#CC = gcc -mno-cygwin
#CXX = g++ -mno-cygwin
#RC = windres
#STRIP = strip

else

# mingw 64bit
CC = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
STRIP = x86_64-w64-mingw32-strip
RC = x86_64-w64-mingw32-windres

endif

CFLAGS = -O2 -fno-exceptions -DUNICODE -D_UNICODE -MMD -I $(CYGLIB_DIR) -I $(COMMON_DIR)
#CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions -DUNICODE -D_UNICODE -MMD -I../cyglib -I../../teraterm/common
#CFLAGS = -D_GNU_SOURCE -O2 -fno-exceptions
CXXFLAGS = $(CFLAGS)
LDFLAGS = -mwindows -municode
#LDFLAGS = -municode

ifneq ("$(wildcard asprintf.cpp)","")
COMMON_DIR = .
CYGLIB_DIR = .
CYGTERM_DIR = .
else
COMMON_DIR = ../../teraterm/common
CYGLIB_DIR = ../cyglib
CYGTERM_DIR = ../cygterm
endif

SRC = \
	cyglaunch.c \
	$(CYGLIB_DIR)/cyglib.c \
	$(COMMON_DIR)/asprintf.cpp \
	$(COMMON_DIR)/win32helper.cpp \
	$(COMMON_DIR)/ttlib_static_dir.cpp \
	$(COMMON_DIR)/ttknownfolders.c
EXE = cyglaunch.exe
RES = cyglaunch.res
SRC_RC = cyglaunch.rc
OBJ = $(addsuffix .o,$(notdir $(basename $(SRC))))
DEP = $(OBJ:.o=.d)
ICO = $(CYGTERM_DIR)/cygterm.ico
ARCHIVE = cygterm+.tar.gz


.PHONY: all clean install uninstall arc $(ARCHIVE)

all : $(EXE)

$(EXE) : $(OBJ) $(RES)
	$(CC) -o $@ $^ $(LDFLAGS) -lole32
	$(STRIP) $@

clean :
	@rm -f $(OBJ)
	@rm -f $(DEP)
	@rm -f $(SRC_RC)
	@rm -f $(RES)
	@rm -f $(EXE)

$(RES): $(SRC_RC) $(ICO)
	@mkdir -p $(dir $@)
	$(RC) -O coff -o $@ $(SRC_RC)

$(SRC_RC):
	echo 'icon ICON "$(ICO)"' > $(SRC_RC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.o: $(COMMON_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: $(CYGLIB_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
ttlib_static_dir.o: $(COMMON_DIR)/ttlib_static_dir.cpp
	$(CC) $(CFLAGS) -c $< -o $@ -DENABLE_COMAPT_WIN=0


-include $(DEP)

test:
	@echo $(SRC)
	@echo $(OBJ)
	@echo $(COMMON_DIR)
	@echo $(CYGLIB_DIR)
	@echo $(MAKEFILE_LIST)
	@echo $(MAKEFILE)
	@echo $(CURRENT_MAKEFILE_DIR)

arc: $(ARCHIVE)

$(ARCHIVE):
ifneq ("$(wildcard $(ARCHIVE))","")
	gunzip $(ARCHIVE)
endif
	tar rvf $(ARCHIVE:.gz=) cyglaunch.c Makefile -C $(COMMON_DIR) asprintf.cpp asprintf.h win32helper.cpp win32helper.h -C $(CURRENT_MAKEFILE_DIR)/$(CYGLIB_DIR) cyglib.c cyglib.h -C $(CURRENT_MAKEFILE_DIR)/$(CYGTERM_DIR) cygterm.ico
	gzip $(ARCHIVE:.gz=)

