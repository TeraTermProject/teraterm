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

/* C99風に記述できるようにcppとした */
/* Visual Studio 2005 が C89 */

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
#include <malloc.h>
#include <time.h>

#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "codeconv.h"
#include "compat_win.h"
#include "fileread.h"

#include "ttlib.h"

// for isInvalidFileNameCharW / replaceInvalidFileNameCharW
static const wchar_t *invalidFileNameCharsW = L"\\/:*?\"<>|";

/**
 *	MessageBoxを表示する
 *
 *	@param[in]	hWnd			親 window
 *	@param[in]	info			タイトル、メッセージ
 *	@param[in]	UILanguageFile	lngファイル
 *	@param[in]	...				フォーマット引数
 *
 *	info.message を書式化文字列として、
 *	UILanguageFileより後ろの引数を出力する
 *
 *	info.message_key, info.message_default 両方ともNULLの場合
 *		可変引数の1つ目を書式化文字列として使用する
 */
int TTMessageBoxW(HWND hWnd, const TTMessageBoxInfoW *info, const wchar_t *UILanguageFile, ...)
{
	const char *section = info->section;
	const UINT uType = info->uType;
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		GetI18nStrWW(section, info->title_key, info->title_default, UILanguageFile, &title);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format;
		GetI18nStrWW(section, info->message_key, info->message_default, UILanguageFile, &format);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);

	return r;
}

/**
 *	MessageBoxを表示する
 *
 *	@param[in]	hWnd			親 window
 *	@param[in]	info			タイトル、メッセージ
 *	@param[in]	UILanguageFile	lngファイル
 *	@param[in]	...				フォーマット引数
 *
 *	info.message を書式化文字列として、
 *	UILanguageFileより後ろの引数を出力する
 *
 *	info.message_key, info.message_default 両方ともNULLの場合
 *		可変引数の1つ目を書式化文字列として使用する
 */
int TTMessageBoxA(HWND hWnd, const TTMessageBoxInfoW *info, const char *UILanguageFile, ...)
{
	const char *section = info->section;
	const UINT uType = info->uType;
	wchar_t *UILanguageFileW = ToWcharA(UILanguageFile);
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		title = TTGetLangStrW(section, info->title_key, info->title_default, UILanguageFileW);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format = TTGetLangStrW(section, info->message_key, info->message_default, UILanguageFileW);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);
	free(UILanguageFileW);

	return r;
}

/**
 *	strがテキストかどうかチェック
 *
 *	@param[in]	str		テストする文字
 *	@param[in]	len		テストするテキスト長(L'\0'は含まない)
 *						0のときL'\0'の前までテストする
 *	@retval		TRUE	テキストと思われる(長さ0のときも含む)
 *				FALSE	テキストではない
 *
 *	検査するデータ内に0x20未満のテキストには出てこない文字があれば
 *	バイナリと判断
 *	IsTextUnicode() のほうが良いか？
 */
BOOL IsTextW(const wchar_t *str, size_t len)
{
	if (len == 0) {
		len = wcslen(str);
		if (len == 0) {
			return TRUE;
		}
	}

	BOOL result = TRUE;
	while(len-- > 0) {
		wchar_t c = *str++;
		if (c >= 0x20) {
			continue;
		}
		if ((7 <= c && c <= 0x0d) || c == 0x1b) {
			/* \a, \b, \t, \n, \v, \f, \r, \e */
			continue;
		}
		result = FALSE;
		break;
	}
	return result;
}

/**
 *	クリップボードからwchar_t文字列を取得する
 *	文字列長が必要なときはwcslen()すること
 *	@param	hWnd
 *	@param	emtpy		TRUEのときクリップボードを空にする
 *	@retval	NULL		エラー
 *	@retval	NULL以外	文字列へのポインタ 使用後free()すること
 */
wchar_t *GetClipboardTextW(HWND hWnd, BOOL empty)
{
	static const UINT list[] = {
		CF_UNICODETEXT,
		CF_TEXT,
		CF_OEMTEXT,
		CF_HDROP,
	};
	UINT Cf = GetPriorityClipboardFormat((UINT *)list, _countof(list));
	if (Cf == 0) {
		// クリップボードが空だった
		// 空文字を返す
		return _wcsdup(L"");
	}
	if (Cf == -1) {
		// 扱えるデータがなかった
		return NULL;
	}

 	if (!OpenClipboard(hWnd)) {
		return NULL;
	}
	HGLOBAL TmpHandle = GetClipboardData(Cf);
	if (TmpHandle == NULL) {
		return NULL;
	}
	wchar_t *str_w = NULL;
	size_t str_w_len;
	if (Cf == CF_HDROP) {
		HDROP hDrop = (HDROP)TmpHandle;
		UINT count = DragQueryFileW(hDrop, (UINT)-1, NULL, 0);

		// text length
		size_t length = 0;
		for (UINT i = 0; i < count; i++) {
			const UINT l = DragQueryFileW(hDrop, i, NULL, 0);
			if (l == 0) {
				continue;
			}
			length += (l + 1);
		}

		// filename
		str_w = (wchar_t *)malloc(length * sizeof(wchar_t));
		wchar_t *p = str_w;
		for (UINT i = 0; i < count; i++) {
			const UINT l = DragQueryFileW(hDrop, i, p, (UINT)(length - (p - str_w)));
			if (l == 0) {
				continue;
			}
			p += l;
			*p++ = (i + 1 == count) ? L'\0' : '\n';
		}
	}
	else if (Cf == CF_UNICODETEXT) {
		const wchar_t *str_cb = (wchar_t *)GlobalLock(TmpHandle);
		if (str_cb != NULL) {
			size_t str_cb_len = GlobalSize(TmpHandle);	// bytes
			str_w_len = str_cb_len / sizeof(wchar_t);
			str_w = (wchar_t *)malloc((str_w_len + 1) * sizeof(wchar_t));	// +1 for terminator
			if (str_w != NULL) {
				memcpy(str_w, str_cb, str_cb_len);
				str_w[str_w_len] = L'\0';
			}
		}
	}
	else {
		const char *str_cb = (char *)GlobalLock(TmpHandle);
		if (str_cb != NULL) {
			size_t str_cb_len = GlobalSize(TmpHandle);
			str_w_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str_cb, (int)str_cb_len, NULL, 0);
			str_w = (wchar_t *)malloc(sizeof(wchar_t) * (str_w_len + 1));	// +1 for terminator
			if (str_w != NULL) {
				str_w_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str_cb, (int)str_cb_len, str_w, (int)str_w_len);
				str_w[str_w_len] = L'\0';
			}
		}
	}
	GlobalUnlock(TmpHandle);
	if (empty) {
		EmptyClipboard();
	}
	CloseClipboard();
	return str_w;
}

