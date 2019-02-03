/*
 * Copyright (C) 2018-2019 TeraTerm Project
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
#include <string.h>
#include <crtdbg.h>
#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#endif
#include "codeconv.h"

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char	uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int	uint32_t;
#endif

#ifdef _DEBUG
#define malloc(l)     _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)
#define _strdup(s)	  _strdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define _wcsdup(s)    _wcsdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

/**
 * UTF-32 から UTF-8 へ変換する
 * @param[in]		u32		変換するUTF-32
 * @param[in,out]	u8_ptr	変換後UTF-8文字列出力先(NULLのとき出力しない)
 * @param[in]		u8_len	UTF-8出力先文字数(バッファ長,byte数)
 * @retval			使用したutf8文字数(byte数）
 *					0=エラー
 */
size_t UTF32ToUTF8(uint32_t u32, char *u8_ptr_, size_t u8_len)
{
	size_t out_len = 0;
	uint8_t *u8_ptr = (uint8_t *)u8_ptr_;
	if (u8_ptr != NULL) {
		u8_len = 4;
	}

	if (u32 <= 0x0000007f) {
		// 0x00000000 <= u32 <= 0x0000007f
		if (u8_len >= 1) {
			if (u8_ptr != NULL) {
				u8_ptr[0] = (uint8_t)u32;
			}
			out_len = 1;
		}
	} else if (u32 <= 0x000007ff) {
		// 0x00000080 <= u32 <= 0x000007ff
		if (u8_len >= 2) {
			if (u8_ptr != NULL) {
				u8_ptr[0] = ((u32 >> 6) & 0x1f) | 0xc0;
				u8_ptr[1] = (u32 & 0x3f) | 0x80;
			}
			out_len = 2;
		}
	} else if (u32 <= 0x0000ffff) {
		// 0x00000800 <= u32 <= 0x0000ffff
		if (u8_len >= 3) {
			if (u8_ptr != NULL) {
				u8_ptr[0] = ((u32 >> 12) & 0xf) | 0xe0;
				u8_ptr[1] = ((u32 >> 6) & 0x3f) | 0x80;
				u8_ptr[2] = (u32 & 0x3f) | 0x80;
			}
			out_len = 3;
		}
	} else if (u32 <= 0x0010ffff) {
		// 0x00010000 <= u32 <= 0x0010ffff
		if (u8_len >= 4) {
			if (u8_ptr != NULL) {
				u8_ptr[0] = ((uint8_t)(u32 >> 18)) | 0xf0;
				u8_ptr[1] = ((u32 >> 12) & 0x3f) | 0x80;
				u8_ptr[2] = ((u32 >> 6) & 0x3f) | 0x80;
				u8_ptr[3] = (u32 & 0x3f) | 0x80;
			}
			out_len = 4;
		}
	} else {
		out_len = 0;
	}
	return out_len;
}

/**
 * UTF-8文字列からUTF-32を1文字取り出す
 * @param[in]	u8_ptr	UTF-8文字列へのポインタ
 * @param[in]	u8_len	UTF-8文字列長さ
 * @param[out]	u32		変換したUTF-32文字
 * @retval		使用したUTF-8文字数(byte数）
 *				0=エラー
 */
size_t UTF8ToUTF32(const char *u8_ptr_, size_t u8_len, uint32_t *u32_)
{
	uint8_t *u8_ptr = (uint8_t *)u8_ptr_;
	uint32_t u32;
	size_t u8_in;
	const uint8_t c1 = *u8_ptr++;
    if (c1 <= 0x7f) {
		// 1byte
		if (u8_len >= 1) {
			u32 = (uint32_t)c1;
			u8_in = 1;
		} else {
			goto error;
		}
	} else if (0xc2 <= c1 && c1 <= 0xdf) {
		// 2byte
		if (u8_len >= 2) {
			const uint8_t c2 = *u8_ptr++;
			if (((c1 & 0x1e) != 0) &&
				((c2 & 0xc0) == 0x80))
			{
				u32 = (uint32_t)((c1 & 0x1f) << 6) + (c2 & 0x3f);
				u8_in = 2;
			} else {
				goto error;
			}
		} else {
			goto error;
		}
	} else if (0xe0 <= c1 && c1 <= 0xef) {
		// 3byte
		if (u8_len >= 3) {
			const uint8_t c2 = *u8_ptr++;
			const uint8_t c3 = *u8_ptr++;
			if ((((c1 & 0x0f) != 0) || ((c2 & 0x20) != 0)) &&
				((c2 & 0xc0) == 0x80) &&
				((c3 & 0xc0) == 0x80) )
			{
				u32 = (uint32_t)((c1 & 0x0f) << 12) + ((c2 & 0x3f) << 6);
				u32 += (c3 & 0x3f);
				u8_in = 3;
			} else {
				goto error;
			}
		} else {
			goto error;
		}
	} else if (0xf0 <= c1 && c1 <= 0xf7 && u8_len >= 4) {
		// 4byte
		if (u8_len >= 4) {
			const uint8_t c2 = *u8_ptr++;
			const uint8_t c3 = *u8_ptr++;
			const uint8_t c4 = *u8_ptr++;
			if ((((c1 & 0x07) != 0) || ((c2 & 0x30) != 0)) &&
				((c2 & 0xc0) == 0x80) &&
				((c3 & 0xc0) == 0x80) &&
				((c4 & 0xc0) == 0x80) )
			{
				u32 = (uint32_t)((c1 & 0x07) << 18) + ((c2 & 0x3f) << 12);
				u32 += ((c3 & 0x3f) << 6) + (c4 & 0x3f);
				u8_in = 4;
			} else {
				goto error;
			}
		} else {
			goto error;
		}
    } else {
	error:
		u32 = 0;
		u8_in = 0;
	}
	*u32_ = u32;
	return u8_in;
}

