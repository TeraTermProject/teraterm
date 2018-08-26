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

/* TERATERM.EXE, TTSET interface */
#include "teraterm.h"
#include "tttypes.h"

#include "ttsetup.h"
#include "ttplug.h" /* TTPLUG */
#include "ttutil.h"

#include "ttwinman.h"

static HANDLE HTTSET = NULL;
static int UseCount = 0;

PReadIniFile ReadIniFile;
PWriteIniFile WriteIniFile;
PReadKeyboardCnf ReadKeyboardCnf;
PCopyHostList CopyHostList;
PAddHostToList AddHostToList;
PParseParam ParseParam;
PCopySerialList CopySerialList;
PAddValueToList AddValueToList;

static const GetProcAddressList ProcList[] = {
	{ &ReadIniFile, "ReadIniFile", 8 },
	{ &WriteIniFile, "WriteIniFile", 8 },
	{ &ReadKeyboardCnf, "ReadKeyboardCnf", 12 },
	{ &CopyHostList, "CopyHostList", 8 },
	{ &AddHostToList, "AddHostToList", 8 },
	{ &ParseParam, "ParseParam", 12 },
	{ &CopySerialList, "CopySerialList", 20 },
	{ &AddValueToList, "AddValueToList", 20 },
};

static void FreeTTSetCommon()
{
	FreeLibrary(HTTSET);
	HTTSET = NULL;

	ClearProcAddressses(ProcList, _countof(ProcList));
}

BOOL LoadTTSET()
{
	if (UseCount == 0) {
		BOOL ret;

		HTTSET = LoadHomeDLL("TTPSET.DLL");
		if (HTTSET == NULL) return FALSE;

		ret = GetProcAddressses(HTTSET, ProcList, _countof(ProcList));
		if (!ret) {
			FreeTTSetCommon();
			return FALSE;
		}

		TTXGetSetupHooks(); /* TTPLUG */
	}
	UseCount++;
	return TRUE;
}

BOOL FreeTTSET()
{
	if (UseCount == 0) {
		return FALSE;
	}
	UseCount--;
	if (UseCount == 0) {
		FreeTTSetCommon();
	}
	return TRUE;
}