/**
 *	クリップボードからANSI文字列を取得する
 *	文字列長が必要なときはstrlen()すること
 *	@param	hWnd
 *	@param	emtpy	TRUEのときクリップボードを空にする
 *	@retval	文字列へのポインタ 使用後free()すること
 *			文字がない(またはエラー時)はNULL
 */
char *GetClipboardTextA(HWND hWnd, BOOL empty)
{
	HGLOBAL hGlobal;
	const char *lpStr;
	size_t length;
	char *pool;

    OpenClipboard(hWnd);
    hGlobal = (HGLOBAL)GetClipboardData(CF_TEXT);
    if (hGlobal == NULL) {
        CloseClipboard();
		return NULL;
    }
    lpStr = (const char *)GlobalLock(hGlobal);
	length = GlobalSize(hGlobal);
	if (length == 0) {
		pool = NULL;
	} else {
		pool = (char *)malloc(length + 1);	// +1 for terminator
		memcpy(pool, lpStr, length);
		pool[length] = '\0';
	}
	GlobalUnlock(hGlobal);
	if (empty) {
		EmptyClipboard();
	}
	CloseClipboard();

	return pool;
}

/**
 *	クリップボードにテキストをセットする
 *	str_w	クリップボードにセットする文字列へのポインタ
 *			NULLのときクリップボードを空にする(str_lenは参照されない)
 *	str_len	文字列長
 *			0のとき文字列長が自動で算出される
 */
BOOL CBSetTextW(HWND hWnd, const wchar_t *str_w, size_t str_len)
{
	HGLOBAL CBCopyWideHandle;

	if (str_w == NULL) {
		str_len = 0;
	} else {
		if (str_len == 0)
			str_len = wcslen(str_w);
	}

	if (!OpenClipboard(hWnd)) {
		return FALSE;
	}

	EmptyClipboard();
	if (str_len == 0) {
		CloseClipboard();
		return TRUE;
	}

	{
		// 文字列をコピー、最後のL'\0'も含める
		wchar_t *CBCopyWidePtr;
		const size_t alloc_bytes = (str_len + 1) * sizeof(wchar_t);
		CBCopyWideHandle = GlobalAlloc(GMEM_MOVEABLE, alloc_bytes);
		if (CBCopyWideHandle == NULL) {
			CloseClipboard();
			return FALSE;
		}
		CBCopyWidePtr = (wchar_t *)GlobalLock(CBCopyWideHandle);
		if (CBCopyWidePtr == NULL) {
			CloseClipboard();
			return FALSE;
		}
		memcpy(CBCopyWidePtr, str_w, alloc_bytes - sizeof(wchar_t));
		CBCopyWidePtr[str_len] = L'\0';
		GlobalUnlock(CBCopyWideHandle);
	}

	SetClipboardData(CF_UNICODETEXT, CBCopyWideHandle);

	// TODO 9x系では自動でCF_TEXTにセットされないらしい?
	// ttl_gui.c の TTLVar2Clipb() ではつぎの2つが行われていた
	//		SetClipboardData(CF_TEXT, hText);
	//		SetClipboardData(CF_UNICODETEXT, wide_hText);
	CloseClipboard();

	return TRUE;
}

// from ttxssh
static void format_line_hexdump(char *buf, int buflen, int addr, int *bytes, int byte_cnt)
{
	int i, c;
	char tmp[128];

	buf[0] = 0;

	/* 先頭のアドレス表示 */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%08X : ", addr);
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* バイナリ表示（4バイトごとに空白を挿入）*/
	for (i = 0; i < byte_cnt; i++) {
		if (i > 0 && i % 4 == 0) {
			strncat_s(buf, buflen, " ", _TRUNCATE);
		}

		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02X", bytes[i]);
		strncat_s(buf, buflen, tmp, _TRUNCATE);
	}

	/* ASCII表示部分までの空白を補う */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "   %*s%*s", (16 - byte_cnt) * 2 + 1, " ", (16 - byte_cnt + 3) / 4, " ");
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* ASCII表示 */
	for (i = 0; i < byte_cnt; i++) {
		c = bytes[i];
		if (isprint(c)) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", c);
			strncat_s(buf, buflen, tmp, _TRUNCATE);
		}
		else {
			strncat_s(buf, buflen, ".", _TRUNCATE);
		}
	}
}

void DebugHexDump(void (*f)(const char *s), const void *data_, size_t len)
{
	const char *data = (char *)data_;
	char buff[4096];
	int c, addr;
	int bytes[16], *ptr;
	int byte_cnt;

	addr = 0;
	byte_cnt = 0;
	ptr = bytes;
	for (size_t i = 0; i < len; i++) {
		c = data[i];
		*ptr++ = c & 0xff;
		byte_cnt++;

		if (byte_cnt == 16) {
			format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
			f(buff);

			addr += 16;
			byte_cnt = 0;
			ptr = bytes;
		}
	}

	if (byte_cnt > 0) {
		format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
		f(buff);
	}
}

static void OutputDebugHexDumpSub(const char *s)
{
	char *s_cr;
	asprintf(&s_cr, "%s\n", s);
	OutputDebugStringA(s_cr);
	free(s_cr);
}

