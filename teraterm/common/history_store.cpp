/*
 * Copyright (C) 2024- TeraTerm Project
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
#include <string.h>
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "asprintf.h"
#include "win32helper.h"
#include "dlglib.h"		// for SetComboBoxHostHistory()

#include "history_store.h"

typedef struct HistoryStoreTag {
	size_t max;
	size_t count;
	wchar_t **ptr;
} HistoryStore;

HistoryStore *HistoryStoreCreate(size_t max)
{
	HistoryStore *h = (HistoryStore *)calloc(1, sizeof(*h));
	if (h == NULL) {
		return NULL;
	}
	h->max = max;
	h->count = 0;
	h->ptr = (wchar_t **)calloc(h->max, sizeof(wchar_t *));
	if (h->ptr == NULL) {
		free(h);
		return NULL;
	}
	return h;
}

void HistoryStoreClear(HistoryStore *h)
{
	for (size_t i = 0; i < h->count; i++) {
		free(h->ptr[i]);
		h->ptr[i] = NULL;
	}
	h->count = 0;
}

void HistoryStoreDestroy(HistoryStore *h)
{
	if (h == NULL) {
		return;
	}
	HistoryStoreClear(h);
	free(h->ptr);
	free(h);
}

/**
 *	historyの最新に追加
 *
 *	@param	h
 *	@param	add_str				追加文字列
 *	@param	enable_duplicate	FALSE / TRUE = 追加文字列の重複を許可しない / する
 *	@retval	TRUE				履歴の内容が変化した
 *	@retval	FALSE				履歴の内容が変化しなかった
 */
BOOL HistoryStoreAddTop(HistoryStore *h, const wchar_t *add_str, BOOL enable_duplicate)
{
	if (add_str == NULL) {
		return FALSE;
	}
	wchar_t *s = _wcsdup(add_str);
	if (s == NULL) {
		return FALSE;
	}

	if (h->count != 0) {
		size_t from = 0;
		size_t to = h->count - 1;
		if (!enable_duplicate) {
			for (size_t i = 0; i < h->count; i++) {
				if (wcscmp(h->ptr[i], add_str) == 0) {
					if (i == 0) {
						// Top が同一、Historyは変化しない
						free(s);
						return FALSE;
					}
					from = 0;
					to = i - 1;
					free(h->ptr[i]);
					h->ptr[i] = NULL;
					break;
				}
			}
		}

		if (to + 1 == h->max) {
			// 増加して最大サイズを超える場合は、最後を削除しておく
			free(h->ptr[to]);
			h->ptr[to] = NULL;
			to--;
		}
		else if (to == h->count - 1) {
			// サイズが1つ増える
			h->count++;
		}
		else {
			// 移動だけ
		}

		// 移動
		memmove(&h->ptr[from + 1], &h->ptr[from], (to - from + 1) * sizeof(wchar_t *));
	}
	else {
		h->count++;
	}

	// 追加
	h->ptr[0] = s;

	return TRUE;
}

/**
 *	iniファイルからヒストリを読み込む
 *
 *	@param	h
 *	@param	FName				iniファイル名
 *	@param	section				section名
 *	@param	key					keyのベース
 *
 *	section="Hosts"
 *	key="host" のときの例
 *
 *	[Hosts]
 *	host1=sample1
 *	host2=sample2
 *	host3=sample3
 */
void HistoryStoreReadIni(HistoryStore *h, const wchar_t *FName, const wchar_t *section, const wchar_t *key)
{
	size_t count = 0;
	int unset_count = 0;
	for (size_t i = 0 ; i < h->max; i++) {
		wchar_t *EntName;
		aswprintf(&EntName, L"%s%d", key, i + 1);
		wchar_t *item;
		hGetPrivateProfileStringW(section, EntName, L"", FName, &item);
		free(EntName);
		if (item[0] == 0) {
			free(item);
			unset_count++;
			if (unset_count == 10) {
				break;
			}
		}
		else {
			h->ptr[count] = item;
			count++;
		}
	}
	h->count = count;
}

