/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) Robert O'Callahan
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "asprintf.h"
#include "history_store.h"
#include "win32helper.h"

#include "ttplugin.h"
#include "ttplug.h"

typedef struct _ExtensionList {
	wchar_t *path;					// フルパス
	wchar_t *filename;				// ファイル名
	ExtensionEnable enable;
	TTXExports * exports;			// NULLのときはロードされていない
	HMODULE LibHandle;				// NULLのときはロードされていない
} ExtensionList;

static ExtensionList *Extensions = NULL;
static int NumExtensions = 0;

static int compareOrder(const void * e1, const void * e2)
{
	const ExtensionList *exports1 = (ExtensionList *)e1;
	const ExtensionList *exports2 = (ExtensionList *)e2;

	// ロードされている?
	if (exports1->exports != NULL && exports2->exports != NULL) {
		// 両方ロードされている
		int diff = exports1->exports->loadOrder - exports2->exports->loadOrder;
		if (diff != 0) {
			return diff;
		}
	}
	if (!(exports1->exports == NULL && exports2->exports == NULL)) {
		// 片方しかロードされていない
		if (exports1->exports == NULL) {
			// exports1を後ろ
			return 1;
		}
		if (exports2->exports == NULL) {
			// exports2を後ろ
			return -1;
		}
	}

	// loadOrderで判断できないときは、パス順
	return wcscmp(exports1->filename, exports2->filename);
}

/**
 *	ロードするプライグインを追加する
 *
 *	@param	fullpath
 *	@param	enable
 */
static void PluginListAdd(const wchar_t *fullpath, ExtensionEnable enable)
{
	wchar_t *fullpath_normalized;
	wchar_t *filepart;
	DWORD e = hGetFullPathNameW(fullpath, &fullpath_normalized, &filepart);
	if (e != NO_ERROR) {
		return;
	}
	if (filepart == NULL) {
		free(fullpath_normalized);
		return;
	}

	ExtensionList item = {};
	item.path = fullpath_normalized;
	item.filename = filepart;
	item.enable = enable;

	ExtensionList *p = (ExtensionList *)realloc(Extensions, sizeof(ExtensionList) * (NumExtensions + 1));
	if (p == NULL) {
		free(fullpath_normalized);
		free(filepart);
		return;
	}
	Extensions = p;
	Extensions[NumExtensions] = item;
	NumExtensions++;
}

/**
 *	ファイル名を表す文字列が "ttx*.dll" にマッチするか調べる
 *
 *	最短は "ttx.dll"
 *
 *	@retval	TRUE	マッチした
 *	@retval	FALSE	マッチしない
 */
static BOOL MatchTTXFileName(const wchar_t *fname)
{
	const size_t len = wcslen(fname);
	// "ttx" + 0文字以上 + ".dll" = 7文字以上
	if (len < 7) {
		// 7文字未満=マッチしない
		return FALSE;
	}

	// 先頭が "ttx" で始まる
	if (_wcsnicmp(fname, L"ttx", 3) != 0) {
		return FALSE;
	}

	// 末尾が ".dll" (大文字小文字無視)
	if (_wcsicmp(fname + len - 4, L".dll") != 0) {
		return FALSE;
	}

	return TRUE;
}

/**
 *	フルパス(ファイル名のみ)からファイル名を取得
 *
 *	@param	fullpath
 *	@retval	ファイル名へのポインタ(不要になったらfree()すること)
 *	@retval	NULL ファイル名が無かった
 */
static wchar_t *GetFilename(const wchar_t *fullpath)
{
	const wchar_t *filename_c;

	// ファイル名のみにする
	filename_c = wcsrchr(fullpath, L'\\');
	if (filename_c == NULL) {
		// パス区切り '/' も考慮
		filename_c = wcsrchr(fullpath, L'/');
	}

	if (filename_c == NULL) {
		// 元からファイル名だけだった
		wchar_t *filename = _wcsdup(fullpath);
		return filename;
	}

	filename_c++;
	if (*filename_c == 0) {
		// パス区切りが文字端だった
		return NULL;
	}

	wchar_t *filename = _wcsdup(filename_c);
	return filename;
}

/**
 * ファイル名からリストへのポインタを取得
 *
 * @param filename	プラグインのファイル名
 * @retval	ExtensionList へのポインタ
 * @retval	NULL	見つからなかった
 */
static ExtensionList *PluginGetInfo(const wchar_t *filename)
{
	for (int i = 0; i < NumExtensions; i++) {
		if (_wcsicmp(Extensions[i].filename, filename) == 0) {
			ExtensionList *pl = &Extensions[i];
			return pl;
		}
	}

	// 見つからなかった
	return NULL;
}

