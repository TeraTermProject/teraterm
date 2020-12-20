/*
 * (C) 2020 TeraTerm Project
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
#include <stdio.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <assert.h>

#include "filesys_io.h"
#include "filesys_win32.h"

#include "tttypes.h"
#include "layer_for_unicode.h"
#include "codeconv.h"
#include "ftlib.h"

typedef struct FileIOWin32 {
	HANDLE FileHandle;
	BOOL utf8;
} TFileIOWin32;

static wc GetFilenameW(TFileIOWin32 *data, const char *filename)
{
	wc filenameW;
	if (data->utf8) {
		filenameW = wc::fromUtf8(filename);
	}
	else {
		filenameW = wc(filename);
	}
	return filenameW;
}

static BOOL _OpenRead(TFileIO *fv, const char *filename)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	wc filenameW = GetFilenameW(data, filename);
	HANDLE hFile = _CreateFileW(filenameW,
							   GENERIC_READ, FILE_SHARE_READ, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		data->FileHandle = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	data->FileHandle = hFile;
	return TRUE;
}

static BOOL _OpenWrite(TFileIO *fv, const char *filename)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	wc filenameW = GetFilenameW(data, filename);
	HANDLE hFile = _CreateFileW(filenameW,
								GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
								CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		data->FileHandle = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	data->FileHandle = hFile;
	return TRUE;
}

static size_t _ReadFile(TFileIO *fv, void *buf, size_t bytes)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	HANDLE hFile = data->FileHandle;
	DWORD NumberOfBytesRead;
	BOOL result = ReadFile(hFile, buf, (UINT)bytes, &NumberOfBytesRead, NULL);
	assert(result != 0);
	if (result == 0) {
		return 0;
	}
	return NumberOfBytesRead;
}

static size_t _WriteFile(TFileIO *fv, const void *buf, size_t bytes)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	HANDLE hFile = data->FileHandle;
	DWORD NumberOfBytesWritten;
	UINT length = (UINT)bytes;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	assert(result != 0);
	if (result == 0) {
		return 0;
	}
	return NumberOfBytesWritten;
}

static void _Close(TFileIO *fv)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	if (data->FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(data->FileHandle);
		data->FileHandle = INVALID_HANDLE_VALUE;
	}
}

/**
 *	ファイルのファイルサイズを取得
 *	@param[in]	filename		ファイル名(UTF-8)
 *	@retval		ファイルサイズ
 */
static size_t _GetFSize(TFileIO *fv, const char *filename)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	wc filenameW = GetFilenameW(data, filename);
	size_t file_size = GetFSize64W(filenameW);
	return file_size;
}

/**
 *	@retval	0	ok
 *	@retval	-1	error
 * TODO size_t 以上のファイルの扱い
 *
 */
static int Seek(TFileIO *fv, size_t offset)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
#if _M_X64
	// sizeof(size_t) == 8
	LONG lo = (LONG)((offset >> 0) & 0xffffffff);
	LONG hi = (LONG)((offset >> 32) & 0xffffffff);
#else
	// sizeof(size_t) == 4
	LONG lo = (LONG)((offset >> 0) & 0xffffffff);
	LONG hi = 0;
#endif
	SetFilePointer(data->FileHandle, lo, &hi, 0);
	if (GetLastError() != 0) {
		return -1;
	}
	return 0;
}

static int __stat(TFileIO *fv, const char *filename, struct _stati64* _Stat)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	wc filenameW = GetFilenameW(data, filename);
	return _wstati64(filenameW, _Stat);
}

static int __utime(TFileIO *fv, const char *filename, struct _utimbuf* const _Time)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	wc filenameW = GetFilenameW(data, filename);
	return _wutime(filenameW, _Time);
}

// replace ' ' by '_' in FName
static void FTConvFName(PCHAR FName)
{
	int i;

	i = 0;
	while (FName[i]!=0)
	{
		if (FName[i]==' ')
			FName[i] = '_';
		i++;
	}
}

/**
 *	送信用ファイル名作成
 *	fullname(fullpath)からファイル名を取り出して必要な変換を行う
 *	Windowsファイル名から送信に適したファイル名に変換する
 *
 *	@param[in]	fullname	Tera Termが送信するファイル UTF8(となる予定)
 *	@param[in]	utf8		出力ファイル名のエンコード
 *							TRUEのとき、UTF-8
 *							FALSEのとき、ANSI
 *	@retval		ファイル名 (UTF-8 or ANSI)
 *				不要になったら free() する
 */
static char *GetSendFilename(TFileIO *fv, const char *fullname, BOOL utf8, BOOL space, BOOL upper)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	int FnPos;
	char *filename;
	if (data->utf8) {
		GetFileNamePosU8(fullname, NULL, &FnPos);
		if (utf8) {
			// UTF8 -> UTF8
			filename = _strdup(fullname + FnPos);
		}
		else {
			// UTF8 -> ANSI
			filename = ToCharU8(fullname + FnPos);
		}
	}
	else {
		GetFileNamePos(fullname, NULL, &FnPos);
		if (utf8) {
			// ANSI -> UTF8
			filename = ToU8A(fullname + FnPos);
		}
		else {
			// ANSI -> ANSI
			filename = _strdup(fullname + FnPos);
		}
	}
	if (space) {
		// replace ' ' by '_' in filename
		FTConvFName(filename);
	}
	if (upper) {
		_strupr(filename);
	}
	return filename;
}