void OutputDebugHexDump(const void *data, size_t len)
{
	DebugHexDump(OutputDebugHexDumpSub, data, len);
}

/**
 *	メニューを追加する
 *	InsertMenuA() とほぼ同じ動作
 *	before引数を FALSE にすると次の項目に追加できる
 *
 *	@param[in]	hMenu			メニューハンドル (InsertMenuA() の第1引数)
 *	@param[in]	targetItemID	このIDのメニューの前又は後ろにメニューを追加 (InsertMenuA() の第2引数)
 *	@param[in]	flags			メニューflag (InsertMenuA() の第3引数)
 *	@param[in]	newItemID		メニューID (InsertMenuA() の第4引数)
 *	@param[in]	text			メニュー文字列 (InsertMenuA() の第5引数)
 *	@param[in]	before			TRUE/FALSE 前に追加/後ろに追加 (TRUEのとき InsertMenuA() と同じ動作)
 */
void TTInsertMenuItemA(HMENU hMenu, UINT targetItemID, UINT flags, UINT newItemID, const char *text, BOOL before)
{
	assert((flags & MF_BYPOSITION) == 0);
	for (int i = GetMenuItemCount(hMenu) - 1; i >= 0; i--) {
		HMENU submenu = GetSubMenu(hMenu, i);

		for (int j = GetMenuItemCount(submenu) - 1; j >= 0; j--) {
			if (GetMenuItemID(submenu, j) == targetItemID) {
				const UINT position = (before == FALSE) ? j + 1 : j;
				InsertMenuA(submenu, position, MF_BYPOSITION | flags, newItemID, text);
				return;
			}
		}
	}
}

/*
 *	文字列の改行コードをCR(0x0d)だけにする
 *
 *	@param [in]	*src	入力文字列へのポインタ
 *	@param [in] *len	入力文字列長
 *						NULL または 0 のとき内部で文字列長を測るこの時L'\0'は必須
 *	@param [out] *len	出力文字列長(wcslen()と同じ)
 *						NULL のとき出力されない
 *	@return				変換後文字列(malloc()された領域,free()すること)
 *						NULL メモリが確保できなかった
 *
 *		入力文字列長の指定がある時
 *			入力文字列の途中で L'\0' が見つかったら、そこで変換を終了する
 *			見つからないときは入力文字数分変換(最後にL'\0'は付加されない)
 */
wchar_t *NormalizeLineBreakCR(const wchar_t *src, size_t *len)
{
#define CR   0x0D
#define LF   0x0A
	size_t src_len = 0;
	if (len != NULL) {
		src_len = *len;
	}
	if (src_len == 0) {
		src_len = wcslen(src) + 1;
	}
	wchar_t *dest_top = (wchar_t *)malloc(sizeof(wchar_t) * src_len);
	if (dest_top == NULL) {
		*len = 0;
		return NULL;
	}

	const wchar_t *p = src;
	const wchar_t *p_end = src + src_len;
	wchar_t *dest = dest_top;
	BOOL find_eos = FALSE;
	while (p < p_end) {
		wchar_t c = *p++;
		if (c == CR) {
			if (*p == LF) {
				// CR+LF -> CR
				p++;
				*dest++ = CR;
			} else {
				// CR -> CR
				*dest++ = CR;
			}
		}
		else if (c == LF) {
			// LF -> CR
			*dest++ = CR;
		}
		else if (c == 0) {
			// EOSを見つけたときは打ち切る
			*dest++ = 0;
			find_eos = TRUE;
			break;
		}
		else {
			*dest++ = c;
		}
	}

	if (len != NULL) {
		*len = dest - dest_top;
		if (find_eos) {
			*len = *len - 1;
		}
	}
	return dest_top;
}

/**
 *	改行コードを CR+LF に変換する
 *	@return 変換された文字列
 */
wchar_t *NormalizeLineBreakCRLF(const wchar_t *src_)
{
	const wchar_t *src = src_;
	wchar_t *dest_top;
	wchar_t *dest;
	size_t len, need_len, alloc_len;

	// 貼り付けデータの長さ(len)、および正規化後のデータの長さ(need_len)のカウント
	for (len=0, need_len=0, src=src_; *src != '\0'; src++, len++, need_len++) {
		if (*src == CR) {
			need_len++;
			if (*(src+1) == LF) {
				len++;
				src++;
			}
		}
		else if (*src == LF) {
			need_len++;
		}
	}

	// 正規化後もデータ長が変わらない => 正規化は必要なし
	if (need_len == len) {
		dest = _wcsdup(src_);
		return dest;
	}
	alloc_len = need_len + 1;

	dest_top = (wchar_t *)malloc(sizeof(wchar_t) * alloc_len);

	src = src_ + len - 1;
	dest = dest_top + need_len;
	*dest-- = '\0';

	while (len > 0 && dest_top <= dest) {
		if (*src == LF) {
			*dest-- = *src--;
			if (--len == 0) {
				*dest = CR;
				break;
			}
			if (*src != CR) {
				*dest-- = CR;
				continue;
			}
		}
		else if (*src == CR) {
			*dest-- = LF;
			if (src == dest)
				break;
		}
		*dest-- = *src--;
		len--;
	}

	return dest_top;
}

unsigned long long GetFSize64H(HANDLE hFile)
{
	DWORD file_size_hi;
	DWORD file_size_low;
	file_size_low = GetFileSize(hFile, &file_size_hi);
	if (file_size_low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		// error
		return 0;
	}
	unsigned long long file_size = ((unsigned long long)file_size_hi << 32) + file_size_low;
	return file_size;
}

