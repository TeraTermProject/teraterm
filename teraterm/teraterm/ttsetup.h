/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

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
