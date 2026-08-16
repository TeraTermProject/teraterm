/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

#include <string.h>
#include <crtdbg.h>

#include "tttypes.h"
#include "makeoutputstring.h"

#include "ttcommon.h"

int WINAPI CommReadRawByte(PComVar cv, LPBYTE b)
{
	if ( ! cv->Ready ) {
		return 0;
	}

	if ( cv->InBuffCount>0 ) {
		*b = cv->InBuff[cv->InPtr];
		cv->InPtr++;
		cv->InBuffCount--;
		if ( cv->InBuffCount==0 ) {
			cv->InPtr = 0;
		}
		return 1;
	}
	else {
		cv->InPtr = 0;
		return 0;
	}
}

static void LogBinSkip(PComVar cv, int add)
{
	if (cv->LogBinSkip != NULL) {
		cv->LogBinSkip(add);
	}
}

/**
 *	入力バッファの先頭にデータを入れる
 *	入れたデータはバイナリログに記録されない
 *
 *	@param	b	入れるデータ
 */
void WINAPI CommInsert1Byte(PComVar cv, BYTE b)
{
	if ( ! cv->Ready ) {
		return;
	}

	if (cv->InPtr == 0) {
		memmove(&(cv->InBuff[1]),&(cv->InBuff[0]),cv->InBuffCount);
	}
	else {
		cv->InPtr--;
	}
	cv->InBuff[cv->InPtr] = b;
	cv->InBuffCount++;

	LogBinSkip(cv, 1);
}

static void Log1Bin(PComVar cv, BYTE b)
{
	if (cv->Log1Bin != NULL) {
		cv->Log1Bin(b);
	}
}

int WINAPI CommRead1Byte(PComVar cv, LPBYTE b)
{
	int c;

	if ( ! cv->Ready ) {
		return 0;
	}

	if ( cv->TelMode ) {
		c = 0;
	}
	else {
		c = CommReadRawByte(cv,b);
	}

	if ((c==1) && cv->TelCRFlag) {
		cv->TelCRFlag = FALSE;
		if (*b==0) {
			c = 0;
		}
	}

	if ( c==1 ) {
		if ( cv->IACFlag ) {
			cv->IACFlag = FALSE;
			if ( *b != 0xFF ) {
				cv->TelMode = TRUE;
				CommInsert1Byte(cv,*b);
				LogBinSkip(cv, -1);
				c = 0;
			}
		}
		else if ((cv->PortType==IdTCPIP) && (*b==0xFF)) {
			if (!cv->TelFlag && cv->TelAutoDetect) { /* TTPLUG */
				cv->TelFlag = TRUE;
			}
			if (cv->TelFlag) {
				cv->IACFlag = TRUE;
				c = 0;
			}
		}
		else if (cv->TelFlag && ! cv->TelBinRecv && (*b==0x0D)) {
			cv->TelCRFlag = TRUE;
		}
	}

	if (c == 1) {
		Log1Bin(cv, *b);
	}

	return c;
}

int WINAPI CommRawOut(PComVar cv, /*const*/ PCHAR B, int C)
{
	int a;

	if ( ! cv->Ready ) {
		return C;
	}

	if (C > OutBuffSize - cv->OutBuffCount) {
		a = OutBuffSize - cv->OutBuffCount;
	}
	else {
		a = C;
	}
	if ( cv->OutPtr > 0 ) {
		memmove(&(cv->OutBuff[0]),&(cv->OutBuff[cv->OutPtr]),cv->OutBuffCount);
		cv->OutPtr = 0;
	}
	memcpy(&(cv->OutBuff[cv->OutBuffCount]),B,a);
	cv->OutBuffCount = cv->OutBuffCount + a;
	return a;
}

int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready ) {
		return C;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') && ! cv->TelBinSend ) {
			d[Len++] = '\x00';
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if ( OutBuffSize - cv->OutBuffCount - Len >= 0 ) {
			CommRawOut(cv, d, Len);
			a = 1;
		}
		else {
			a = 0;
		}

		i += a;
	}
	return i;
}