/**
 *	ヒストリをiniファイルへ保存する
 *
 *	@param	h
 *	@param	FName				iniファイル名
 *	@param	section				section名
 *	@param	key					keyのベース
 */
void HistoryStoreSaveIni(HistoryStore *h, const wchar_t *FName, const wchar_t *section, const wchar_t *key)
{
	if (FName == NULL || FName[0] == 0) {
		return;
	}

	// sectionを全部消す
	WritePrivateProfileStringW(section, NULL, NULL, FName);

	if (h->count == 0) {
		return;
	}

	for (size_t i = 0 ; i < h->count; i++) {
		wchar_t *EntName;

		aswprintf(&EntName, L"%s%i", key, (int)i + 1);
		WritePrivateProfileStringW(section, EntName, h->ptr[i], FName);
		free(EntName);
	}

	/* update file */
	/*  第一引数がNULLの時何が起こるかはMSのドキュメントに書かれていない */
	WritePrivateProfileStringW(NULL, NULL, NULL, FName);
}

/**
 *	ヒストリをドロップダウンにセットする
 */
void HistoryStoreSetControl(HistoryStore *h, HWND dlg, int dlg_item)
{
	char class_name[32];
	UINT ADDSTRING;
	int r = GetClassNameA(GetDlgItem(dlg, dlg_item), class_name, _countof(class_name));
	if (r == 0) {
		return;
	}
	if (strcmp(class_name, "ComboBox") == 0) {
		ADDSTRING = CB_ADDSTRING;
	}
	else if (strcmp(class_name, "ListBox") == 0) {
		ADDSTRING = LB_ADDSTRING;
	}
	else {
		assert(FALSE);
		return;
	}

	for (size_t i = 0; i < h->count; i++) {
		SendDlgItemMessageW(dlg, dlg_item, ADDSTRING, 0, (LPARAM)h->ptr[i]);
	}
}

void HistoryStoreGetControl(HistoryStore *h, HWND dlg, int dlg_item)
{
	LRESULT getcount_result;
	size_t item_count;
	size_t i;
	char class_name[32];
	UINT GETCOUNT;
	int r = GetClassNameA(GetDlgItem(dlg, dlg_item), class_name, _countof(class_name));
	if (r == 0) {
		return;
	}
	if (strcmp(class_name, "ListBox") == 0) {
		GETCOUNT = LB_GETCOUNT;
	}
	else {
		assert(FALSE);
		return;
	}

	getcount_result = SendDlgItemMessageW(dlg, dlg_item, GETCOUNT, 0, 0);
	if (getcount_result == LB_ERR) {
		return;
	}
	item_count = (size_t)getcount_result;
	if (item_count > h->max) {
		item_count = h->max;
	}

	HistoryStoreClear(h);

	for (i = 0; i < item_count; i++) {
		wchar_t *strW;
		GetDlgItemIndexTextW(dlg, dlg_item, (WPARAM)i, &strW);
		h->ptr[i] = strW;
	}
	h->count = item_count;
}

/**
 *	接続したホスト履歴をコンボボックスまたはリストボックスにセットする
 *	dlglib.h
 */
void SetComboBoxHostHistory(HWND dlg, int dlg_item, int maxhostlist, const wchar_t *SetupFNW)
{
	HistoryStore *hs = HistoryStoreCreate(maxhostlist);
	HistoryStoreReadIni(hs, SetupFNW, L"Hosts", L"host");
	HistoryStoreSetControl(hs, dlg, dlg_item);
	HistoryStoreDestroy(hs);
}

const wchar_t *HistoryStoreGet(HistoryStore *h, size_t index)
{
	if (index >= h->count) {
		return NULL;
	}
	return h->ptr[index];
}

void HistoryStoreSet(HistoryStore *h, size_t index, const wchar_t *str)
{
	if (index >= h->count) {
		return;
	}
	free(h->ptr[index]);
	h->ptr[index] = _wcsdup(str);
}