// WideCharToMultiByteのUTF8特化版
int WideCharToUTF8(const wchar_t *wstr_ptr, int wstr_len, char *u8_ptr, int u8_len)
{
	int u8_out_sum = 0;
	if (u8_ptr == NULL) {
		u8_len = 4;
	} else {
		if (u8_len == 0) {
			return 0;
		}
	}
	if (wstr_len < 0) {
		wstr_len = (int)wcslen(wstr_ptr) + 1;
	}

	while(u8_len > 0 && wstr_len > 0) {
		const wchar_t u16 = *wstr_ptr++;
		uint32_t u32 = u16;
		size_t u8_out;
		wstr_len--;
		// サロゲート high?
		if (0xd800 <= u16 && u16 < 0xdc00) {
			if (wstr_len >= 1) {
				const wchar_t u16_lo = *wstr_ptr++;
				wstr_len--;
				// サロゲート low?
				if (0xdc00 <= u16_lo && u16_lo < 0xe000) {
					// サロゲートペア デコード
					u32 = 0x10000 + (u16 - 0xd800) * 0x400 + (u16_lo - 0xdc00);
				} else {
					goto unknown_code;
				}
			} else {
			unknown_code:
				if (u8_ptr != NULL) {
					*u8_ptr++ = '?';
				}
				u8_out = 1;
				goto loop_next;
			}
		}
		u8_out = UTF32ToUTF8(u32, u8_ptr, u8_len);
		if (u8_out == 0) {
			goto unknown_code;
		}
	loop_next:
		u8_out_sum += u8_out;
		if (u8_ptr != NULL) {
			u8_ptr += u8_out;
			u8_len -= u8_out;
		}
	}
	return u8_out_sum;
}

// MultiByteToWideCharのUTF8特化版
int UTF8ToWideChar(const char *u8_ptr, int u8_len, wchar_t *wstr_ptr, int wstr_len)
{
	size_t u16_out_sum = 0;
	if (u8_len < 0) {
		u8_len = strlen(u8_ptr) + 1;
	}
	if (wstr_ptr == NULL) {
		wstr_len = 1;
	}

	while(wstr_len > 0 && u8_len > 0) {
		uint32_t u32;
		size_t u16_out;
		size_t u8_in = UTF8ToUTF32(u8_ptr, u8_len, &u32);
		if (u8_in == 0) {
			u32 = '?';
			u8_in = 1;
		}
		u8_ptr += u8_in;
		u8_len -= u8_in;

		if (u32 < 0x10000) {
			if (wstr_ptr != NULL) {
				*wstr_ptr++ = (uint16_t)u32;
			}
			u16_out = 1;
		} else if (u32 <= 0x10ffff) {
			if (wstr_len > 2) {
				if (wstr_ptr != NULL) {
					// サロゲート エンコード
					*wstr_ptr++ = uint16_t((u32 - 0x10000) / 0x400) + 0xd800;
					*wstr_ptr++ = uint16_t((u32 - 0x10000) % 0x400) + 0xdc00;
				}
				u16_out = 2;
			} else {
				*wstr_ptr++ = '?';
				u16_out = 1;
			}
		} else {
			*wstr_ptr++ = '?';
			u16_out = 1;
		}

		if (wstr_ptr != NULL) {
			wstr_len -= u16_out;
		}
		u16_out_sum += u16_out;
	}
	return u16_out_sum;
}

