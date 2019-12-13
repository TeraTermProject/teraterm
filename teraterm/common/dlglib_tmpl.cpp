/*
 * (C) 2005-2019 TeraTerm Project
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

/* Routines for dialog boxes */

#include "dlglib.h"

#include <wchar.h>
#include <assert.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

// https://docs.microsoft.com/ja-jp/windows/desktop/dlgbox/dlgtemplateex
// https://www.pg-fl.jp/program/tips/dlgmem.htm
#pragma pack(push, 1)
typedef struct
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;

	// sz_Or_Ord menu;				// name or ordinal of a menu resource
	// sz_Or_Ord windowClass;		// name or ordinal of a window class
	// WCHAR title[N];
	// WORD pointsize;				// only if DS_SETFONT is set
	// WORD weight;					// only if DS_SETFONT is set
    // BYTE italic;					// only if DS_SETFONT is set
    // BYTE charset;				// only if DS_SETFONT is set
	// WCHAR typeface[stringLen];	// only if DS_SETFONT is set

	// PAD

    // DLGITEMTEMPLATEEX controls[cDlgItems];
} DLGTEMPLATEEX;

typedef struct
{
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    short x;
    short y;
    short cx;
    short cy;
    DWORD id;

	// sz_Or_Ord windowClass;	// name or ordinal of a window class
	// sz_Or_Ord title;			// title string or ordinal of a resource
	// WORD extraCount;			// bytes following creation data
} DLGITEMTEMPLATEEX;
#pragma pack(pop)

#if 0 // !defined(_MSC_VER)
static inline errno_t wcscpy_s(
	wchar_t *dest,
	size_t dest_size,
	const wchar_t *src)
{
	wcscpy(dest, src);
	return 0;
}
#endif

static size_t CopySz(const WORD *src, WORD *dest)
{
	size_t size = 0;
	do {
		if (dest != NULL) {
			*dest++ = *src;
		}
		size++;
	} while (*src++ != 0);
	return size;
}

static size_t CopySzOrOrd(const WORD *src, WORD *dest)
{
	size_t size;
	if (*src == 0x0000) {
		// 0x0000 のとき、なにもない
		if (dest != NULL) {
			*dest = *src;
		}
		size = 1;
	} else if (*src == 0xffff) {
		// 0xffff のとき、1WORDのデータ
		if (dest != NULL) {
			*dest++ = *src++;
			*dest++ = *src++;
		}
		size = 2;
	} else {
		// 以外はwchar_tの文字列
		return CopySz(src, dest);
	}
	return size;
}

static size_t CopyDlgItem(const WORD *src, WORD *dest)
{
	size_t size = sizeof(DLGITEMTEMPLATE) / sizeof(WORD);
	if (dest != NULL) {
		*(DLGITEMTEMPLATE *)dest = *(DLGITEMTEMPLATE *)src;
		dest += size;
	}
	src += size;

	size_t t;
	t = CopySzOrOrd(src, dest);		// windowClass
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySzOrOrd(src, dest);		// title
	size += t;
	src += t; if (dest != NULL) dest += t;
	const WORD extraCount = *src++;
	size++;
	if (dest != NULL) {
		*dest++ = extraCount;
	}
	if (extraCount != 0) {
		size += extraCount;
		if (dest != NULL) {
			memcpy(dest, src, extraCount * sizeof(WORD));
			dest += extraCount;
		}
		src += extraCount;
	}

	size = (size + 1) & ~1;
	return size;
}

/**
 *	drc		ダイアログリソースへのポインタ
 *	dest	コピー先(NULLのときコピーしない)
 *	logfont	設定するフォント情報(ただし、dest_vがNULLの時は取得)
 *			NULLのとき設定,取得しない
 */
static size_t CopyDlgTemplate(
	const WORD *src_v,
	WORD *dest_v,
	LOGFONTW *logfont)
{
	const WORD *src = (const WORD *)src_v;
	WORD *dest = (WORD *)dest_v;
	size_t size = sizeof(DLGTEMPLATE) / sizeof(WORD);
	const DLGTEMPLATE *dlg = (DLGTEMPLATE *)src;
	if (dest != NULL) {
		*(DLGTEMPLATE *)dest = *dlg;
		dest += size;
	}
	src += size;

	size_t t;
	t = CopySzOrOrd(src, dest);	// menu
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySzOrOrd(src, dest);	// windowClass
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySz(src, dest);		// title
	size += t;
	src += t; if (dest != NULL) dest += t;
	size_t dsize = 0;
	if (dlg->style & DS_SETFONT) {
		if (dest == NULL) {
			// 取得
			if (logfont != NULL) {
				memset(logfont, 0, sizeof(*logfont));
				logfont->lfHeight = *src++;
				t = CopySz(src, (WORD *)(&logfont->lfFaceName[0]));
			} else {
				src++;
				t = CopySz(src, NULL);
			}
			size += t + 1;
		} else {
			// セット
			if (logfont != NULL) {
				*dest++ = (WORD)logfont->lfHeight;
				src += 1;
				t = CopySz((WORD *)(&logfont->lfFaceName[0]), dest);
				dest += t;
				dsize = size + t + 1;
				t = CopySz(src, NULL);
				size += t + 1;
			} else {
				*dest++ = *src++;
				t = CopySz(src, dest);
				size += t + 1;
				dsize = size;
			}
		}
	}

	size = (size + 1) & ~1;
	src = ((const WORD *)src_v) + size;
	if (dest != NULL) {
		dsize = (dsize + 1) & ~1;
		dest = ((WORD *)dest_v) + dsize;
		size = dsize;
	}
	for (int i = 0; i < dlg->cdit; i++) {
		t = CopyDlgItem(src, dest);
		src += t; if (dest != NULL) dest += t;
		size += t;
	}

	return size * sizeof(WORD);
}

