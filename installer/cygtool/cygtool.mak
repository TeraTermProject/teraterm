CPP=cl.exe
LINK32=link.exe

CFLAG=/nologo /I "..\..\teraterm\common" /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /W2
LDFLAG=/nologo /SUBSYSTEM:WINDOWS /DLL

all: cygtool.dll cygtool.exe

cygtool.dll: cygtool.c
	$(CPP) $(CFLAG) /MT /c cygtool.c
	$(LINK32) $(LDFLAG) /DEF:cygtool.def cygtool.obj

cygtool.exe: cygtool.c
	$(CPP) $(CFLAG) /D "EXE" cygtool.c

clean:
	del *.exe *.dll *.obj *.exp *.lib