/**
 *	iniファイルを読んで enable/disable 設定を行う
 *
 *	[Plugin]
 *	list1=file, 1
 *	  :
 *
 *	@param	SetupFNW	iniファイル
 */
static void PluginListRead(const wchar_t *SetupFNW)
{
	HistoryStore *hs = HistoryStoreCreate(0);
	if (hs == NULL) {
		return;
	}
	HistoryStoreReadIni(hs, SetupFNW, L"Plugin", L"list");

	for (size_t i = 0;; i++) {
		const wchar_t *s = HistoryStoreGet(hs, i);
		if (s == NULL) {
			break;
		}
		size_t len_ch = wcslen(s) + 1;
		wchar_t *fname = (wchar_t *)malloc(sizeof(wchar_t) * len_ch);
		if (fname == NULL) {
			continue;
		}
		int enable_int;
		// ""file", 1"  を想定
#if !defined(__MINGW32__)
		int r = swscanf_s(s, L"\"%[^\"]\", %d", fname, (unsigned int)len_ch, &enable_int);
#else
		int r = swscanf(s, L"\"%[^\"]\", %d", fname, &enable_int);
#endif
		if (r == 2) {
			wchar_t *filename = GetFilename(fname);
			ExtensionList *pl = PluginGetInfo(filename);
			if (pl != NULL) {
				pl->enable =
					enable_int == 0 ? EXTENSION_DISABLE :
					enable_int == 1 ? EXTENSION_ENABLE :
					EXTENSION_UNSPECIFIED;
			}
			free(filename);
		}
		free(fname);
	}
	HistoryStoreDestroy(hs);
}

void PluginWriteList(const wchar_t *SetupFNW)
{
	HistoryStore *hs = HistoryStoreCreate(0);
	for (size_t i = 0; i < NumExtensions; i++) {
		const ExtensionList *pl = &Extensions[i];
		if (pl->enable == EXTENSION_ENABLE ||
			pl->enable == EXTENSION_DISABLE) {
			wchar_t *s;
			aswprintf(&s, L"\"%s\", %d", pl->filename, pl->enable);
			HistoryStoreAddTop(hs, s, FALSE);
			free(s);
		}
	}
	HistoryStoreSaveIni(hs, SetupFNW, L"Plugin", L"list");
	HistoryStoreDestroy(hs);
}

/**
 *	1ファイル読み込む
 */
static void loadExtension(ExtensionList *pl, const wchar_t *UILanguageFile)
{
	DWORD err;
	const wchar_t *sub_message;
	HMODULE hPlugin;

	pl->exports = NULL;
	pl->LibHandle = NULL;

	// ファイルがない
	if (GetFileAttributesW(pl->path) == INVALID_FILE_ATTRIBUTES) {
		return;
	}

	hPlugin = LoadLibraryW(pl->path);
	if (hPlugin != NULL) {
		TTXBindProc bind = NULL;
		FARPROC *pbind = (FARPROC *)&bind;
#if defined(_MSC_VER)
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "_TTXBind@8");
#else
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "TTXBind@8");
#endif
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "TTXBind");
		if (bind != NULL) {
			TTXExports * exports = (TTXExports *)calloc(1, sizeof(TTXExports));
			if (exports == NULL) {
				FreeLibrary(hPlugin);
				return;
			}
			exports->size = sizeof(TTXExports);

			if (bind(TTVERSION, exports)) {
				pl->exports = exports;
				pl->LibHandle = hPlugin;
				return;
			}
			else {
				free(exports);
			}
		}
		FreeLibrary(hPlugin);
	}

	err = GetLastError();
	switch (err) {
	case ERROR_GEN_FAILURE:		// 31
		sub_message = L"Unresolved dll entry";
		break;
	case ERROR_DLL_INIT_FAILED:	// 1114
		// DllMain() が FALSE を返した
		sub_message = L"tls entry exists";
		break;
	case ERROR_FILE_NOT_FOUND:	// 2
		// 依存dllが存在しない
		sub_message = L"rejected by plugin";
		break;
	case ERROR_BAD_EXE_FORMAT:	// 193
		// dllではないファイル、アーキテクチャが異なるファイル
		sub_message = L"invalid dll image";
		break;
	case ERROR_MOD_NOT_FOUND: 	// 126
		// 依存dllが存在しない
		sub_message = L"dll not exist";
		break;
	case ERROR_PROC_NOT_FOUND:	// 127
		// プロシージャが存在しない
		sub_message = L"procedure could not be found";
		break;
	default:
		sub_message = L"unknown";
		break;
	}
	// 言語ファイルによるメッセージの国際化を行っているが、この時点では設定が
	// まだ読み込まれていない為、メッセージが英語のままとなる。要検討。
	{
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_TT_ERROR", L"Tera Term: Error",
			"MSG_LOAD_EXT_ERROR", L"Cannot load extension %s (%d, %s)",
			MB_OK | MB_ICONEXCLAMATION
		};
		TTMessageBoxW(NULL, &info, UILanguageFile, pl->path, err, sub_message);
	}
}