static size_t CopyDlgItemEx(const WORD *src, WORD *dest)
{
	size_t size = sizeof(DLGITEMTEMPLATEEX) / sizeof(WORD);
	if (dest != NULL) {
		*(DLGITEMTEMPLATEEX *)dest = *(DLGITEMTEMPLATEEX *)src;
		dest += size;
	}
	src += size;

	size_t t;
	t = CopySzOrOrd(src, dest);		// windowClass
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySzOrOrd(src, dest);		// title
	size += t;
	src += t; if (dest != NULL) dest += t;
	const WORD extraCount = *src++;
	size++;
	if (dest != NULL) {
		*dest++ = extraCount;
	}
	if (extraCount != 0) {
		size += extraCount;
		if (dest != NULL) {
			memcpy(dest, src, extraCount * sizeof(WORD));
			dest += extraCount;
		}
		src += extraCount;
	}

	size = (size + 1) & ~1;
	return size;
}

/**
 *	drc_v	ダイアログリソースへのポインタ
 *	dest_v	コピー先(NULLのときコピーしない)
 *	logfont	設定するフォント情報(ただし、dest_vがNULLの時は取得)
 *			NULLのとき設定,取得しない
 */
static size_t CopyDlgTemplateEx(
	const DLGTEMPLATE *src_v,
	DLGTEMPLATE *dest_v,
	LOGFONTW *logfont)
{
	const WORD *src = (const WORD *)src_v;
	WORD *dest = (WORD *)dest_v;
	if (*src != 1) {
		return CopyDlgTemplate(src, dest, logfont);
	}
	// version 1
	size_t size = sizeof(DLGTEMPLATEEX) / sizeof(WORD);
	const DLGTEMPLATEEX *dlg = (DLGTEMPLATEEX *)src;
	if (dest != NULL) {
		*(DLGTEMPLATEEX *)dest = *dlg;
		dest += size;
	}
	src += size;

	size_t t;
	t = CopySzOrOrd(src, dest);	// menu
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySzOrOrd(src, dest);	// windowClass
	size += t;
	src += t; if (dest != NULL) dest += t;
	t = CopySz(src, dest);		// title
	size += t;
	src += t; if (dest != NULL) dest += t;
	size_t dsize = 0;
	if (dlg->style & DS_SETFONT) {
		if (dest == NULL) {
			// 取得
			if (logfont != NULL) {
				memset(logfont, 0, sizeof(*logfont));
				logfont->lfHeight = *src++;
				logfont->lfWeight = *src++;
				logfont->lfItalic = *((BYTE *)src);
				logfont->lfCharSet = *(((BYTE *)src)+1);
				src++;
				t = CopySz(src, (WORD *)(&logfont->lfFaceName[0]));
			} else {
				src++;
				src++;
				src++;
				t = CopySz(src, NULL);
			}
			size += t + 3;
		} else {
			// セット
			if (logfont != NULL) {
				*dest++ = (WORD)logfont->lfHeight;
				*dest++ = (WORD)logfont->lfWeight;
				*((BYTE *)dest) = logfont->lfItalic;
				*(((BYTE *)dest)+1) = logfont->lfCharSet;
				dest++;
				src += 3;
				t = CopySz((WORD *)(&logfont->lfFaceName[0]), dest);
				dest += t;
				dsize = size + t + 3;
				t = CopySz(src, NULL);
				size += t + 3;
			} else {
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				t = CopySz(src, dest);
				size += t + 3;
				dsize = size;
			}
		}
	}

	size = (size + 1) & ~1;
	src = ((const WORD *)src_v) + size;
	if (dest != NULL) {
		dsize = (dsize + 1) & ~1;
		dest = ((WORD *)dest_v) + dsize;
		size = dsize;
	}
	for (int i = 0; i < dlg->cDlgItems; i++) {
		t = CopyDlgItemEx(src, dest);
		src += t; if (dest != NULL) dest += t;
		size += t;
	}

	return size * sizeof(WORD);
}

