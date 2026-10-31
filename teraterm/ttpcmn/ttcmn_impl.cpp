/*
 * (C) 2026- TeraTerm Project
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

/*
 *	入出力バッファ制御(ttcmn_buff.c)の互換エクスポート
 *	プラグインから ttpcmn.dll の Comm 系 API を呼び出せるようにする。
 *	ttpcmn.lib経由でリンクする。
 *	エクスポートした関数から、ttermpro.exe 内の関数ポインタを呼び出す。
 *	ttcmn_buff.c の実体は ttermpro.exe 内。
 *
 *	TODO  Comm*()系削除 このエントリを使っているpluginはないと思われる
 */

#include <string.h>
#include <windows.h>

#include "teraterm.h"
#include "tttypes.h"

#define DllExport __declspec(dllexport)
#include "ttcmn_i.h"

static CommBuffFuncs Funcs;

extern "C" {

/**
 *	入出力バッファ制御の実体を登録する
 *	ttermpro.exe の起動時に一度だけ呼ばれる
 *
 *	@param	funcs	NULL のとき登録を解除する
 */
DllExport void WINAPI CommBuffSetFuncs(const CommBuffFuncs *funcs)
{
	if (funcs == NULL) {
		memset(&Funcs, 0, sizeof(Funcs));
	}
	else {
		Funcs = *funcs;
	}
}

DllExport int WINAPI CommReadRawByte(PComVar cv, LPBYTE b)
{
	if (Funcs.CommReadRawByte == NULL) {
		return 0;
	}
	return Funcs.CommReadRawByte(cv, b);
}

DllExport int WINAPI CommRead1Byte(PComVar cv, LPBYTE b)
{
	if (Funcs.CommRead1Byte == NULL) {
		return 0;
	}
	return Funcs.CommRead1Byte(cv, b);
}

DllExport void WINAPI CommInsert1Byte(PComVar cv, BYTE b)
{
	if (Funcs.CommInsert1Byte == NULL) {
		return;
	}
	Funcs.CommInsert1Byte(cv, b);
}

DllExport int WINAPI CommRawOut(PComVar cv, PCHAR B, int C)
{
	if (Funcs.CommRawOut == NULL) {
		return 0;
	}
	return Funcs.CommRawOut(cv, B, C);
}

DllExport int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C)
{
	if (Funcs.CommBinaryOut == NULL) {
		return 0;
	}
	return Funcs.CommBinaryOut(cv, B, C);
}

DllExport int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C)
{
	if (Funcs.CommBinaryBuffOut == NULL) {
		return 0;
	}
	return Funcs.CommBinaryBuffOut(cv, B, C);
}

DllExport int WINAPI CommTextOutW(PComVar cv, const wchar_t *B, int C)
{
	if (Funcs.CommTextOutW == NULL) {
		return 0;
	}
	return Funcs.CommTextOutW(cv, B, C);
}

DllExport int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C)
{
	if (Funcs.CommBinaryEcho == NULL) {
		return 0;
	}
	return Funcs.CommBinaryEcho(cv, B, C);
}

DllExport int WINAPI CommTextEchoW(PComVar cv, const wchar_t *B, int C)
{
	if (Funcs.CommTextEchoW == NULL) {
		return 0;
	}
	return Funcs.CommTextEchoW(cv, B, C);
}

} // extern "C"
