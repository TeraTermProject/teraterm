/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTSET interface */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (PASCAL *PReadIniFile)
  (PCHAR FName, PTTSet ts);
typedef void (PASCAL *PWriteIniFile)
  (PCHAR FName, PTTSet ts);
typedef void (PASCAL *PReadKeyboardCnf)
  (PCHAR FName, PKeyMap KeyMap, BOOL ShowWarning);
typedef void (PASCAL *PCopyHostList)
  (PCHAR IniSrc, PCHAR IniDest);
typedef void (PASCAL *PAddHostToList)
  (PCHAR FName, PCHAR Host);
typedef void (PASCAL *PParseParam)
  (PCHAR Param, PTTSet ts, PCHAR DDETopic);
typedef void (PASCAL *PCopySerialList)
  (PCHAR IniSrc, PCHAR IniDest, PCHAR section, PCHAR key, int MaxList);
typedef void (PASCAL *PAddValueToList)
  (PCHAR FName, PCHAR Host, PCHAR section, PCHAR key, int MaxList);

extern PReadIniFile ReadIniFile;
extern PWriteIniFile WriteIniFile;
extern PReadKeyboardCnf ReadKeyboardCnf;
extern PCopyHostList CopyHostList;
extern PAddHostToList AddHostToList;
extern PParseParam ParseParam;
extern PCopySerialList CopySerialList;
extern PAddValueToList AddValueToList;

/* proto types */
BOOL LoadTTSET();
void FreeTTSET();

#ifdef __cplusplus
}
#endif