/**
 *	wchar_t文字列をマルチバイト文字列へ変換
 *	@param[in]	*wstr_ptr	wchar_t文字列
 *	@param[in]	wstr_len	wchar_t文字列長(0のとき自動)
 *	@param[in]	code_page	変換先コードページ
 *	@param[out]	*mb_len_	mb文字列長(NULLのとき内部エラー)
 *	@retval		mb文字列へのポインタ(NULLの時変換エラー)
 */
char *_WideCharToMultiByte(const wchar_t *wstr_ptr, size_t wstr_len, int code_page, size_t *mb_len_)
{
	const DWORD flags = 0;
	char *mb_ptr;
	if (mb_len_ != NULL) {
		*mb_len_ = 0;
	}
	if (wstr_len == 0) {
		wstr_len = wcslen(wstr_ptr) + 1;
	}
    int len;
	if (code_page == CP_UTF8) {
		len = WideCharToUTF8(wstr_ptr, (DWORD)wstr_len,
							 NULL, 0);
	} else {
		len = ::WideCharToMultiByte(code_page, flags,
									wstr_ptr, (DWORD)wstr_len,
									NULL, 0,
									NULL, NULL);
	}
	if (len == 0) {
		return NULL;
	}
	mb_ptr = (char *)malloc(len);
	if (mb_ptr == NULL) {
		return NULL;
	}
	if (code_page == CP_UTF8) {
		len = WideCharToUTF8(wstr_ptr, (DWORD)wstr_len,
							 mb_ptr, len);
	} else {
		len = ::WideCharToMultiByte(code_page, flags,
									wstr_ptr, (DWORD)wstr_len,
									mb_ptr, len,
									NULL,NULL);
	}
	if (len == 0) {
		free(mb_ptr);
		return NULL;
	}
	if (mb_len_ != NULL) {
		*mb_len_ = len - 1;
	}
    return mb_ptr;
}

/**
 *	マルチバイト文字列をwchar_t文字列へ変換
 *	@param[in]	*str_ptr	mb(char)文字列
 *	@param[in]	str_len		mb(char)文字列長(0のとき自動)
 *	@param[in]	code_page	変換元コードページ
 *	@param[out]	*w_len_		wchar_t文字列長
 *	@retval		mb文字列へのポインタ(NULLの時変換エラー)
 */
wchar_t *_MultiByteToWideChar(const char *str_ptr, size_t str_len, int code_page, size_t *w_len_)
{
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (code_page == CP_UTF8) {
		// CP_UTF8 When this is set, dwFlags must be zero.
		flags = 0;
	}
	if (w_len_ != NULL) {
		*w_len_ = 0;
	}
	if (str_len == 0) {
		str_len = strlen(str_ptr) + 1;
	}
	int len;
	if (code_page == CP_UTF8) {
		len = UTF8ToWideChar(str_ptr, (int)str_len,
							 NULL, 0);
	} else {
		len = ::MultiByteToWideChar(code_page, flags,
									str_ptr, (int)str_len,
									NULL, 0);
	}
	if (len == 0) {
		return NULL;
	}
	wchar_t *wstr_ptr = (wchar_t *)malloc(len*sizeof(wchar_t));
	if (wstr_ptr == NULL) {
		return NULL;
	}
	if (code_page == CP_UTF8) {
		len = UTF8ToWideChar(str_ptr, (int)str_len,
							 wstr_ptr, len);
	} else {
		len = ::MultiByteToWideChar(code_page, flags,
									str_ptr, (int)str_len,
									wstr_ptr, len);
	}
	if (len == 0) {
		free(wstr_ptr);
		return NULL;
	}
	if (w_len_ != NULL) {
		*w_len_ = len - 1;
	}
	return wstr_ptr;
}

const char *ToCharW(const wchar_t *strW)
{
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
}

const char *ToCharA(const char *strA)
{
	return _strdup(strA);
}

const char *ToCharU8(const char *strU8)
{
	const wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	if (strW == NULL) {
		return NULL;
	}
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	free((void *)strW);
	return strA;
}

const wchar_t *ToWcharA(const char *strA)
{
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	return strW;
}

const wchar_t *ToWcharW(const wchar_t *strW)
{
	return _wcsdup(strW);
}

const wchar_t *ToWcharU8(const char *strU8)
{
	const wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	return strW;
}

const char *ToU8W(const wchar_t *strW)
{
	const char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	return strU8;
}

const char *ToU8A(const char *strA)
{
	const wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	if (strW == NULL) {
		return NULL;
	}
	const char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	free((void *)strW);
	return strU8;
}

//////////////////////////////////////////////////////////////////////////////