unsigned long long GetFSize64W(const wchar_t *FName)
{
	HANDLE hFile = CreateFileW(FName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	unsigned long long file_size = GetFSize64H(hFile);
	CloseHandle(hFile);
	return file_size;
}

unsigned long long GetFSize64A(const char *FName)
{
	HANDLE hFile = CreateFileA(FName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	unsigned long long file_size = GetFSize64H(hFile);
	CloseHandle(hFile);
	return file_size;
}

// Append a slash to the end of a path name
void AppendSlashW(wchar_t *Path, size_t destlen)
{
	size_t len = wcslen(Path);
	if (len > 0) {
		if (Path[len - 1] != L'\\') {
			wcsncat_s(Path,destlen,L"\\",_TRUNCATE);
		}
	}
}

/**
 *	ファイル名(パス名)を解析する
 *	GetFileNamePos() の UTF-8版
 *
 *	@param[in]	PathName	ファイル名、フルパス
 *	@param[out]	DirLen		末尾のスラッシュを含むディレクトリパス長
 *							NULLのとき値を返さない
 *	@param[out]	FNPos		ファイル名へのindex
 *							&PathName[FNPos] がファイル名
 *							NULLのとき値を返さない
 *	@retval		FALSE		PathNameが不正
 */
BOOL GetFileNamePosU8(const char *PathName, int *DirLen, int *FNPos)
{
	BYTE b;
	const char *Ptr;
	const char *DirPtr;
	const char *FNPtr;
	const char *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((strlen(PathName)>=2) && (PathName[1]==':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr++;

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		b = Ptr[0];
		PtrOld = Ptr;
		Ptr++;
		switch (b) {
			case ':':
				return FALSE;
			case '/':	/* FALLTHROUGH */
			case '\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

/**
 *	ファイル名(パス名)を解析する
 *	GetFileNamePos() の wchar_t版
 *
 *	@param[in]	PathName	ファイル名、フルパス
 *	@param[out]	DirLen		末尾のスラッシュを含むディレクトリパス長
 *							NULLのとき値を返さない
 *	@param[out]	FNPos		ファイル名へのindex
 *							&PathName[FNPos] がファイル名
 *							NULLのとき値を返さない
 *	@retval		FALSE		PathNameが不正
 */
BOOL GetFileNamePosW(const wchar_t *PathName, size_t *DirLen, size_t *FNPos)
{
	const wchar_t *Ptr;
	const wchar_t *DirPtr;
	const wchar_t *FNPtr;
	const wchar_t *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((wcslen(PathName)>=2) && (PathName[1]==L':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr++;

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		wchar_t b = Ptr[0];
		PtrOld = Ptr;
		Ptr++;
		switch (b) {
			case L':':
				return FALSE;
			case L'/':	/* FALLTHROUGH */
			case L'\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

/**
 *	ConvHexCharW() の wchar_t 版
 */
BYTE ConvHexCharW(wchar_t b)
{
	if ((b>='0') && (b<='9')) {
		return (b - 0x30);
	}
	else if ((b>='A') && (b<='F')) {
		return (b - 0x37);
	}
	else if ((b>='a') && (b<='f')) {
		return (b - 0x57);
	}
	else {
		return 0;
	}
}

/**
 *	Hex2Str() の wchar_t 版
 */
int Hex2StrW(const wchar_t *Hex, wchar_t *Str, size_t MaxLen)
{
	wchar_t b, c;
	size_t i, imax, j;

	j = 0;
	imax = wcslen(Hex);
	i = 0;
	while ((i < imax) && (j<MaxLen)) {
		b = Hex[i];
		if (b=='$') {
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = ConvHexCharW(c) << 4;
			i++;
			if (i < imax) {
				c = (BYTE)Hex[i];
			}
			else {
				c = 0x30;
			}
			b = b + ConvHexCharW(c);
		};

		Str[j] = b;
		j++;
		i++;
	}
	if (j<MaxLen) {
		Str[j] = 0;
	}

	return (int)j;
}

/**
 *	ExtractFileName() の wchar_t 版
 *	フルパスからファイル名部分を取り出す
 *
 *	@return	ファイル名部分(不要になったらfree()する)
 */
wchar_t *ExtractFileNameW(const wchar_t *PathName)
{
	size_t i;
	if (!GetFileNamePosW(PathName, NULL, &i))
		return NULL;
	wchar_t *filename = _wcsdup(&PathName[i]);
	return filename;
}

/**
 *	ExtractDirName() の wchar_t 版
 *
 *	@return	ディレクトリ名部分(不要になったらfree()する)
 */
wchar_t *ExtractDirNameW(const wchar_t *PathName)
{
	size_t i;
	wchar_t *DirName = _wcsdup(PathName);
	if (!GetFileNamePosW(DirName, &i, NULL))
		return NULL;
	DirName[i] = 0;
	return DirName;
}

/*
 *	UILanguageFileのフルパスを取得する
 *
 *	@param[in]		HomeDir					exe,dllの存在するフォルダ GetExeDir()で取得できる
 *	@param[in]		UILanguageFileRel		lngファイル、HomeDirからの相対パス
 *	@return			LanguageFile			lngファイルのフルパス
 */
wchar_t *GetUILanguageFileFullW(const wchar_t *HomeDir, const wchar_t *UILanguageFileRel)
{
	wchar_t *fullpath;
	size_t size = wcslen(HomeDir) + 1 + wcslen(UILanguageFileRel) + 1;
	wchar_t *rel = (wchar_t *)malloc(sizeof(wchar_t) * size);
	wcscpy_s(rel, size, HomeDir);
	wcscat_s(rel, size, L"\\");
	wcscat_s(rel, size, UILanguageFileRel);
	hGetFullPathNameW(rel, &fullpath, NULL);
	free(rel);
	return fullpath;
}

/**
 *	設定ファイルのフルパスを取得する
 *
 *	@param[in]	home	ttermpro.exe 等の実行ファイルのあるフォルダ
 *	@param[in]	file	設定ファイル名(パスは含まない)
 *	@return		フルパス (不要になったら free() すること)
 */
wchar_t *GetDefaultFNameW(const wchar_t *home, const wchar_t *file)
{
	size_t destlen = (wcslen(home) + wcslen(file) + 1 + 1) * sizeof(wchar_t);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * destlen);
	wcscpy_s(dest, destlen, home);
	AppendSlashW(dest,destlen);
	wcsncat_s(dest, destlen, file, _TRUNCATE);
	return dest;
}

// デフォルトの TERATERM.INI のフルパスを取得
wchar_t *GetDefaultSetupFNameW(const wchar_t *home)
{
	const wchar_t *ini = L"TERATERM.INI";
	wchar_t *buf = GetDefaultFNameW(home, ini);
	return buf;
}

/**
 *	エスケープ文字を処理する
 *	\\,\n,\t,\0 を置き換える
 *	@return		文字数（L'\0'を含む)
 */
size_t RestoreNewLineW(wchar_t *Text)
{
	size_t i;
	int j=0;
	size_t size= wcslen(Text);
	wchar_t *buf = (wchar_t *)_alloca((size+1) * sizeof(wchar_t));

	memset(buf, 0, (size+1) * sizeof(wchar_t));
	for (i=0; i<size; i++) {
		if (Text[i] == L'\\' && i<size ) {
			switch (Text[i+1]) {
				case L'\\':
					buf[j] = L'\\';
					i++;
					break;
				case L'n':
					buf[j] = L'\n';
					i++;
					break;
				case L't':
					buf[j] = L'\t';
					i++;
					break;
				case L'0':
					buf[j] = L'\0';
					i++;
					break;
				default:
					buf[j] = L'\\';
			}
			j++;
		}
		else {
			buf[j] = Text[i];
			j++;
		}
	}
	/* use memcpy to copy with '\0' */
	j++;	// 文字列長
	memcpy(Text, buf, j * sizeof(wchar_t));
	return j;
}

BOOL GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest)
{
	int i, j, k;

	i = 1;
	j = 0;
	k = 0;

	while (i<Nth && Source[j] != 0) {
		if (Source[j++] == ',') {
			i++;
		}
	}

	if (i == Nth) {
		while (Source[j] != 0 && Source[j] != ',' && k<Size-1) {
			Dest[k++] = Source[j++];
		}
	}

	Dest[k] = 0;
	return (i>=Nth);
}

void GetNthNum(PCHAR Source, int Nth, int *Num)
{
	char T[15];

	GetNthString(Source,Nth,sizeof(T),T);
	if (sscanf_s(T, "%d", Num) != 1) {
		*Num = 0;
	}
}

int GetNthNum2(PCHAR Source, int Nth, int defval)
{
	char T[15];
	int v;

	GetNthString(Source, Nth, sizeof(T), T);
	if (sscanf_s(T, "%d", &v) != 1) {
		v = defval;
	}

	return v;
}

BOOL GetNthStringW(const wchar_t *Source, int Nth, size_t Size, wchar_t *Dest)
{
	int i, j, k;

	i = 1;
	j = 0;
	k = 0;

	while (i<Nth && Source[j] != 0) {
		if (Source[j++] == ',') {
			i++;
		}
	}

	if (i == Nth) {
		while (Source[j] != 0 && Source[j] != ',' && k<Size-1) {
			Dest[k++] = Source[j++];
		}
	}

	Dest[k] = 0;
	return (i>=Nth);
}

void GetNthNumW(const wchar_t *Source, int Nth, int *Num)
{
	wchar_t T[15];

	GetNthStringW(Source,Nth,_countof(T),T);
	if (swscanf_s(T, L"%d", Num) != 1) {
		*Num = 0;
	}
}

int GetNthNum2W(const wchar_t *Source, int Nth, int defval)
{
	wchar_t T[15];
	int v;

	GetNthStringW(Source, Nth, _countof(T), T);
	if (swscanf_s(T, L"%d", &v) != 1) {
		v = defval;
	}

	return v;
}

wchar_t *GetDownloadFolderW(void)
{
	wchar_t *download;
	_SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, NULL, &download);
	return download;
}

/* fit a filename to the windows-filename format */
/* FileName must contain filename part only. */
void FitFileNameW(wchar_t *FileName, size_t destlen, const wchar_t *DefExt)
{
	int i, j, NumOfDots;
	wchar_t Temp[MAX_PATH];
	wchar_t b;

	NumOfDots = 0;
	i = 0;
	j = 0;
	/* filename started with a dot is illeagal */
	if (FileName[0]==L'.') {
		Temp[0] = L'_';  /* insert an underscore char */
		j++;
	}

	do {
		b = FileName[i];
		i++;
		if (b==L'.')
			NumOfDots++;
		if ((b!=0) &&
		    (j < MAX_PATH-1)) {
			Temp[j] = b;
			j++;
		}
	} while (b!=0);
	Temp[j] = 0;

	if ((NumOfDots==0) &&
	    (DefExt!=NULL)) {
		/* add the default extension */
		wcsncat_s(Temp,_countof(Temp),DefExt,_TRUNCATE);
	}

	wcsncpy_s(FileName,destlen,Temp,_TRUNCATE);
}

#if 0
void ConvFNameW(const wchar_t *HomeDir, wchar_t *Temp, size_t templen, const wchar_t *DefExt, wchar_t *FName, size_t destlen)
{
	// destlen = sizeof FName
	size_t DirLen, FNPos;

	FName[0] = 0;
	if ( ! GetFileNamePosW(Temp,&DirLen,&FNPos) ) {
		return;
	}
	FitFileNameW(&Temp[FNPos],templen - FNPos,DefExt);
	if ( DirLen==0 ) {
		wcsncpy_s(FName,destlen,HomeDir,_TRUNCATE);
		AppendSlashW(FName,destlen);
	}
	wcsncat_s(FName,destlen,Temp,_TRUNCATE);
}
#endif

/**
 *	pathが相対パスかどうかを返す
 *		TODO "\path\path" は 相対パスではないのではないか?
 */
BOOL IsRelativePathW(const wchar_t *path)
{
	if (path[0] == '\\' || path[0] == '/' || (path[0] != '\0' && path[1] == ':')) {
		return FALSE;
	}
	return TRUE;
}

BOOL IsRelativePathA(const char *path)
{
	if (path[0] == '\\' || path[0] == '/' || (path[0] != '\0' && path[1] == ':')) {
		return FALSE;
	}
	return TRUE;
}

// OSが 指定されたバージョンと等しい かどうかを判別する。
BOOL IsWindowsVer(DWORD dwPlatformId, DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEXA osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = dwPlatformId;
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = _VerifyVersionInfoA(&osvi, VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OSが 指定されたバージョン以降 かどうかを判別する。
//   dwPlatformId を見ていないので NT カーネル内でしか比較できない
//   5.0 以上で比較すること
BOOL IsWindowsVerOrLater(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEXA osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = _VerifyVersionInfoA(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OSが Windows XP 以降 かどうかを判別する。
//
// return TRUE:  XP or later
//        FALSE: 2000 or earlier
BOOL IsWindowsXPOrLater(void)
{
	return IsWindowsVerOrLater(5, 1);
}

/**
 *	DeleteComment の wchar_t 版
 */
wchar_t *DeleteCommentW(const wchar_t *src)
{
	size_t dest_size = wcslen(src);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * (dest_size + 1));
	wchar_t *dest_top = dest;
	BOOL quoted = FALSE;
	wchar_t *dest_end = dest + dest_size;

	while (*src != '\0' && dest < dest_end && (quoted || *src != ';')) {
		*dest++ = *src;

		if (*src++ == '"') {
			if (*src == '"' && dest < dest_end) {
				*dest++ = *src++;
			}
			else {
				quoted = !quoted;
			}
		}
	}

	*dest = '\0';

	return dest_top;
}

DWORD TTWinExecA(const char *commandA)
{
	wchar_t *commandW = ToWcharA(commandA);
	DWORD e = TTWinExec(commandW);
	free(commandW);
	return e;
}

/**
 *	バックアップファイルを作成する
 *
 *	@param[in]	fname		ファイル名(フルパス)
 *	@param[in]	prev_str	ファイル名の前に追加する文字列
 *							NULLの場合は、現在日時文字列となる
 *	例
 *		frame
 *			"C:\Users\user\AppData\Roaming\teraterm5\TERATERM.INI"
 *		prev_str
 *			NULL
 *		作成されるバックアップファイル
 *			"C:\Users\user\AppData\Roaming\teraterm5\211204_231234_TERATERM.INI"
 */
void CreateBakupFile(const wchar_t *fname, const wchar_t *prev_str)
{
	DWORD attr = GetFileAttributesW(fname);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		// ファイルがない
		return;
	}

	wchar_t *date_str = NULL;
	const wchar_t *prev_str_ptr = prev_str;
	if (prev_str == NULL) {
		date_str = MakeISO8601Str(0);
		awcscat(&date_str, L"_");
		prev_str_ptr = date_str;
	}

	wchar_t *dup = _wcsdup(fname);
	wchar_t *p = wcsrchr(dup, L'\\') + 1;
	wchar_t *name = _wcsdup(p);
	*p = 0;
	const wchar_t *path = dup;

	wchar_t *bak_fname = NULL;
	awcscats(&bak_fname, path, prev_str_ptr, name, NULL);
	free(dup);
	free(name);

	MoveFileW(fname, bak_fname);
	CopyFileW(bak_fname, fname, TRUE);
	free(bak_fname);
	if (date_str != NULL) {
		free(date_str);
	}
}

// common_static.lib 単体でビルドできるよう追加
// TODO すべてcommon_static移動する?
static BOOL _IsWindows2000OrLater(void)
{
	return IsWindowsVerOrLater(5, 0);
}

/**
 *	iniファイルを現在の環境に合わせて変換する
 *
 *	@param	fname	フルパスファイル名
 *	@param	bak_str	変換時に元ファイルをバックアップするときに追加する文字列
 *	@retval	TRUE	変換した(バックアップを作成した)
 *	@retval	FALSE	変換しなかった(ファイルが存在しない)
 *
 *	iniファイルの文字コード
 *		NT系ではANSIまたはUnicode,UTF16BE BOM)を使用できる
 *		9x系ではANSIのみ
 */
BOOL ConvertIniFileCharCode(const wchar_t *fname,  const wchar_t *bak_str)
{
	FILE *fp;
	_wfopen_s(&fp, fname, L"rb");
	if (fp == NULL) {
		// ファイルが存在しない
		return FALSE;
	}

	// ファイルを読み込む
	LoadFileCode code;
	size_t len;
	char *ini_u8 = LoadFileU8C(fp, &len, &code);
	fclose(fp);

	BOOL converted = FALSE;
	if(_IsWindows2000OrLater()) {
		// UTF16 LE
		if (code != FILE_CODE_UTF16LE) {
			CreateBakupFile(fname, bak_str);

			wchar_t *ini_u16 = ToWcharU8(ini_u8);
			_wfopen_s(&fp, fname, L"wb");
			if (fp != NULL) {
				fwrite("\xff\xfe", 2, 1, fp);	// BOM
				fwrite(ini_u16, wcslen(ini_u16), sizeof(wchar_t), fp);
				fclose(fp);
				converted = TRUE;
			}
			free(ini_u16);
		}
	}
	else {
		// ACP
		if (code != FILE_CODE_ACP) {
			CreateBakupFile(fname, bak_str);

			char *ini_acp = ToCharU8(ini_u8);
			_wfopen_s(&fp, fname, L"wb");
			if (fp != NULL) {
				fwrite(ini_acp, strlen(ini_acp), sizeof(char), fp);
				fclose(fp);
				converted = TRUE;
			}
			free(ini_acp);
		}
	}
	free(ini_u8);

	return converted;
}

/**
 *	ISO8601基本形式の日時文字列を作成する
 *
 *	@param	t	文字列を作成する時間
 *				0で現在時刻
 *	@return		文字列、不要になったらfree()すること
 */
wchar_t *MakeISO8601Str(time_t t)
{
	if (t == 0) {
		t = time(NULL);
	}
	struct tm now_tm;
	localtime_s(&now_tm, &t);
	wchar_t date_str[128];
	wcsftime(date_str, _countof(date_str), L"%Y%m%dT%H%M%S%z", &now_tm);
	return _wcsdup(date_str);
}

// base
// https://www.daniweb.com/programming/software-development/threads/481786/saving-an-hdc-as-a-bmp-file
void HDCToFile(const wchar_t* fname, HDC hdc, RECT Area, WORD BitsPerPixel = 24)
{
    LONG Width = Area.right - Area.left;
    LONG Height = Area.bottom - Area.top;

    BITMAPINFO Info;
    BITMAPFILEHEADER Header;
    memset(&Info, 0, sizeof(Info));
    memset(&Header, 0, sizeof(Header));
    Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Info.bmiHeader.biWidth = Width;
    Info.bmiHeader.biHeight = Height;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = BitsPerPixel;
    Info.bmiHeader.biCompression = BI_RGB;
    Info.bmiHeader.biSizeImage = Width * Height * (BitsPerPixel > 24 ? 4 : 3);
    Header.bfType = 0x4D42;
    Header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    char* Pixels = NULL;
    HDC MemDC = CreateCompatibleDC(hdc);
    HBITMAP Section = CreateDIBSection(hdc, &Info, DIB_RGB_COLORS, (void**)&Pixels, 0, 0);
    DeleteObject(SelectObject(MemDC, Section));
    BitBlt(MemDC, 0, 0, Width, Height, hdc, Area.left, Area.top, SRCCOPY);
    DeleteDC(MemDC);

	FILE *fp;
	_wfopen_s(&fp, fname, L"wb");
    if (fp != NULL) {
        fwrite((char*)&Header, sizeof(Header), 1, fp);
		fwrite((char *)&Info.bmiHeader, sizeof(Info.bmiHeader),1, fp);
		fwrite(Pixels, (((BitsPerPixel * Width + 31) & ~31) / 8) * Height, 1, fp);
		fclose(fp);
    }

    DeleteObject(Section);
}

void SaveBmpFromHDC(const wchar_t* fname, HDC hdc, int width, int height)
{
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = width;
	rect.bottom = height;

	HDCToFile(fname, hdc, rect);
}

/**
 *	ダイアログフォントを取得する
 *	GetMessageboxFontA() の unicode版
 *	エラーは発生しない
 */
void GetMessageboxFontW(LOGFONTW *logfont)
{
	NONCLIENTMETRICSW nci;
	const int st_size = CCSIZEOF_STRUCT(NONCLIENTMETRICSW, lfMessageFont);
	BOOL r;

	memset(&nci, 0, sizeof(nci));
	nci.cbSize = st_size;
	r = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, st_size, &nci, 0);
	assert(r == TRUE);
	*logfont = nci.lfStatusFont;
}

/**
 *	ファイルをランダムに選ぶ
 *
 *		rand() を使っているのでプラグインで利用するときはsrand()すること
 *			srand((unsigned int)time(NULL));
 *
 *	@param[in] filespec_src		入力ファイル名
 *								ワイルドカード("*","?"など)が入っていてもよい、なくてもよい
 *								ワイルドカードが入っているときはランダムにファイルが出力される
 *								%HOME% 等は環境変数として展開される
 *								フルパスでなくてもよい
 *	@return		選ばれたファイル、不要になったらfree()すること
 *				NULL=ファイルがない
 */
wchar_t *RandomFileW(const wchar_t *filespec_src)
{
	int    i;
	int    file_num;
	wchar_t   *fullpath;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t *file_env;
	DWORD e;
	wchar_t *dir;

	if (filespec_src == NULL || filespec_src[0] == 0) {
		return NULL;
	}

	//環境変数を展開
	hExpandEnvironmentStringsW(filespec_src, &file_env);

	//絶対パスに変換
	e = hGetFullPathNameW(file_env, &fullpath, NULL);
	free(file_env);
	file_env = NULL;
	if(e != NO_ERROR) {
		return NULL;
	}

	//ファイルが存在しているか?
	if (GetFileAttributesW(fullpath) != INVALID_FILE_ATTRIBUTES) {
		// マスク("*.jpg"など)がなくファイルが直接指定してある場合
		return fullpath;
	}

	//ファイルを数える
	hFind = FindFirstFileW(fullpath,&fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		free(fullpath);
		return NULL;
	}
	file_num = 0;
	do {
		if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			file_num ++;
	} while(FindNextFileW(hFind,&fd));
	FindClose(hFind);
	if(!file_num) {
		// 0個
		free(fullpath);
		return NULL;
	}

	//何番目のファイルにするか決める。
	file_num = rand()%file_num + 1;

	hFind = FindFirstFileW(fullpath,&fd);
	if(hFind == INVALID_HANDLE_VALUE) {
		free(fullpath);
		return NULL;
	}
	i = 0;
	do{
		if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			i ++;
	} while(i < file_num && FindNextFileW(hFind,&fd));
	FindClose(hFind);

	//ディレクトリ取得
	dir = ExtractDirNameW(fullpath);
	free(fullpath);

	fullpath = NULL;
	awcscats(&fullpath, dir, L"\\", fd.cFileName);
	free(dir);
	return fullpath;
}

/**
 *	ファイルをランダムに選ぶ
 *
 *	@param[in] filespec_src		入力ファイル名
 *								ワイルドカード("*","?"など)が入っていてもよい、なくてもよい
 *								ワイルドカードが入っているときはランダムにファイルが出力される
 *								フルパスでなくてもよい
 *	@param[out]	filename		フルパスファイル名
 *								カレントフォルダ相対でフルパスに変換される
 *								ファイルがないときは [0] = NULL
 *	@param[in]	destlen			filename の領域長
 */
void RandomFile(const char *filespec_src,char *filename, int destlen)
{
	wchar_t *filespec_srcW = ToWcharA(filespec_src);
	wchar_t *one = RandomFileW(filespec_srcW);
	if (one != NULL) {
		WideCharToACP_t(one, filename, destlen);
		free(one);
	}
	else {
		filename[0] = 0;
	}
	free(filespec_srcW);
}

/**
 *	IsDBCSLeadByteEx() とほぼ同じ
 *	ismbblead() の拡張
 *
 *	@param	b			テストする文字
 *	@param	code_page	コードページ,CP_ACPの時現在のコードページ
 */
int __ismbblead(BYTE b, int code_page)
{
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	switch (code_page) {
		case 932:
			// 日本語 shift jis
			if (((0x81 <= b) && (b <= 0x9f)) || ((0xe0 <= b) && (b <= 0xfc))) {
				return TRUE;
			}
			return FALSE;
		case 51949:
			// Korean CP51949
			if ((0xA1 <= b) && (b <= 0xFE)) {
				return TRUE;
			}
			return FALSE;
		case 936:
			// CP936 GB2312
			if ((0xA1 <= b) && (b <= 0xFE)) {
				return TRUE;
			}
			return FALSE;
		case 950:
			// CP950 Big5
			if (((0xA1 <= b) && (b <= 0xc6)) || ((0xc9 <= b) && (b <= 0xf9))) {
				return TRUE;
			}
			return FALSE;
		default:
			break;
	}
	return FALSE;
}

/**
 *	ismbbtrail() の拡張
 *
 *	@param	b			テストする文字
 *	@param	code_page	コードページ,CP_ACPの時現在のコードページ
 */
int __ismbbtrail(BYTE b, int code_page)
{
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	switch (code_page) {
		case 932:
			if (((0x40 <= b) && (b <= 0x7E)) || ((0x80 <= b) && (b <= 0xFC))) {
				return TRUE;
			}
			return FALSE;
		case 51949:
			// Korean CP51949
			if ((0xA1 <= b) && (b <= 0xFE)) {
				return TRUE;
			}
			return FALSE;
		case 936:
			// CP936 GB2312
			if ((0xA1 <= b) && (b <= 0xFE)) {
				return TRUE;
			}
			return FALSE;
		case 950:
			// CP950 Big5
			if (((0x40 <= b) && (b <= 0x7e)) || ((0xa1 <= b) && (b <= 0xfe))) {
				return TRUE;
			}
			return FALSE;
		default:
			break;
	}
	return FALSE;
}

// ファイル名に使用できない文字が含まれているか確かめる
BOOL isInvalidFileNameCharW(const wchar_t *FName)
{
	// ファイルに使用することができない文字
	// cf. Naming Files, Paths, and Namespaces
	//     http://msdn.microsoft.com/en-us/library/aa365247.aspx
	static const wchar_t *invalidFileNameStrings[] = {
		L"AUX", L"CLOCK$", L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
		L"CON", L"CONFIG$", L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9",
		L"NUL", L"PRN",
		L".", L"..",
		NULL
	};

	size_t i, len;
	const wchar_t **p;
	wchar_t c;

	// チェック対象の文字を強化した。(2013.3.9 yutaka)
	p = invalidFileNameStrings;
	while (*p) {
		if (_wcsicmp(FName, *p) == 0) {
			return TRUE;  // Invalid
		}
		p++;
	}

	len = wcslen(FName);
	for (i=0; i<len; i++) {
		if ((FName[i] < ' ') || wcschr(invalidFileNameCharsW, FName[i])) {
			return TRUE;
		}
	}

	// ファイル名の末尾にピリオドおよび空白はNG。
	c = FName[len - 1];
	if (c == '.' || c == ' ')
		return TRUE;

	return FALSE;
}

// ファイル名に使用できない文字を c に置き換える
// c に 0 を指定した場合は文字を削除する
wchar_t *replaceInvalidFileNameCharW(const wchar_t *FName, wchar_t c)
{
	size_t i, j = 0, len;
	wchar_t *dest;

	len = wcslen(FName);
	dest = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));

	if ((c < ' ') || wcschr(invalidFileNameCharsW, c)) {
		c = 0;
	}

	for (i = 0; i < len; i++) {
		if ((FName[i] < ' ') || wcschr(invalidFileNameCharsW, FName[i])) {
			if (c) {
				dest[j++] = c;
			}
		}
		else {
			dest[j++] = FName[i];
		}
	}
	dest[j] = 0;
	return dest;
}

// strftime に渡せない文字が含まれているか確かめる
BOOL isInvalidStrftimeCharW(const wchar_t *format)
{
	size_t i, len, p;

	len = wcslen(format);
	for (i=0; i<len; i++) {
		if (format[i] == '%') {
			if (format[i+1] != 0) {
				p = i+1;
				if (format[i+2] != 0 && format[i+1] == '#') {
					p = i+2;
				}
				switch (format[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						i = p;
						break;
					default:
						return TRUE;
				}
			}
			else {
				// % で終わっている場合はエラーとする
				return TRUE;
			}
		}
	}

	return FALSE;;
}

// strftime に渡せない文字を削除する
void deleteInvalidStrftimeCharW(wchar_t *FName)
{
	size_t i, j=0, len, p;

	len = wcslen(FName);
	for (i=0; i<len; i++) {
		if (FName[i] == '%') {
			if (FName[i+1] != 0) {
				p = i+1;
				if (FName[i+2] != 0 && FName[i+1] == '#') {
					p = i+2;
				}
				switch (FName[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						FName[j] = FName[i]; // %
						j++;
						i++;
						if (p-i == 2) {
							FName[j] = FName[i]; // #
							j++;
							i++;
						}
						FName[j] = FName[i];
						j++;
						break;
					default:
						i++; // %
						if (p-i == 2) {
							i++; // #
						}
				}
			}
			// % で終わっている場合はコピーしない
		}
		else {
			FName[j] = FName[i];
			j++;
		}
	}

	FName[j] = 0;
}
