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

#pragma once

/* TERATERM.EXE, file transfer routines */
#ifdef __cplusplus
extern "C" {
#endif

BOOL IsSendVarNULL(void);
BOOL IsFileVarNULL(void);

BOOL FileSendStart(const wchar_t *filename, int binary);
void FileSend(void);
void FileSendEnd(void);
void FileSendPause(BOOL Pause);

void ProtoEnd(void);
int ProtoDlgParse(void);
void ProtoDlgTimeOut(void);
void ProtoDlgCancel(void);
BOOL KermitStartSend(const char *filename);
BOOL KermitGet(const char *filename);
BOOL KermitStartRecive(BOOL macro);
BOOL KermitFinish(BOOL macro);
BOOL XMODEMStartReceive(const char *fiename, WORD ParamBinaryFlag, WORD ParamXmodemOpt);
BOOL XMODEMStartSend(const char *fiename, WORD ParamXmodemOpt);
BOOL YMODEMStartReceive(BOOL macro);
BOOL YMODEMStartSend(const char *fiename);
BOOL ZMODEMStartReceive(BOOL macro, BOOL autostart);
BOOL ZMODEMStartSend(const char *fiename, WORD ParamBinaryFlag, BOOL autostart);
BOOL BPStartSend(const char *filename);
BOOL BPStartReceive(BOOL macro, BOOL autostart);
BOOL QVStartReceive(BOOL macro);
BOOL QVStartSend(const char *filename);

#ifdef __cplusplus
}
#endif

#include "filesys_log.h"