/**
 *	フォルダ内のプラグイン一覧を取得
 *	dir のプラグインをリストする
 */
static void ListPlugins(const wchar_t *dir)
{
	wchar_t *load_mask;
	WIN32_FIND_DATAW fd;
	HANDLE hFind;

	aswprintf(&load_mask, L"%s\\TTX*.DLL", dir);

	hFind = FindFirstFileW(load_mask, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			const wchar_t *filename = fd.cFileName;
			if (MatchTTXFileName(filename) == FALSE) {
				// - 8.3(Short File Name)変換で "TTX*.DLL" にマッチしても
				//   実際のファイル名が "TTX*.DLL" ではない場合がある
				// - cFileName は MAX_PATH(260=3+256+1)文字
				//   - NTFSのファイル名長は256文字,終端L"\0"含む (内部文字コードはUTF-16LE?)
				//   - Ext4のファイル名長は256byte終端"\0"含む (概ねUTF-8と思われる)
				continue;
			}
			wchar_t *fullpath;
			aswprintf(&fullpath, L"%s\\%s", dir, filename);
			PluginListAdd(fullpath, EXTENSION_UNSPECIFIED);
			free(fullpath);
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(load_mask);
}

#if 0
static void DeletePluginList(size_t index)
{
	if (index >= NumExtensions) {
		return;
	}

	ExtensionList *p = &Extensions[index];
	free(p->filename);
	p->filename = NULL;
	if (p->exports != NULL) {
		free(p->exports);
		p->exports = NULL;
	}
	if (index + 1 < NumExtensions) {
		memmove(&Extensions[index], &Extensions[index + 1], sizeof(ExtensionList) * (NumExtensions - index - 1));
	}
	NumExtensions--;

	// リストが空になった
	if (NumExtensions == 0) {
		free(Extensions);
		Extensions = NULL;
	}
}
#endif

/**
 * プラグイン情報を取得する
 * @param[in]	index		プラグインのインデックス
 * @param[out]	info		プラグイン情報を格納する構造体へのポインタ
 * @return					成功した場合はTRUE、失敗した場合はFALSE
 */
BOOL PluginGetInfo(int index, PluginInfo *info)
{
	*info = {};

	if (index < 0 || index >= NumExtensions) {
		return FALSE;
	}

	const ExtensionList *pl = &Extensions[index];
	info->filename = pl->filename;
	info->enable = pl->enable;
	if (pl->exports != NULL) {
		// ロードしている
		info->loaded = EXTENSION_LOADED;
		info->load_order = pl->exports->loadOrder;
	} else {
		// ロードしていない
		info->loaded = EXTENSION_UNLOADED;
	}

	return TRUE;
}

/**
 * プラグイン情報を変更する
 *
 * @param info.filename		プラグインファイル名
 *							ファイル名部分のみ
 * @param info.enable		ExtensionEnable.EXTENSION_DISABLE
 *							ExtensionEnable.EXTENSION_ENABLE
 *							ExtensionEnable.EXTENSION_UNSPECIFIED
 */
void PluginChangeInfo(const PluginInfo *info)
{
	wchar_t *filename = GetFilename(info->filename);
	ExtensionList *pl = PluginGetInfo(filename);
	free(filename);
	if (pl == NULL) {
		// 見つからない
		return;
	}

	// 変更
	pl->enable = info->enable;
}

static void LoadExtensions(PTTSet ts_)
{
	ListPlugins(ts_->ExeDirW);
	PluginListRead(ts_->SetupFNameW);

	if (NumExtensions==0) return;

	// プラグインをロード
	for (size_t i = 0; i < NumExtensions; i++) {
		ExtensionList *pl = &Extensions[i];
		if ((pl->enable == EXTENSION_ENABLE) ||
			(pl->enable == EXTENSION_UNSPECIFIED ))	// 未指定
		{
			loadExtension(pl, ts_->UILanguageFileW);
		}
	}

	// プラグインの並べ替え
	qsort(Extensions, NumExtensions, sizeof(Extensions[0]), compareOrder);
}

static void UnloadExtensions()
{
	int i;
	for (i = 0; i < NumExtensions; i++) {
		ExtensionList *p = &Extensions[i];
		free(p->filename);
		p->filename = NULL;
		free(p->path);
		p->path = NULL;
		if (p->exports != NULL) {
			free(p->exports);
			p->exports = NULL;
			FreeLibrary(p->LibHandle);
			p->LibHandle = NULL;
		}
	}

	free(Extensions);
	Extensions = NULL;
	NumExtensions = 0;
}

void PASCAL TTXInit(PTTSet ts_, PComVar cv_)
{
	int i;

	LoadExtensions(ts_);

	if (NumExtensions==0) return;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXInit != NULL) {
			Extensions[i].exports->TTXInit(ts_, cv_);
		}
	}
}

