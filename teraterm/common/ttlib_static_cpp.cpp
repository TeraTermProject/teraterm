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

#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "codeconv.h"
#include "compat_win.h"

#include "ttlib.h"

#if _WIN32_WINNT >= 0x0600 // Vista+
#define IFILEOPENDIALOG_ENABLE 1
#endif

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
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		title = TTGetLangStrW(section, info->title_key, info->title_default, UILanguageFile);
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
		wchar_t *format = TTGetLangStrW(section, info->message_key, info->message_default, UILanguageFile);
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
 *	@param	emtpy	TRUEのときクリップボードを空にする
 *	@retval	文字列へのポインタ 使用後free()すること
 *			文字がない(またはエラー時)はNULL
 */
wchar_t *GetClipboardTextW(HWND hWnd, BOOL empty)
{
	UINT Cf;
	wchar_t *str_w = NULL;
	size_t str_w_len;
	HGLOBAL TmpHandle;

	// TODO GetPriorityClipboardFormat()
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		Cf = CF_UNICODETEXT;
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		Cf = CF_TEXT;
	}
	else if (IsClipboardFormatAvailable(CF_OEMTEXT)) {
		Cf = CF_OEMTEXT;
	}
	else if (IsClipboardFormatAvailable(CF_HDROP)) {
		Cf = CF_HDROP;
	}
	else {
		return NULL;
	}

 	if (!OpenClipboard(hWnd)) {
		return NULL;
	}
	TmpHandle = GetClipboardData(Cf);
	if (TmpHandle == NULL) {
		return NULL;
	}
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
 * Get Exe(exe,dll) directory
 *	ttermpro.exe, プラグインがあるフォルダ
 *	ttypes.ExeDirW と同一
 *	もとは GetHomeDirW() だった
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			ExeDir		不要になったら free() すること
 */
wchar_t *GetExeDirW(HINSTANCE hInst)
{
	wchar_t *TempW;
	wchar_t *dir;
	DWORD error = hGetModuleFileNameW(hInst, &TempW);
	if (error != NO_ERROR) {
		// パスの取得に失敗した。致命的、abort() する。
		abort();
	}
	dir = ExtractDirNameW(TempW);
	free(TempW);
	return dir;
}

/*
 * Get home directory
 *		個人用設定ファイルフォルダ取得
 *		ttypes.HomeDirW と同一
 *		TERATERM.INI などがおいてあるフォルダ
 *		ttermpro.exe があるフォルダは GetExeDirW() で取得
 *		%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			HomeDir		不要になったら free() すること
 */
wchar_t *GetHomeDirW(HINSTANCE hInst)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);
	wchar_t *ret = NULL;
	awcscats(&ret, path, L"\\teraterm5", NULL);
	free(path);
	return ret;
}

/*
 * Get log directory
 *		ログ保存フォルダ取得
 *		ttypes.LogDirW と同一
 *		%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @return			LogDir		不要になったら free() すること
 */
wchar_t* GetLogDirW(void)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	wchar_t *ret = NULL;
	awcscats(&ret, path, L"\\teraterm5", NULL);
	free(path);
	return ret;
}

/*
 *	UILanguageFileのフルパスを取得する
 *
 *	@param[in]		HomeDir					exe,dllの存在するフォルダ GetExeDir()で取得できる
 *	@param[in]		UILanguageFileRel		lngファイル、HomeDirからの相対パス
 *	@param[in,out]	UILanguageFileFull		lngファイルptr、フルパス
 *	@param[in]		UILanguageFileFullLen	lngファイルlen、フルパス
 */
wchar_t *GetUILanguageFileFullW(const wchar_t *HomeDir, const wchar_t *UILanguageFileRel)
{
	wchar_t *fullpath;
	size_t size = wcslen(HomeDir) + 1 + wcslen(UILanguageFileRel) + 1;
	wchar_t *rel = (wchar_t *)malloc(sizeof(wchar_t) * size);
	wcscpy(rel, HomeDir);
	wcscat(rel, L"\\");
	wcscat(rel, UILanguageFileRel);
	hGetFullPathNameW(rel, &fullpath, NULL);
	free(rel);
	return fullpath;
}

/**
 *	設定ファイルのフルパスを取得する
 *
 *	@param[in]	home	ttermpro.exe 等の実行ファイルのあるフォルダ
 *						My Documents にファイルがあった場合は使用されない
 *	@param[in]	file	設定ファイル名(パスは含まない)
 *	@return		フルパス (不要になったら free() すること)
 *
 *	- My Documents にファイルがあった場合,
 *		- "%USERPROFILE%\My Documents\{file}"
 *		- "%USERPROFILE%\Documents\{file}" など
 *	- なかった場合
 *		- "{home}\{file}"
 */
