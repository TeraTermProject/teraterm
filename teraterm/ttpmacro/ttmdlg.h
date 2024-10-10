/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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
BOOL GetFileName(HWND HWin, wchar_t **fname);
void SetDlgPos(int x, int y);
int OpenInpDlg(wchar_t *Input, const wchar_t *Text, const wchar_t *Caption,
                const wchar_t *Default, BOOL Paswd);
int OpenErrDlg(const char *Msg, const char *Line, int lineno, int start, int end, const char *FileName);
int OpenMsgDlg(const wchar_t *Text, const wchar_t *Caption, BOOL YesNo);
void OpenStatDlg(const wchar_t *Text, const wchar_t *Caption);
void CloseStatDlg();
void BringupStatDlg();

int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int ext, int DlgWidth, int DlgHeight);

extern wchar_t *HomeDirW;
extern wchar_t FileName[MAX_PATH];
extern wchar_t TopicName[11];
extern wchar_t ShortName[MAX_PATH];
extern wchar_t **Params;
extern int ParamCnt;
extern BOOL SleepFlag;
extern DPI_AWARENESS_CONTEXT DPIAware;

#ifdef __cplusplus
}
#endif
