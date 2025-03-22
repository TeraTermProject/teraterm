/*
 * Copyright (C) 2020- TeraTerm Project
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

/* ttlib_static_cpp ���番�� */
/* �t�H���_�Ɋւ���֐��A*/
/* cyglaunch�Ŏg�� TTWinExec() */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <wchar.h>
#include <shlobj.h>

// compat_win �𗘗p����
//		cyglaunch �̒P�ƃr���h�̂Ƃ��Acompatwin ���g�p�����r���h����
#if !defined(ENABLE_COMAPT_WIN)
#define ENABLE_COMAPT_WIN	1
#endif

#include "asprintf.h"
#include "win32helper.h"	// for hGetModuleFileNameW()
#include "ttknownfolders.h"

#if ENABLE_COMAPT_WIN
#include "compat_win.h"
#endif

#include "ttlib_static_dir.h"

// �|�[�^�u���łƂ��ē��삷�邩�����߂�t�@�C��
#define PORTABLE_FILENAME L"portable.ini"

/**
 *	AppData�t�H���_�̎擾
 *	���ϐ� APPDATA �̃t�H���_
 *
 *	@retval	AppData�t�H���_
 */
#if ENABLE_COMAPT_WIN
static wchar_t *GetAppdataDir(void)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &path);
	return path;
}
#else
static wchar_t *GetAppdataDir(void)
{
#if _WIN32_WINNT > 0x0600
	wchar_t *appdata;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &appdata);
	wchar_t *retval = _wcsdup(appdata);
	CoTaskMemFree(appdata);
	return retval;
#else
	LPITEMIDLIST pidl;
	HRESULT r = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
	if (r == NOERROR) {
		wchar_t appdata[MAX_PATH];
		SHGetPathFromIDListW(pidl, appdata);
		wchar_t *retval = wcsdup(appdata);
		CoTaskMemFree(pidl);
		return retval;
	}
	char *env = getenv("APPDATA");
	if (env == NULL) {
		// �����ƌÂ� windows ?
		abort();
	}
	wchar_t *appdata = ToWcharA(env);
	return appdata;
#endif
}
#endif

/**
 *	LocalAppData�t�H���_�̎擾
 *	���ϐ� LOCALAPPDATA �̃t�H���_
 *
 *	@retval	LocalAppData�t�H���_
 */
#if ENABLE_COMAPT_WIN
static wchar_t *GetLocalAppdataDir(void)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &path);
	return path;
}
#endif

/*
 * Get Exe(exe,dll) directory
 *	ttermpro.exe, �v���O�C��������t�H���_
 *	ttypes.ExeDirW �Ɠ���
 *	���Ƃ� GetHomeDirW() ������
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			ExeDir		�s�v�ɂȂ����� free() ���邱��
 *								������̍Ō�Ƀp�X��؂�('\')�͂��Ă��Ȃ�
 */
wchar_t *GetExeDirW(HINSTANCE hInst)
{
	wchar_t *dir;
	DWORD error = hGetModuleFileNameW(hInst, &dir);
	if (error != NO_ERROR) {
		// �p�X�̎擾�Ɏ��s�����B�v���I�Aabort() ����B
		abort();
	}
	wchar_t *sep = wcsrchr(dir, L'\\');
	*sep = 0;
	return dir;
}

/**
 *	�|�[�^�u���łƂ��ē��삷�邩
 *
 *	@retval		TRUE		�|�[�^�u����
 *	@retval		FALSE		�ʏ�C���X�g�[����
 */
BOOL IsPortableMode(void)
{
	static BOOL called = FALSE;
	static BOOL ret_val = FALSE;
	if (called == FALSE) {
		called = TRUE;
		wchar_t *exe_dir = GetExeDirW(NULL);
		wchar_t *portable_ini = NULL;
		awcscats(&portable_ini, exe_dir, L"\\", PORTABLE_FILENAME, NULL);
		free(exe_dir);
		DWORD r = GetFileAttributesW(portable_ini);
		free(portable_ini);
		if (r == INVALID_FILE_ATTRIBUTES) {
			//�t�@�C�������݂��Ȃ�
			ret_val = FALSE;
		}
		else {
			ret_val = TRUE;
		}
	}
	return ret_val;
}

/*
 * Get home directory
 *		�l�p�ݒ�t�@�C���t�H���_�擾
 *		ttypes.HomeDirW �Ɠ���
 *		TERATERM.INI �Ȃǂ������Ă���t�H���_
 *		ttermpro.exe ������t�H���_�� GetHomeDirW() �ł͂Ȃ� GetExeDirW() �Ŏ擾�ł���
 *		ExeDirW �� portable.ini ������ꍇ
 *			ExeDirW
 *		ExeDirW �� portable.ini ���Ȃ��ꍇ
 *			%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			HomeDir		�s�v�ɂȂ����� free() ���邱��
 *								������̍Ō�Ƀp�X��؂�('\')�͂��Ă��Ȃ�
 */
wchar_t *GetHomeDirW(HINSTANCE hInst)
{
	if (IsPortableMode()) {
		return GetExeDirW(hInst);
	}
	else {
		wchar_t *path = GetAppdataDir();
		wchar_t *ret = NULL;
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
		CreateDirectoryW(ret, NULL);
		return ret;
	}
}

/*
 * Get log directory
 *		���O�ۑ��t�H���_�擾
 *		ttypes.LogDirW �Ɠ���
 *		ExeDirW �� portable.ini ������ꍇ
 *			ExeDirW\log
 *		ExeDirW �� portable.ini ���Ȃ��ꍇ
 *			%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			LogDir		�s�v�ɂȂ����� free() ���邱��
 *								������̍Ō�Ƀp�X��؂�('\')�͂��Ă��Ȃ�
 */
wchar_t* GetLogDirW(HINSTANCE hInst)
{
	wchar_t *ret = NULL;
	if (IsPortableMode()) {
		wchar_t *ExeDirW = GetExeDirW(hInst);
		awcscats(&ret, ExeDirW, L"\\log", NULL);
		free(ExeDirW);
	}
	else {
		wchar_t *path = GetLocalAppdataDir();
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
	}
	CreateDirectoryW(ret, NULL);
	return ret;
}

/**
 *	�v���O���������s����
 *
 *	@param[in]	command		���s����R�}���h���C��
 *							CreateProcess() �ɂ��̂܂ܓn�����
 * 	@retval		NO_ERROR	�G���[�Ȃ�
 *	@retval		�G���[�R�[�h	(NO_ERROR�ȊO)
 *
 *	�V���v���Ƀv���O�������N�����邾���̊֐�
 *		CreateProcess() �� CloseHandle() ��Y��ăn���h�����[�N���N�����₷��
 *		�P���ȃv���O�������s�ł͂��̊֐����g�p����ƈ��S
 */
DWORD TTWinExec(const wchar_t *command)
{
	STARTUPINFOW si = {};
	PROCESS_INFORMATION pi = {};

	GetStartupInfoW(&si);

	BOOL r = CreateProcessW(NULL, (LPWSTR)command, NULL, NULL, FALSE, 0,
							NULL, NULL, &si, &pi);
	if (r == 0) {
		// error
		return GetLastError();
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return NO_ERROR;
}
