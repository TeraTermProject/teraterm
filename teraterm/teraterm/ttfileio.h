/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Serial/File interface */
#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (PASCAL FAR *TReadFile)
  (HANDLE FHandle, LPVOID Buff, DWORD ReadSize, LPDWORD ReadBytes,
   LPOVERLAPPED ReadOverLap);
typedef BOOL (PASCAL FAR *TWriteFile)
  (HANDLE FHandle, LPCVOID Buff, DWORD WriteSize, LPDWORD WriteBytes,
   LPOVERLAPPED WriteOverLap);
typedef HANDLE (PASCAL FAR *TCreateFile)
  (LPCTSTR FName, DWORD AcMode, DWORD ShMode, LPSECURITY_ATTRIBUTES SecAttr,
   DWORD CreateDisposition, DWORD FileAttr, HANDLE Template);
typedef BOOL (PASCAL FAR *TCloseFile)
  (HANDLE FHandle);

extern TReadFile PReadFile;
extern TWriteFile PWriteFile;
extern TCreateFile PCreateFile;
extern TCloseFile PCloseFile;

/* proto types */
void InitFileIO(int ConnType);

#ifdef __cplusplus
}
#endif
