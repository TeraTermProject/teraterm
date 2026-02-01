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

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "asprintf.h"
#include "history_store.h"

#include "ttplugin.h"
#include "ttplug.h"

typedef struct _ExtensionList {
	//wchar_t *path;					// フルパス
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

static void PluginListAdd(const ExtensionList *item)
{
	NumExtensions++;
	Extensions = (ExtensionList *)realloc(Extensions, sizeof(ExtensionList) * NumExtensions);
	ExtensionList *pl = &Extensions[NumExtensions - 1];
	*pl = *item;
	wchar_t *filename = wcsrchr(item->filename, L'\\');
	if (filename == NULL) {
		filename = item->filename;	// フルパスでない場合はそのまま
	} else {
		filename++; // '\\'の次の文字から
	}
	pl->filename = _wcsdup(filename);
}

static void PluginListRead(const wchar_t *SetupFNW)
{
	HistoryStore *hs = HistoryStoreCreate(20);
	HistoryStoreReadIni(hs, SetupFNW, L"Plugin", L"list");

	for (size_t i = 0;; i++) {
		const wchar_t *s = HistoryStoreGet(hs, i);
		if (s == NULL) {
			break;
		}
		size_t len_ch = wcslen(s) + 1;
		wchar_t *buf = (wchar_t *)malloc(sizeof(wchar_t) * len_ch);
		int enable_int;
		// ""file", 1"  を想定
		int r = swscanf_s(s, L"\"%[^\"]\", %d", buf, (unsigned int)len_ch, &enable_int);
		if (r <= 1) {
			free(buf);
			continue;
		}
		ExtensionEnable enable =
			enable_int == 0 ? EXTENSION_DISABLE :
			enable_int == 1 ? EXTENSION_ENABLE :
			EXTENSION_UNSPECIFIED;

		PluginInfo info;
		info.filename = buf;
		info.enable = enable;
		PluginAddInfo(&info);
		free(buf);
	}
	HistoryStoreDestroy(hs);
}

void PluginWriteList(const wchar_t *SetupFNW)
{
	HistoryStore *hs = HistoryStoreCreate(20);
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

static void loadExtension(size_t index, const wchar_t *ExeDirW, const wchar_t *UILanguageFile)
{
	DWORD err;
	const wchar_t *sub_message;
	HMODULE hPlugin;
	wchar_t *filename;
	aswprintf(&filename, L"%s\\%s", ExeDirW, Extensions[index].filename);

	Extensions[index].exports = NULL;
	Extensions[index].LibHandle = NULL;

	// ファイルがない
	if (GetFileAttributesW(filename) == INVALID_FILE_ATTRIBUTES) {
		free(filename);
		return;
	}

	hPlugin = LoadLibraryW(filename);
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
			TTXExports * exports = (TTXExports *)malloc(sizeof(TTXExports));
			if (exports == NULL) {
				return;
			}
			memset(exports, 0, sizeof(TTXExports));
			exports->size = sizeof(TTXExports);

			if (bind(TTVERSION, exports)) {
				Extensions[index].exports = exports;
				Extensions[index].LibHandle = hPlugin;
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
		case 31:
			sub_message = L"Unresolved dll entry";
			break;
		case 1114:
			sub_message = L"tls entry exists";
			break;
		case 2:
			sub_message = L"rejected by plugin";
			break;
		case 193:
			sub_message = L"invalid dll image";
			break;
		case 126:
			sub_message = L"dll not exist";
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
		TTMessageBoxW(NULL, &info, UILanguageFile, filename, err, sub_message);
	}

	free(filename);
}

static void ListPlugins(const wchar_t *ExeDirW)
{
	wchar_t *load_mask;
	WIN32_FIND_DATAW fd;
	HANDLE hFind;

	aswprintf(&load_mask, L"%s\\TTX*.DLL", ExeDirW);

	hFind = FindFirstFileW(load_mask, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			ExtensionList item = {};
			item.filename = _wcsdup(fd.cFileName);
			item.enable = EXTENSION_UNSPECIFIED;
			PluginListAdd(&item);
			free(item.filename);
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(load_mask);
}

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

/**
 * プラグイン情報を取得する
 * @param index		プラグインのインデックス
 * @param info		プラグイン情報を格納する構造体へのポインタ
 *					文字列は書き換えないこと
 * @return			成功した場合はTRUE、失敗した場合はFALSE
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
		// ロードしていない
		info->loaded = EXTENSION_LOADED;
		info->load_order = pl->exports->loadOrder;
	} else {
		// ロードしていない
		info->loaded = EXTENSION_UNLOADED;
	}

	return TRUE;
}

/**
 * プラグイン情報を設定する
 * @param index		プラグインのインデックス
 * @param info		プラグイン情報を格納する構造体へのポインタ
 *					文字列は書き換えないこと
 * @return			成功した場合はTRUE、失敗した場合はFALSE
 */
void PluginAddInfo(const PluginInfo *info)
{
	// ファイル名のみにする
	wchar_t *filename = wcsrchr(info->filename, L'\\');
	if (filename == NULL) {
		// 元からファイル名だけだった
		filename = info->filename;
	} else {
		filename++;
	}

	BOOL find = FALSE;
	int index = 0;
	for (int i = 0; i < NumExtensions; i++) {
		if (wcscmp(Extensions[i].filename, filename) == 0) {
			index = i;
			find = TRUE;
			break;
		}
	}
	if (find == FALSE) {
		// 新規追加
		ExtensionList e_item = {};
		e_item.filename = filename;
		e_item.enable = info->enable;
		PluginListAdd(&e_item);
	}
	else {
		// 変更
		ExtensionList *pl = &Extensions[index];
		free(pl->filename);
		pl->filename = _wcsdup(filename);
		pl->enable = info->enable;
	}
}

static void LoadExtensions(PTTSet ts_)
{
	ListPlugins(ts_->ExeDirW);
	PluginListRead(ts_->SetupFNameW);

	if (NumExtensions==0) return;

	// プラグインをロード
	for (size_t i = 0; i < NumExtensions; i++) {
		const ExtensionList *pl = &Extensions[i];
		if ((pl->enable == EXTENSION_ENABLE) ||
			(pl->enable == EXTENSION_UNSPECIFIED ))	// 未指定
		{
			loadExtension(i, ts_->ExeDirW, ts_->UILanguageFileW);
		}
	}

	qsort(Extensions, NumExtensions, sizeof(Extensions[0]), compareOrder);
}

static void UnloadExtensions()
{
	int i;
	for (i = 0; i < NumExtensions; i++) {
		ExtensionList *p = &Extensions[i];
		free(p->filename);
		p->filename = NULL;
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
