/*
 * Copyright (C) 2018-2020 TeraTerm Project
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
#include <assert.h>

/**
 *	win16_lcreat() の wchar_t版
 *	@param[in]	iAttribute	teratermでは0しか使用しない
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lcreatW(const wchar_t *FileName, int iAttribute)
{
	HANDLE handle;
	assert(iAttribute == 0);
	handle = CreateFileW(FileName,
						 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return handle;
}

/**
 *	@param[in]	iAttribute	teratermでは0しか使用しない
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lcreat(const char *FileName, int iAttribute)
{
	HANDLE handle;
	assert(iAttribute == 0);
	handle = CreateFileA(FileName,
						 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return handle;
}

/**
 *	win16_lopen() の wchar_t 版
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lopenW(const wchar_t *FileName, int iReadWrite)
{
	HANDLE handle;
	switch(iReadWrite) {
	case OF_READ:
		// read only
		handle = CreateFileW(FileName,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_WRITE:
		// write
		handle = CreateFileW(FileName,
							 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	default:
		assert(FALSE);
		handle = INVALID_HANDLE_VALUE;
		break;
	}
	return handle;
}

/**
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lopen(const char *FileName, int iReadWrite)
{
	HANDLE handle;
	switch(iReadWrite) {
	case OF_READ:
		// read only
		handle = CreateFileA(FileName,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_WRITE:
		// write
		handle = CreateFileA(FileName,
							 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	default:
		assert(FALSE);
		handle = INVALID_HANDLE_VALUE;
		break;
	}
	return handle;
}

/**
 *	@retval なし
 *			(実際のAPIはオープンしていたHFILEを返す)
 */
void win16_lclose(HANDLE hFile)
{
	CloseHandle(hFile);
}

/**
 *	@retval 読み込みバイト数
 */
UINT win16_lread(HANDLE hFile, LPVOID lpBuffer, UINT uBytes)
{
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

/**
 *	@retval 書き込みバイト数
 */
UINT win16_lwrite(HANDLE hFile, const char*buf, UINT length)
{
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

/*
 *	@param[in]	iOrigin
 *				@arg 0(FILE_BEGIN)
 *				@arg 1(FILE_CURRENT)
 *				@arg 2(FILE_END)
 *	@retval ファイル位置
 *	@retval HFILE_ERROR((HFILE)-1)	エラー
 *	@retval INVALID_SET_FILE_POINTER((DWORD)-1) エラー
 */
LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin)
{
	DWORD pos = SetFilePointer(hFile, lOffset, NULL, iOrigin);
	if (pos == INVALID_SET_FILE_POINTER) {
		return HFILE_ERROR;
	}
	return pos;
}
