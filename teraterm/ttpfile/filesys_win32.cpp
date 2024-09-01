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
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "filesys_io.h"
#include "filesys_win32.h"

#include "tttypes.h"
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
	HANDLE hFile = CreateFileW(filenameW,
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
	HANDLE hFile = CreateFileW(filenameW,
							   GENERIC_WRITE, 0, NULL,
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
 *	�t�@�C���̃t�@�C���T�C�Y���擾
 *	@param[in]	filename		�t�@�C����(UTF-8)
 *	@retval		�t�@�C���T�C�Y
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
 * TODO size_t �ȏ�̃t�@�C���̈���
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
 *	���M�p�t�@�C�����쐬
 *	fullname(fullpath)����t�@�C���������o���ĕK�v�ȕϊ����s��
 *	Windows�t�@�C�������瑗�M�ɓK�����t�@�C�����ɕϊ�����
 *
 *	@param[in]	fullname	Tera Term�����M����t�@�C�� UTF8(�ƂȂ�\��)
 *	@param[in]	utf8		�o�̓t�@�C�����̃G���R�[�h
 *							TRUE�̂Ƃ��AUTF-8
 *							FALSE�̂Ƃ��AANSI
 *	@retval		�t�@�C���� (UTF-8 or ANSI)
 *				�s�v�ɂȂ����� free() ����
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
		// �g���q�Ȃ�
		char *d = new_name;
		memcpy(d, fullpath, FnPos + filename_len);
		d += FnPos + filename_len;
		memcpy(d, Num, num_len);
		d += num_len;
		*d = 0;
	}
	else {
		// �g���q����
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
 *	��M�p�t�@�C�����쐬
 *	fullname����K�v�ȕϊ����s��
 *	�����Ă����t�@�C������ANSI��UTF8����
 *	�ʐM���v���g�R���ɂ��Ǝv����
 *	��M�����t�@�C��������Windows�ɓK�����t�@�C�����ɕϊ�����
 *
 *	@param[in]	filename	�ʐM�ő����Ă����t�@�C����, ���̓t�@�C����
 *	@param[in]	utf8		���̓t�@�C�����̃G���R�[�h
 *							TRUE�̂Ƃ��AUTF-8
 *							FALSE�̂Ƃ��AANSI
 *	@param[in]	path		��M�t�H���_ �t�@�C�����̑O�ɕt������� UTF-8
 *							NULL�̂Ƃ��t������Ȃ�
 *	@param[in]	unique		TRUE�̂Ƃ��A���łɃt�@�C�������݂��Ă��邩�`�F�b�N����
 *							�t�@�C�������݂����Ƃ��A�t�@�C�����̌��ɐ�����ǉ�����
 *	@retval		�t�@�C���� UTF-8
 *				�s�v�ɂȂ����� free() ����
 */
static char* GetRecieveFilename(struct FileIO* fv, const char* filename, BOOL utf8, const char *path, BOOL unique)
{
	char* new_name = NULL;
	if (!utf8) {
		// ANSI -> UTF8
		int FnPos;
		GetFileNamePos(filename, NULL, &FnPos);
		new_name = ToU8A(&filename[FnPos]);
		// new_name == NULL �̂Ƃ� UTF-8�ɕϊ��ł��Ȃ�����
	}
	if (new_name == NULL) {
		// UTF8 -> UTF8
		int FnPos;
		GetFileNamePosU8(filename, NULL, &FnPos);
		new_name = _strdup(&filename[FnPos]);
	}
	if (new_name == NULL) {
		return NULL;
	}
	size_t len = strlen(new_name) + 1;
	FitFileName(new_name, len, NULL);
	char *new_name_safe = replaceInvalidFileNameCharU8(new_name, '_');
	free(new_name);
	new_name = NULL;

	// to fullpath
	char *full;
	if (path == NULL) {
		full = new_name_safe;
	}
	else {
		size_t full_len = strlen(new_name_safe) + strlen(path) + 1;
		full = (char *)malloc(full_len);
		strcpy(full, path);
		strcat(full, new_name_safe);
	}
	free(new_name_safe);
	new_name_safe = NULL;

	// to unique
	if (unique && DoesFileExist(full)) {
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

static BOOL _SetFMtime(TFileIO *fv, const char *FName, DWORD mtime)
{
	struct _utimbuf filetime;

	filetime.actime = mtime;
	filetime.modtime = mtime;
	return _utime(FName, &filetime);
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
	TFileIOWin32 *data = (TFileIOWin32 *)calloc(1, sizeof(TFileIOWin32));
	if (data == NULL) {
		return NULL;
	}
	data->FileHandle = INVALID_HANDLE_VALUE;
	data->utf8 = TRUE;
	TFileIO *fv = (TFileIO *)calloc(1, sizeof(TFileIO));
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
	fv->SetFMtime = _SetFMtime;
	return fv;
}