static void PASCAL TTXInternalOpenTCP(TTXSockHooks * hooks)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXOpenTCP != NULL) {
			Extensions[i].exports->TTXOpenTCP(hooks);
		}
	}
}

void PASCAL TTXOpenTCP(void)
{
	static TTXSockHooks SockHooks = {
		&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
		&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
		&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
		&PWSACancelAsyncRequest, &PWSAGetLastError,
		/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
	};
	TTXInternalOpenTCP(&SockHooks);
}

static void PASCAL TTXInternalCloseTCP(TTXSockHooks * hooks)
{
	int i;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXCloseTCP != NULL) {
			Extensions[i].exports->TTXCloseTCP(hooks);
		}
	}
}

void PASCAL TTXCloseTCP(void)
{
	static TTXSockHooks SockHooks = {
		&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
		&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
		&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
		&PWSACancelAsyncRequest, &PWSAGetLastError,
		/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
	};
	TTXInternalCloseTCP(&SockHooks);
}

static void PASCAL TTXInternalOpenFile(TTXFileHooks * hooks)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXOpenFile != NULL) {
			Extensions[i].exports->TTXOpenFile(hooks);
		}
	}
}

void PASCAL TTXOpenFile(void)
{
	static TTXFileHooks FileHooks = {
		&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
	};
	TTXInternalOpenFile(&FileHooks);
}

static void PASCAL TTXInternalCloseFile(TTXFileHooks * hooks)
{
	int i;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXCloseFile != NULL) {
			Extensions[i].exports->TTXCloseFile(hooks);
		}
	}
}

void PASCAL TTXCloseFile(void)
{
	static TTXFileHooks FileHooks = {
		&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
	};
	TTXInternalCloseFile(&FileHooks);
}

static void PASCAL TTXInternalGetUIHooks(TTXUIHooks * hooks)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXGetUIHooks != NULL) {
			Extensions[i].exports->TTXGetUIHooks(hooks);
		}
	}
}

void PASCAL TTXGetUIHooks(void)
{
	static TTXUIHooks UIHooks = {
		&SetupTerminal, &SetupWin, &SetupKeyboard, &SetupSerialPort,
		&SetupTCPIP, &GetHostName, &ChangeDirectory, &AboutDialog,
		&ChooseFontDlg, &SetupGeneral, &WindowWindow
	};
	TTXInternalGetUIHooks(&UIHooks);
}

static void PASCAL TTXInternalGetSetupHooks(TTXSetupHooks * hooks)
{
	int i;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXGetSetupHooks != NULL) {
			Extensions[i].exports->TTXGetSetupHooks(hooks);
		}
	}
}

void PASCAL TTXGetSetupHooks(void)
{
	static TTXSetupHooks SetupHooks = {
		&ReadIniFile, &WriteIniFile, &ReadKeyboardCnf, &CopyHostList,
		&AddHostToList, &ParseParam
	};
	TTXInternalGetSetupHooks(&SetupHooks);
}

void PASCAL TTXSetWinSize(int rows, int cols)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXSetWinSize != NULL) {
			Extensions[i].exports->TTXSetWinSize(rows, cols);
		}
	}
}

void PASCAL TTXModifyMenu(HMENU menu)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXModifyMenu != NULL) {
			Extensions[i].exports->TTXModifyMenu(menu);
		}
	}
}

void PASCAL TTXModifyPopupMenu(HMENU menu)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXModifyPopupMenu != NULL) {
			Extensions[i].exports->TTXModifyPopupMenu(menu);
		}
	}
}

BOOL PASCAL TTXProcessCommand(HWND hWin, WORD cmd)
{
	int i;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXProcessCommand != NULL) {
			if (Extensions[i].exports->TTXProcessCommand(hWin,cmd)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

void PASCAL TTXEnd(void)
{
	int i;

	if (NumExtensions == 0)
		return;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXEnd != NULL) {
			Extensions[i].exports->TTXEnd();
		}
	}

	UnloadExtensions();
}

void PASCAL TTXSetCommandLine(wchar_t *cmd, int cmdlen, PGetHNRec rec)
{
	int i;

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports == NULL) {
			continue;
		}
		if (Extensions[i].exports->TTXSetCommandLine != NULL) {
			Extensions[i].exports->TTXSetCommandLine(cmd, cmdlen, rec);
		}
	}
}