/**
 *	データ(文字列)を出力バッファへ書き込む
 *
 *	指定データがすべて書き込めない場合は書き込まない
 *	CommRawOut() は書き込める分だけ書き込む
 *
 *	@retval	TRUE	出力できた
 *	@retval	FALSE	出力できなかった(buffer full)
 */
static BOOL WriteOutBuff(PComVar cv, const char *TempStr, int TempLen)
{
	BOOL output;

	if (TempLen == 0) {
		// 長さ0で書き込みに来る場合あり
		return TRUE;
	}

	output = FALSE;
	if (cv->TelLineMode) {
		const BOOL Full = OutBuffSize - cv->LineModeBuffCount - TempLen < 0;
		if (!Full) {
			output = TRUE;
			memcpy(&(cv->LineModeBuff[cv->LineModeBuffCount]), TempStr, TempLen);
			cv->LineModeBuffCount += TempLen;
			if (cv->Flush) {
				cv->FlushLen = cv->LineModeBuffCount;
			}
		}
		if (cv->FlushLen > 0) {
			const int OutLen = CommRawOut(cv, cv->LineModeBuff, cv->FlushLen);
			cv->FlushLen -= OutLen;
			cv->LineModeBuffCount -= OutLen;
			memmove(cv->LineModeBuff, &(cv->LineModeBuff[OutLen]), cv->LineModeBuffCount);
		}
		cv->Flush = FALSE;
	}
	else {
		const BOOL Full = OutBuffSize-cv->OutBuffCount-TempLen < 0;
		if (! Full) {
			output = TRUE;
			CommRawOut(cv, (char *)TempStr, TempLen);
		}
	}
	return output;
}

/**
 *	データ(文字列)を入力バッファへ書き込む
 *	入力バッファへ入れる -> エコーされる
 *
 *	@retval	TRUE	出力できた
 *	@retval	FALSE	出力できなかった(buffer full)
 */
static BOOL WriteInBuff(PComVar cv, const char *TempStr, int TempLen)
{
	if (TempLen == 0) {
		return TRUE;
	}

    if (InBuffSize - cv->InPtr - cv->InBuffCount < TempLen) {
        return FALSE;
	}

	memcpy(&(cv->InBuff[cv->InPtr + cv->InBuffCount]), TempStr, TempLen);
	cv->InBuffCount += TempLen;
	return TRUE;
}

/**
 *	入力バッファの先頭に空きがあったら詰める
 */
static void PackInBuff(PComVar cv)
{
	if (cv->InBuffCount == 0) {
		cv->InPtr = 0;
	} else if (cv->InPtr > 0 && cv->InPtr + cv->InBuffCount > InBuffSize / 4 * 3) {
		memmove(cv->InBuff, &(cv->InBuff[cv->InPtr]), cv->InBuffCount);
		cv->InPtr = 0;
	}
}

int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready ) {
		return C;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if (B[i] == CR) {
			if ( cv->TelFlag && ! cv->TelBinSend ) {
				d[Len++] = '\x00';
			}
			if (cv->TelLineMode) {
				cv->Flush = TRUE;
			}
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if (WriteOutBuff(cv, d, Len)) {
			a = 1;
			i++;
		} else {
			a = 0;
		}
	}
	return i;
}

/**
 *	出力用
 *	@param	cv
 *	@param	u32			入力文字
 *	@param	check_only	TRUEで処理は行わず、
 *	@param	TempStr		出力文字数
 *	@param	StrLen		TempStrへの出力文字数
 *	@retval	処理を行った
 */
