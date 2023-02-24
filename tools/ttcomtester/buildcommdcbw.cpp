/*
 * (C) 2023- TeraTerm Project
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

#include <windows.h>
#include <string.h>
#include <wchar.h>

#include "buildcommdcbw.h"

/**
 *	BuildCommDCBW() API wrapper
 *		"rts=hs" がうまくパースできないようなのでラップする
 *		"rts=on" "rts=off" は問題ない
 *
 *		BuildCommDCBW()
 *			https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-buildcommdcbw
 *		DCB
 *			https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-dcb
 *		mode command
 *			https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/mode
 */
BOOL _BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
	bool rts_handshake = false;
	wchar_t *def = _wcsdup(lpDef);
	wchar_t *rts = wcsstr(def, L"rts=hs");
	if (rts != NULL) {
		rts_handshake = true;
		wmemcpy(rts, L"      ", 6);
	}
	BOOL r = BuildCommDCBW(def, lpDCB);
	free(def);
	if (r == FALSE) {
		return FALSE;
	}
	if (rts_handshake) {
		lpDCB->fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	return TRUE;
}