wchar_t *GetDefaultFNameW(const wchar_t *home, const wchar_t *file)
{
	// My Documents に file がある場合、
	// それを読み込むようにした。(2007.2.18 maya)
	wchar_t *MyDoc;
	_SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &MyDoc);

	if (MyDoc[0] != 0) {
		// My Documents に file があるか?
		size_t destlen = (wcslen(MyDoc) + wcslen(file) + 1 + 1) * sizeof(wchar_t);
		wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * destlen);
		wcscpy(dest, MyDoc);
		AppendSlashW(dest,destlen);
		wcsncat_s(dest, destlen, file, _TRUNCATE);
		DWORD r = GetFileAttributesW(dest);
		free(MyDoc);
		if (r != INVALID_FILE_ATTRIBUTES) {
			// My Documents の設定ファイル
			return dest;
		}
		free(dest);
	}
	else {
		free(MyDoc);
	}

	size_t destlen = (wcslen(home) + wcslen(file) + 1 + 1) * sizeof(wchar_t);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * destlen);
	wcscpy(dest, home);
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

void GetNthNum(PCHAR Source, int Nth, int far *Num)
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

wchar_t *GetDownloadFolderW(void)
{
	wchar_t *download;
	_SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &download);
	return download;
}

#if IFILEOPENDIALOG_ENABLE
static BOOL doSelectFolderWCOM(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pDialog);
	if (FAILED(hr)) {
		*folder = NULL;
		return FALSE;
	}

	DWORD options;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS);
	pDialog->SetTitle(msg);
	{
		IShellItem *psi;
		hr = SHCreateItemFromParsingName(def, NULL, IID_IShellItem, (void **)&psi);
		if (SUCCEEDED(hr)) {
			hr = pDialog->SetFolder(psi);
			psi->Release();
		}
	}
	hr = pDialog->Show(hWnd);

	BOOL result = FALSE;
	if (SUCCEEDED(hr)) {
		IShellItem *pItem;
		hr = pDialog->GetResult(&pItem);
		if (SUCCEEDED(hr)) {
			PWSTR pPath;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
			if (SUCCEEDED(hr)) {
				*folder = _wcsdup(pPath);
				CoTaskMemFree(pPath);
				result = TRUE;
			}
		}
	}

	if (!result) {
		// cancel(or some error)
		*folder = NULL;
	}
	pDialog->Release();
	return result;
}
#else
static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg) {
	case BFFM_INITIALIZED: {
		// 初期化時

		const wchar_t *folder = (wchar_t *)lpData;
		if (folder == NULL || folder[0] == 0) {
			// 選択フォルダが指定されていない
			break;
		}
		// フォルダを選択状態にする
		SendMessageW(hwnd, BFFM_SETSELECTIONW, (WPARAM)TRUE, (LPARAM)folder);
		break;
	}
	default:
		break;
	}
	return 0;
}

static BOOL doSelectFolderWAPI(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	wchar_t buf[MAX_PATH];
	BROWSEINFOW bi = {};
	bi.hwndOwner = hWnd;
	bi.pidlRoot = 0;	// 0 = from desktop
	bi.pszDisplayName = buf;
	bi.lpszTitle = msg;
	bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallback;
	bi.lParam = (LPARAM)def;
	LPITEMIDLIST pidlBrowse = SHBrowseForFolderW(&bi);
	if (pidlBrowse == NULL) {
		*folder = NULL;
		return FALSE;
	}

	// PIDL形式の戻り値のファイルシステムのパスに変換
	// TODO SHGetPathFromIDListEx() へ切り替え?
	if (!SHGetPathFromIDListW(pidlBrowse, buf)) {
		return FALSE;
	}
	*folder = _wcsdup(buf);
	CoTaskMemFree(pidlBrowse);
	return TRUE;
}
#endif

/**
 *	フォルダを選択する
 *	SHBrowseForFolderW() をコールする
 *
 *	@param[in]	def			選択フォルダの初期値(特に指定しないときは NULL or "")
 *	@param[out]	**folder	選択したフォルダのフルパス(キャンセル時はセットされない)
 *							不要になったら free() すること(キャンセル時はfree()不要)
 *	@retval	TRUE	選択した
 *	@retval	FALSE	キャンセルした
 *
 */
BOOL doSelectFolderW(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	// TODO 両立したい
#if IFILEOPENDIALOG_ENABLE
	return doSelectFolderWCOM(hWnd, def, msg, folder);
#else
	return doSelectFolderWAPI(hWnd, def, msg, folder);
#endif
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
	wchar_t *dest_end = dest + dest_size - 1;

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

/**
 *	プログラムを実行する
 *
 *	@param[in]	command		実行するコマンドライン
 *							CreateProcess() にそのまま渡される
 * 	@retval		NO_ERROR	エラーなし
 *	@retval		エラーコード	(NO_ERROR以外)
 *
 *	シンプルにプログラムを起動するだけの関数
 *		CreateProcess() は CloseHandle() を忘れてハンドルリークを起こしやすい
 *		単純なプログラム実行ではこの関数を使用すると安全
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

DWORD TTWinExecA(const char *commandA)
{
	wchar_t *commandW = ToWcharA(commandA);
	DWORD e = TTWinExec(commandW);
	free(commandW);
	return e;
}