static BOOL OutControl(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data)
{
	PComVar cv = data;
	const wchar_t d = u32;
	size_t TempLen = 0;
	BOOL retval = FALSE;
	if (check_only == TRUE) {
		/* チェックのみ */
		if (d == CR || d == BS || d == 0x15/*ctrl-u*/) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	if (d==CR) {
		TempStr[TempLen++] = 0x0d;
		if (cv->CRSend==IdCRLF) {
			TempStr[TempLen++] = 0x0a;
		}
		else if ((cv->CRSend ==IdCR) &&
				 cv->TelFlag && ! cv->TelBinSend) {
			TempStr[TempLen++] = 0;
		}
		else if (cv->CRSend == IdLF) {
			TempStr[TempLen-1] = 0x0a;
		}
		if (cv->TelLineMode) {
			cv->Flush = TRUE;
		}
		retval = TRUE;
	}
	else if (d== BS) {
		if (cv->TelLineMode) {
			if (cv->FlushLen < cv->LineModeBuffCount) {
				cv->LineModeBuffCount--;
			}
		}
		else {
			TempStr[TempLen++] = BS;
		}
		retval = TRUE;
	}
	else if (d==0x15) { // ctrl-u
		if (cv->TelLineMode) {
			cv->LineModeBuffCount = cv->FlushLen;
		}
		else {
			TempStr[TempLen++] = 0x15;
		}
		retval = TRUE;
	}
	*StrLen = TempLen;
	return retval;
}
static BOOL ControlEcho(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data)
{
	PComVar cv = data;
	const wchar_t d = u32;
	size_t TempLen = 0;
	BOOL retval = FALSE;
	if (check_only == TRUE) {
		/* チェックのみ */
		if (d == CR || (d == 0x15/*ctrl-u*/ && cv->TelLineMode)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	if (d==CR) {
		TempStr[TempLen++] = 0x0d;
		if (cv->CRSend==IdCRLF) {
			TempStr[TempLen++] = 0x0a;
		}
		else if ((cv->CRSend ==IdCR) && cv->TelFlag && ! cv->TelBinSend) {
			TempStr[TempLen++] = 0;
		}
		else if (cv->CRSend == IdLF) {
			TempStr[TempLen-1] = 0x0a;
		}
		retval = TRUE;
	}
	else if (d==0x15/*ctrl-u*/ && cv->TelLineMode) {
		// Move to top of line (CHA "\033[G") and erase line (EL "\033[K")
		memcpy(TempStr, "\033[G\033[K", 6);
		TempLen += 6;
		retval = TRUE;
	}
	*StrLen = TempLen;
	return retval;
}

/**
 * CommTextOut() の wchar_t 版
 *
 *	@retval		出力文字数(wchar_t単位)
 */
int WINAPI CommTextOutW(PComVar cv, const wchar_t *B, int C)
{
	char TempStr[12];
	BOOL Full = FALSE;
	int i = 0;
	while (! Full && (i < C)) {
		// 出力用データを作成
		size_t TempLen = 0;
		size_t output_char_count;	// 消費した文字数
		output_char_count = MakeOutputString(cv->StateSend, &B[i], C - i, TempStr, &TempLen, OutControl, cv);

		// データを出力バッファへ
		if (WriteOutBuff(cv, TempStr, TempLen)) {
			i += output_char_count;		// output_char_count 文字数 処理した
		} else {
			Full = TRUE;
		}
	} // end of "while {}"
	_CrtCheckMemory();
	return i;
}

/**
 * CommTextEcho() の wchar_t 版
 *
 *	@retval		出力文字数(wchar_t単位)
 */
int WINAPI CommTextEchoW(PComVar cv, const wchar_t *B, int C)
{
	char TempStr[12];
	BOOL Full = FALSE;
	int i = 0;
	while (! Full && (i < C)) {
		// 出力用データを作成
		size_t TempLen = 0;
		size_t output_char_count;	// 消費した文字数
		output_char_count = MakeOutputString(cv->StateEcho, &B[i], C - i, TempStr, &TempLen, ControlEcho, cv);

		// データを出力バッファへ
		if (WriteInBuff(cv, TempStr, TempLen)) {
			i += output_char_count;		// output_char_count 文字数 処理した
		} else {
			Full = TRUE;
		}
	} // end of "while {}"
	_CrtCheckMemory();
	return i;
}

int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready )
		return C;

	PackInBuff(cv);

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') &&
		     ! cv->TelBinSend ) {
			d[Len] = 0x00;
			Len++;
		}

		if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len] = '\xff';
			Len++;
		}

		if (WriteInBuff(cv, d, Len)) {
			a = 1;
			i++;
		} else {
			a = 0;
		}
	}
	return i;
}
