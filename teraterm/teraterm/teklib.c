/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2017 TeraTerm Project
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

/* TERATERM.EXE, TTTEK.DLL interface */

#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"
#include "ttwinman.h"
#include "ttutil.h"

#include "teklib.h"

PTEKInit TEKInit;
PTEKResizeWindow TEKResizeWindow;
PTEKChangeCaret TEKChangeCaret;
PTEKDestroyCaret TEKDestroyCaret;
PTEKParse TEKParse;
PTEKReportGIN TEKReportGIN;
PTEKPaint TEKPaint;
PTEKWMLButtonDown TEKWMLButtonDown;
PTEKWMLButtonUp TEKWMLButtonUp;
PTEKWMMouseMove TEKWMMouseMove;
PTEKWMSize TEKWMSize;
PTEKCMCopy TEKCMCopy;
PTEKCMCopyScreen TEKCMCopyScreen;
PTEKPrint TEKPrint;
PTEKClearScreen TEKClearScreen;
PTEKSetupFont TEKSetupFont;
PTEKResetWin TEKResetWin;
PTEKRestoreSetup TEKRestoreSetup;
PTEKEnd TEKEnd;

static HMODULE HTTTEK = NULL;
static UseCount = 0;

static const GetProcAddressList ProcList[] = {
	{ &TEKInit, "TEKInit", 8 },
	{ &TEKResizeWindow, "TEKResizeWindow", 16 },
	{ &TEKChangeCaret, "TEKChangeCaret", 8 },
	{ &TEKDestroyCaret, "TEKDestroyCaret", 8 },
	{ &TEKParse, "TEKParse", 12 },
	{ &TEKReportGIN, "TEKReportGIN", 16  },
	{ &TEKPaint, "TEKPaint", 16 },
	{ &TEKWMLButtonDown, "TEKWMLButtonDown", 20 },
	{ &TEKWMLButtonUp, "TEKWMLButtonUp", 8 },
	{ &TEKWMMouseMove, "TEKWMMouseMove", 16 },
	{ &TEKWMSize, "TEKWMSize", 24 },
	{ &TEKCMCopy, "TEKCMCopy", 8 },
	{ &TEKCMCopyScreen, "TEKCMCopyScreen", 8 },
	{ &TEKPrint, "TEKPrint", 16 },
	{ &TEKClearScreen, "TEKClearScreen", 8 },
	{ &TEKSetupFont, "TEKSetupFont", 8 },
	{ &TEKResetWin, "TEKResetWin", 12 },
	{ &TEKRestoreSetup, "TEKRestoreSetup", 8 },
	{ &TEKEnd, "TEKEnd", 4 },
};

static void FreeTTTEKCommon()
{
    FreeLibrary(HTTTEK);
    HTTTEK = NULL;

	ClearProcAddressses(ProcList, _countof(ProcList));
}

BOOL LoadTTTEK()
{
	if (UseCount == 0) {
		BOOL ret;

		HTTTEK = LoadHomeDLL("TTPTEK.DLL");
		if (HTTTEK == NULL) return FALSE;

		ret = GetProcAddressses(HTTTEK, ProcList, _countof(ProcList));
		if (!ret)
		{
			FreeTTTEKCommon();
			return FALSE;
		}
	}
	UseCount++;
	return TRUE;
}

BOOL FreeTTTEK()
{
	if (UseCount == 0) {
		return FALSE;
	}
	UseCount--;
	if (UseCount == 0) {
		FreeTTTEKCommon();
	}
	return TRUE;
}
