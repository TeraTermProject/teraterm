/*
 * (C) 2025- TeraTerm Project
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

#include "tttypes.h"

#include "vtdraw.h"

/**
 * iniファイルのIDから実際の描画で使うIDを返す
 * @param ini_id iniファイルのID,IdVtDrawAPIAutoの場合あり
 * @return API ID Autoの場合、自動判定してUnicodeかANSIを返す
 */
IdVtDrawAPI VTDrawFromID(IdVtDrawAPI ini_id)
{
	if (ini_id == IdVtDrawAPIAuto) {
		return IsWindowsNTKernel() ? IdVtDrawAPIUnicode : IdVtDrawAPIANSI;
	} else {
		return ini_id;
	}
}

/**
 *
 *	@param ini	iniファイル内のIdVtDrawAPI
 *	@retval		実際に使用する IdVtDrawAPI
 */
IdVtDrawAPI VTDrawFromIni(const wchar_t *str, IdVtDrawAPI *ini)
{
	if (_wcsicmp(str, L"auto") == 0) {
		*ini = IdVtDrawAPIAuto;
	} else if (_wcsicmp(str, L"ansi") == 0) {
		*ini = IdVtDrawAPIANSI;
	} else if (_wcsicmp(str, L"unicode") == 0) {
		*ini = IdVtDrawAPIUnicode;
	} else {
		*ini = IdVtDrawAPIAuto;
	}
	return VTDrawFromID(*ini);
}

const wchar_t *VTDrawToIni(IdVtDrawAPI api)
{
	switch (api) {
	case IdVtDrawAPIUnicode:
		return L"Unicode";
	case IdVtDrawAPIANSI:
		return L"ANSI";
	case IdVtDrawAPIAuto:
	default:
		return L"Auto";
	}
}