u8::u8()
{
	u8str_ = NULL;
}

u8::u8(const char *strA)
{
	u8str_ = NULL;
	assign(strA, CP_ACP);
}

u8::u8(const char *strA, int cpage)
{
	u8str_ = NULL;
	assign(strA, cpage);
}

u8::u8(const wchar_t *strW)
{
	u8str_ = NULL;
	assign(strW);
}

u8::u8(const u8 &obj)
{
	copy(obj);
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
u8::u8(u8 &&obj) noexcept
{
	move(obj);
}
#endif

u8::~u8()
{
	if (u8str_ != NULL) {
		free(u8str_);
		u8str_ = NULL;
	}
}

u8& u8::operator=(const char *strA)
{
	assign(strA, CP_ACP);
	return *this;
}

u8& u8::operator=(const wchar_t *strW)
{
	assign(strW);
	return *this;
}

u8& u8::operator=(const u8 &obj)
{
	copy(obj);
	return *this;
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
u8& u8::operator=(u8 &&obj) noexcept
{
	move(obj);
	return *this;
}
#endif

u8::operator const char *() const
{
	return cstr();
}

const char *u8::cstr() const
{
	if (u8str_ == NULL) {
		return "";
	}
	return u8str_;
}

void u8::assign(const char *strA, int code_page)
{
	if (u8str_ != NULL) {
		free(u8str_);
	}
	wchar_t *strW = _MultiByteToWideChar(strA, 0, code_page, NULL);
	if (strW != NULL) {
		assign(strW);
		free(strW);
	} else {
		u8str_ = NULL;
	}
}

void u8::assign(const wchar_t *strW)
{
	if (u8str_ != NULL) {
		free(u8str_);
	}
	char *u8str = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	if (u8str != NULL) {
		u8str_ = u8str;
	} else {
		u8str_ = NULL;
	}
}

void u8::copy(const u8 &obj)
{
	if (u8str_ != NULL) {
		free(u8str_);
	}
	u8str_ = _strdup(obj.u8str_);
}

void u8::move(u8 &obj)
{
	if (this != &obj) {
		if (u8str_ != NULL) {
			free(u8str_);
		}
		u8str_ = obj.u8str_;
		obj.u8str_ = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////

tc::tc()
{
	tstr_ = NULL;
}

tc::tc(const char *strA)
{
	tstr_ = NULL;
	assign(strA, CP_ACP);
}

tc::tc(const char *strA, int code_page)
{
	tstr_ = NULL;
	assign(strA, code_page);
}

tc::tc(const wchar_t *strW)
{
	tstr_ = NULL;
	assign(strW);
}

tc::tc(const tc &obj)
{
	copy(obj);
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
tc::tc(tc &&obj) noexcept
{
	tstr_ = NULL;
	move(obj);
}
#endif

tc::~tc()
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
}

tc& tc::operator=(const char *strA)
{
	assign(strA, CP_ACP);
	return *this;
}

tc& tc::operator=(const wchar_t *strW)
{
	assign(strW);
	return *this;
}

tc &tc::operator=(const tc &obj)
{
	copy(obj);
	return *this;
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
tc& tc::operator=(tc &&obj) noexcept
{
	move(obj);
	return *this;
}
#endif

tc tc::fromUtf8(const char *strU8)
{
	const wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	tc _tc = strW;
	free((void *)strW);
	return _tc;
}

// voidなしが一般的と思われるが、
// VS2005でリンクエラーが出てしまうため void 追加
tc::operator const TCHAR *(void) const
{
	return cstr();
}

const TCHAR *tc::cstr() const
{
	if (tstr_ == NULL) {
		return _T("");
	}
	return tstr_;
}

void tc::assign(const char *strA, int code_page)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
#if !defined(UNICODE)
	(void)code_page;
	tstr_ = _strdup(strA);
#else
	wchar_t *strW = _MultiByteToWideChar(strA, 0, code_page, NULL);
	if (strW != NULL) {
		tstr_ = strW;
	} else {
		tstr_ = NULL;
	}
#endif
}

void tc::assign(const wchar_t *strW)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
#if defined(UNICODE)
	tstr_ = _wcsdup(strW);
#else
	char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	if (strA != NULL) {
		tstr_ = strA;
	} else {
		tstr_ = NULL;
	}
#endif
}

void tc::copy(const tc &obj)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
	tstr_ = _tcsdup(obj.tstr_);
}

void tc::move(tc &obj)
{
	if (this != &obj) {
		if (tstr_ != NULL) {
			free(tstr_);
		}
		tstr_ = obj.tstr_;
		obj.tstr_ = NULL;
	}
}
