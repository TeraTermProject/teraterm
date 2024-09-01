/*
 * Copyright (C) 2021- TeraTerm Project
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

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "codeconv.h"

#include "inifile_com.h"

/**
 *	GetPrivateProfileStringA() �̃t�@�C���������� wchar_t ��
 */
DWORD GetPrivateProfileStringAFileW(const char *appA, const char *keyA, const char* defA, char *strA, DWORD size, const wchar_t *filenameW)
{
	DWORD lenA;
	wchar_t *appW = ToWcharA(appA);
	wchar_t *keyW = ToWcharA(keyA);
	wchar_t *defW = ToWcharA(defA);
	DWORD lenW_max = size;
	wchar_t *strW = (wchar_t *)malloc(sizeof(wchar_t) * lenW_max);
	DWORD lenW = GetPrivateProfileStringW(appW, keyW, defW, strW, lenW_max, filenameW);
	free(appW);
	free(keyW);
	free(defW);
	if (lenW == 0) {
		free(strW);
		*strA = '\0';
		return 0;
	}
	if (lenW < lenW_max) {
		lenW++;	// for L'\0'
	}
	lenA = WideCharToMultiByte(CP_ACP, 0, strW, lenW, strA, size, NULL, NULL);
	// GetPrivateProfileStringW() �̖߂�l�� '\0' ���܂܂Ȃ�������
	// WideCharToMultiByte() �̖߂�l�� '\0' ���܂ޕ�����
	if (lenW != 0 && strA[lenA-1] == 0) {
		lenA--;
	}
	free(strW);
	return lenA;
}

/**
 *	WritePrivateProfileStringA() �̃t�@�C���������� wchar_t ��
 */
BOOL WritePrivateProfileStringAFileW(const char *appA, const char *keyA, const char *strA, const wchar_t *filenameW)
{
	wchar_t *appW = ToWcharA(appA);
	wchar_t *keyW = ToWcharA(keyA);
	wchar_t *strW = ToWcharA(strA);
	BOOL r = WritePrivateProfileStringW(appW, keyW, strW, filenameW);
	free(appW);
	free(keyW);
	free(strW);
	return r;
}

/**
 *	GetPrivateProfileIntFileA() �̃t�@�C���������� wchar_t ��
 */
UINT GetPrivateProfileIntAFileW(const char *appA, const char *keyA, int def, const wchar_t *filenameW)
{
	wchar_t *appW = ToWcharA(appA);
	wchar_t *keyW = ToWcharA(keyA);
	UINT r = GetPrivateProfileIntW(appW, keyW, def, filenameW);
	free(appW);
	free(keyW);
	return r;
}

BOOL WritePrivateProfileIntW(const wchar_t *app, const wchar_t *key, int val, const wchar_t *filenameW)
{
	// int�ł�12�������ok			123456789012
	// INT_MIN(int�ŏ��l)			-2147483648
	// INT_MAX(int�ő�l)			2147483647
	wchar_t str[16];
	_snwprintf_s(str, _countof(str), _TRUNCATE, L"%d", val);
	return WritePrivateProfileStringW(app, key, str, filenameW);
}

/**
 *	WritePrivateProfileIntA() �̃t�@�C���������� wchar_t ��
 */
BOOL WritePrivateProfileIntAFileW(const char *appA, const char *keyA, int val, const wchar_t *filenameW)
{
	wchar_t strW[MAX_PATH];
	wchar_t *appW = ToWcharA(appA);
	wchar_t *keyW = ToWcharA(keyA);
	BOOL r;
	_snwprintf_s(strW, _countof(strW), _TRUNCATE, L"%d", val);
	r = WritePrivateProfileStringW(appW, keyW, strW, filenameW);
	free(appW);
	free(keyW);
	return r;
}

/**
 *	�t�@�C�����Ȃ��Ȃ� BOM ��������������
 *
 *	@param	filename
 *	@param	overwrite	TRUE	�㏑������
 *						FALSE	�t�@�C��������΂Ȃɂ����Ȃ�
 */
void WriteIniBom(const wchar_t *filename, BOOL overwrite)
{
	if (!overwrite) {
		const DWORD attr = GetFileAttributesW(filename);
		if (attr != INVALID_FILE_ATTRIBUTES) {
			// �t�@�C��������Ȃ�Ȃɂ����Ȃ�
			return;
		}
	}

	FILE *fp;
	errno_t e = _wfopen_s(&fp, filename, L"wb");
	if (e == 0 && fp != NULL) {
		const char *bom = "\xff\xfe";
		fwrite(bom, 1, 2, fp);
		fclose(fp);
	}
}
