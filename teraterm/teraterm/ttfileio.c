/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTSET interface */
#include "teraterm.h"
#include "tttypes.h"

#include "ttfileio.h"
#include "ttplug.h" /* TTPLUG */

TReadFile PReadFile;
TWriteFile PWriteFile;
TCreateFile PCreateFile;
TCloseFile PCloseFile;

static BOOL PASCAL FAR DummyWriteFile(HANDLE fh, LPCVOID buff,
  DWORD len, LPDWORD wbytes, LPOVERLAPPED wol)
{
	*wbytes = len;
	return TRUE;
}

void InitFileIO(int ConnType)
{
  PReadFile = ReadFile;
  if (ConnType == IdFile) {
	PWriteFile = DummyWriteFile;
  }
  else {
	PWriteFile = WriteFile;
  }
  PCreateFile = CreateFile;
  PCloseFile = CloseHandle;
  return;
}
