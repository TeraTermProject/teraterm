/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2020 TeraTerm Project
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

/* TTFILE.DLL, file transfer, VT window printing */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <direct.h>
#include <commdlg.h>
#include <string.h>

#include "ttlib.h"
#include "ftlib.h"
#include "dlglib.h"
#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"

#include "filesys_proto.h"
#include "ttfile_proto.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <assert.h>

#if 0
void _ProtoInit(int Proto, PFileVarProto fv, PComVar cv, PTTSet ts)
{
	switch (Proto) {
	case PROTO_KMT:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_XM:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_YM:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_ZM:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_BP:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_QV:
		fv->Init(fv,cv,ts);
		break;
	}
}
#endif

BOOL _ProtoParse(int Proto, PFileVarProto fv, PComVar cv)
{
	BOOL Ok;

	Ok = FALSE;
	switch (Proto) {
	case PROTO_KMT:
		Ok = fv->Parse(fv,cv);
		break;
	case PROTO_XM:
		Ok = fv->Parse(fv, cv);
		break;
	case PROTO_YM:
		Ok = fv->Parse(fv, cv);
		break;
	case PROTO_ZM:
		Ok = fv->Parse(fv, cv);
		break;
	case PROTO_BP:
		Ok = fv->Parse(fv, cv);
		break;
	case PROTO_QV:
		Ok = fv->Parse(fv,cv);
		break;
	}
	return Ok;
}

void _ProtoTimeOutProc(int Proto, PFileVarProto fv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		fv->TimeOutProc(fv,cv);
		break;
	case PROTO_XM:
		fv->TimeOutProc(fv,cv);
		break;
	case PROTO_YM:
		fv->TimeOutProc(fv,cv);
		break;
	case PROTO_ZM:
		fv->TimeOutProc(fv, cv);
		break;
	case PROTO_BP:
		fv->TimeOutProc(fv, cv);
		break;
	case PROTO_QV:
		fv->TimeOutProc(fv,cv);
		break;
	}
}

BOOL _ProtoCancel(int Proto, PFileVarProto fv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		fv->Cancel(fv,cv);
		break;
	case PROTO_XM:
		fv->Cancel(fv,cv);
		break;
	case PROTO_YM:
		fv->Cancel(fv,cv);
		break;
	case PROTO_ZM:
		fv->Cancel(fv, cv);
		break;
	case PROTO_BP:
		fv->Cancel(fv, cv);
		break;
	case PROTO_QV:
		fv->Cancel(fv,cv);
		break;
	}
	return TRUE;
}

int _ProtoSetOpt(PFileVarProto fv, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	int r = fv->SetOptV(fv, request, ap);
	va_end(ap);
	return r;
}
