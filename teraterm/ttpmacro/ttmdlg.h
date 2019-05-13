/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TTMACRO.EXE, dialog boxes */

#pragma once

#include "ttmdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void ParseParam(PBOOL IOption, PBOOL VOption);
BOOL GetFileName(HWND HWin);
void SetDlgPos(int x, int y);
void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd);
int OpenErrDlg(PCHAR Msg, PCHAR Line, int lineno, int start, int end, PCHAR FileName);
int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo);
void OpenStatDlg(PCHAR Text, PCHAR Caption);
void CloseStatDlg();
void BringupStatDlg();

int OpenListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int Selected);

extern char HomeDir[MAX_PATH];
extern char FileName[MAX_PATH];
extern char TopicName[11];
extern char ShortName[MAX_PATH];
extern char **Params;
extern int ParamCnt;
extern BOOL SleepFlag;

#ifdef __cplusplus
}
#endif