static DLGTEMPLATE *GetDlgTemplate(
	const DLGTEMPLATE *src,
	const WCHAR *FontFaceName, LONG FontHeight, BYTE FontCharSet,
	size_t *PrevTemplSize, size_t *NewTemplSize)
{
	LOGFONTW logfont;
	const size_t prev_size = CopyDlgTemplateEx(src, NULL, &logfont);
	DLGTEMPLATE *dest;
	size_t new_size = 0;
	if (FontFaceName == NULL || FontFaceName[0] == '\0') {
		// simple copy
		dest = (DLGTEMPLATE *)malloc(prev_size);
		CopyDlgTemplateEx(src, dest, NULL);
		new_size = prev_size;
	} else {
		// copy with replacing font
		int size_namediff =
			wcslen(FontFaceName) - wcslen(logfont.lfFaceName);
		size_namediff *= sizeof(WCHAR);
		size_namediff += 3;		// テキスト、フォント名長でalignが変化する
		size_t new_size_forcast = prev_size + size_namediff;
		new_size_forcast = (new_size_forcast + 3) & ~3;
		dest = (DLGTEMPLATE *)malloc(new_size_forcast);
		logfont.lfCharSet = FontCharSet;
		logfont.lfHeight = FontHeight;
		wcscpy_s(logfont.lfFaceName, _countof(logfont.lfFaceName),
				 FontFaceName);
		new_size = CopyDlgTemplateEx(src, dest, &logfont);
		assert(new_size <= new_size_forcast);
	}

	if (PrevTemplSize != NULL) {
		*PrevTemplSize = prev_size;
	}
	if (NewTemplSize != NULL) {
		*NewTemplSize = new_size;
	}
	return dest;
}

static DLGTEMPLATE *GetDlgTemplate(
	HINSTANCE hInst, LPCTSTR lpTemplateName,
	const WCHAR *FontFaceName, LONG FontHeight, BYTE FontCharSet,
	size_t *PrevTemplSize, size_t *NewTemplSize)
{
	HRSRC hResource = ::FindResource(hInst, lpTemplateName, RT_DIALOG);
	assert(hResource != NULL);
	HANDLE hDlgTemplate = ::LoadResource(hInst, hResource);
	const DLGTEMPLATE *src = (DLGTEMPLATE *)::LockResource(hDlgTemplate);

	DLGTEMPLATE *dest = GetDlgTemplate(
		src,
		FontFaceName, FontHeight, FontCharSet,
		PrevTemplSize, NewTemplSize);

	::FreeResource(hDlgTemplate);

	return dest;
}

static wchar_t FontFaceName[LF_FACESIZE];
static LONG FontHeight;
static BYTE FontCharSet;

/**
 *	ダイアログのフォントを設定する
 *	heightの単位はポイント
 */
void TTSetDlgFontW(const wchar_t *face, int height, int charset)
{
	assert(height > 0);
	if (face != NULL) {
		wcscpy_s(FontFaceName, face);
	} else {
		FontFaceName[0] = L'\0';
	}
	FontHeight = height;
	FontCharSet = (BYTE)charset;
}

/**
 *	ダイアログのフォントを設定する
 *	heightの単位はポイント
 */
void TTSetDlgFontA(const char *face, int height, int charset)
{
	assert(height > 0);
	if (face != NULL) {
		MultiByteToWideChar(CP_ACP, 0, face, -1, FontFaceName, LF_FACESIZE);
	} else {
		FontFaceName[0] = L'\0';
	}
	FontHeight = height;
	FontCharSet = (BYTE)charset;
}

DLGTEMPLATE *TTGetNewDlgTemplate(
	HINSTANCE hInst, const DLGTEMPLATE *src,
	size_t *PrevTemplSize, size_t *NewTemplSize)
{
	(void)hInst;
	DLGTEMPLATE *DlgTemplate =
		GetDlgTemplate(src,
					   FontFaceName, FontHeight, FontCharSet,
					   PrevTemplSize, NewTemplSize);

	return DlgTemplate;
}

DLGTEMPLATE *TTGetDlgTemplate(HINSTANCE hInst, LPCTSTR lpTemplateName)
{
	DLGTEMPLATE *DlgTemplate =
		GetDlgTemplate(hInst, lpTemplateName,
					   FontFaceName, FontHeight, FontCharSet,
					   NULL, NULL);
	return DlgTemplate;
}

/*
 *	ダイアログテンプレートのクラス名取得
 *	@retval		クラス文字列
 *	@retval		NULL クラスなし
 */
const wchar_t *TTGetClassName(const DLGTEMPLATE *DlgTempl)
{
	const WORD *src = (const WORD *)DlgTempl;
	if (*src != 1) {
		// DLGTEMPLATE
		src += sizeof(DLGTEMPLATE) / sizeof(WORD);
	} else {
		// DLGTEMPLATEEX
		src += sizeof(DLGTEMPLATEEX) / sizeof(WORD);
	}
	size_t t = CopySzOrOrd(src, NULL);	// menu
	src += t;

	if (*src == L'\0') {
		// no class name
		return NULL;
	}
	return (wchar_t *)src;
}