static char *CreateFilenameWithNumber(const char *fullpath, int n)
{
	int FnPos;
	char Num[11];

	size_t len = strlen(fullpath) + sizeof(Num);
	char *new_name = (char *)malloc(len);

//	GetFileNamePos(fullpath, NULL, &FnPos);
	GetFileNamePosU8(fullpath, NULL, &FnPos);

	_snprintf_s(Num,sizeof(Num),_TRUNCATE,"%u",n);
	size_t num_len = strlen(Num);

	const char *filename = &fullpath[FnPos];
	size_t filename_len = strlen(filename);
	const char *ext = strrchr(filename, '.');
	if (ext == NULL) {
		// 拡張子なし
		char *d = new_name;
		memcpy(d, fullpath, FnPos + filename_len);
		d += FnPos + filename_len;
		memcpy(d, Num, num_len);
		d += num_len;
		*d = 0;
	}
	else {
		// 拡張子あり
		size_t ext_len = strlen(ext);
		size_t base_len = filename_len - ext_len;

		char *d = new_name;
		memcpy(d, fullpath, FnPos + base_len);
		d += FnPos + base_len;
		memcpy(d, Num, num_len);
		d += num_len;
		memcpy(d, ext, ext_len);
		d += ext_len;
		*d = 0;
	}
	return new_name;
}

static char *CreateUniqueFilename(const char *fullpath)
{
	int i = 1;
	while(1) {
		char *filename = CreateFilenameWithNumber(fullpath, i);
		if (!DoesFileExist(filename)) {
			return filename;
		}
		free(filename);
		i++;
	}
}

/**
 *	受信用ファイル名作成
 *	fullnameから必要な変換を行う
 *	送られてきたファイル名がANSIかUTF8かは
 *	通信先やプロトコルによると思われる
 *	受信したファイル名からWindowsに適したファイル名に変換する
 *
 *	@param[in]	filename	通信で送られてきたファイル名, 入力ファイル名
 *	@param[in]	utf8		入力ファイル名のエンコード
 *							TRUEのとき、UTF-8
 *							FALSEのとき、ANSI
 *	@param[in]	path		受信フォルダ ファイル名の前に付加される UTF-8
 *							NULLのとき付加されない
 *	@param[in]	unique		TRUEのとき
 *							ファイルが存在したときファイル名の後ろに数字を追加する
 *	@retval		ファイル名 UTF-8
 *				不要になったら free() する
 */
static char* GetRecieveFilename(struct FileIO* fv, const char* filename, BOOL utf8, const char *path, BOOL unique)
{
	char* new_name;
	if (utf8) {
		// UTF8 -> UTF8
		int FnPos;
		GetFileNamePosU8(filename, NULL, &FnPos);
		new_name = _strdup(&filename[FnPos]);
	}
	else {
		// ANSI -> UTF8
		int FnPos;
		GetFileNamePos(filename, NULL, &FnPos);
		new_name = ToU8A(&filename[FnPos]);
	}
	size_t len = strlen(new_name) + 1;
	FitFileName(new_name, len, NULL);
	replaceInvalidFileNameChar(new_name, '_');

	// to fullpath
	char *full;
	if (path == NULL) {
		full = new_name;
	}
	else {
		size_t full_len = len + strlen(path);
		full = (char *)malloc(full_len);
		strcpy(full, path);
		strcat(full, new_name);
	}

	// to unique
	if (unique) {
		char *filename = CreateUniqueFilename(full);
		free(full);
		full = filename;
	}

	return full;
}

static long GetFMtime(TFileIO *fv, const char *FName)
{
	struct _stat st;

	if (_stat(FName,&st)==-1) {
		return 0;
	}
	return (long)st.st_mtime;
}

static void FileSysDestroy(TFileIO *fv)
{
	TFileIOWin32 *data = (TFileIOWin32 *)fv->data;
	fv->Close(fv);
	free(data);
	fv->data = NULL;
	free(fv);
}

TFileIO *FilesysCreateWin32(void)
{
	TFileIOWin32 *data = (TFileIOWin32 *)calloc(sizeof(TFileIOWin32), 1);
	if (data == NULL) {
		return NULL;
	}
	data->FileHandle = INVALID_HANDLE_VALUE;
	data->utf8 = TRUE;
	TFileIO *fv = (TFileIO *)calloc(sizeof(TFileIO), 1);
	if (fv == NULL) {
		free(data);
		return NULL;
	}
	fv->data = data;
	fv->OpenRead = _OpenRead;
	fv->OpenWrite = _OpenWrite;
	fv->ReadFile = _ReadFile;
	fv->WriteFile = _WriteFile;
	fv->Close = _Close;
	fv->Seek = Seek;
	fv->GetFSize = _GetFSize;
	fv->utime = __utime;
	fv->stat = __stat;
	fv->FileSysDestroy = FileSysDestroy;
	fv->GetSendFilename = GetSendFilename;
	fv->GetRecieveFilename = GetRecieveFilename;
	fv->GetFMtime = GetFMtime;
	return fv;
}
