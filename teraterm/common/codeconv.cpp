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

/* unicode関連の文字コード変換 */

#include <windows.h>
#include <string.h>
#include <assert.h>
#include <crtdbg.h>
#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#endif
#include "codemap.h"
#include "codeconv.h"

// cp932変換時、Windows API より Tera Term の変換テーブルを優先する
//#define PRIORITY_CP932_TABLE

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char	uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int	uint32_t;
#endif

#if defined(_DEBUG) && !defined(_CRTDBG_MAP_ALLOC)
#define malloc(l)     _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)
#define _strdup(s)	  _strdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define _wcsdup(s)    _wcsdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

/*
 *	見つからない場合は 0 を返す
 */
static unsigned short _ConvertUnicode(unsigned short code, const codemap_t *table, int tmax)
{
	int low, mid, high;
	unsigned short result;

	low = 0;
	high = tmax - 1;
	result = 0; // convert error

	// binary search
	while (low < high) {
		mid = (low + high) / 2;
		if (table[mid].from_code < code) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	if (table[low].from_code == code) {
		result = table[low].to_code;
	}

	return (result);
}

static int IsHighSurrogate(wchar_t u16)
{
	return 0xd800 <= u16 && u16 < 0xdc00;
}

static int IsLowSurrogate(wchar_t u16)
{
	return 0xdc00 <= u16 && u16 < 0xe000;
}

/**
 * CP932文字(Shift_JIS) 1文字からUTF-32へ変換する
 * @param[in]		cp932		CP932文字
 * @retval			変換したUTF-32文字
 *					0=エラー(変換できなかった)
 */
unsigned int CP932ToUTF32(unsigned short cp932)
{
#include "../ttpcmn/sjis2uni.map"		// mapSJISToUnicode[]
	wchar_t wchar;
	int ret;
	unsigned int u32;
	unsigned char buf[2];
	int len = 0;

#if defined(PRIORITY_CP932_TABLE)
	u32 = _ConvertUnicode(cp932, mapSJISToUnicode, sizeof(mapSJISToUnicode)/sizeof(mapSJISToUnicode[0]));
	if (u32 != 0) {
		return u32;
	}
#endif
	if (cp932 < 0x100) {
		buf[0] = cp932 & 0xff;
		len = 1;
	} else {
		buf[0] = cp932 >> 8;
		buf[1] = cp932 & 0xff;
		len = 2;
	}
	ret = ::MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, (char *)buf, len, &wchar, 1);
	if (ret <= 0) {
		// MultiByteToWideChar()が変換失敗
#if !defined(PRIORITY_CP932_TABLE)
		u32 = _ConvertUnicode(cp932, mapSJISToUnicode, sizeof(mapSJISToUnicode)/sizeof(mapSJISToUnicode[0]));
		// テーブルにもなかった場合 c = 0(変換失敗時)
#else
		u32 = 0;
#endif
	} else {
		u32 = (unsigned int)wchar;
	}

	return u32;
}

/**
 * UnicodeからDEC特殊文字へ変換
 * @param	u32			UTF-32文字コード
 * @return	下位8bit	DEC特殊文字コード
 *			上位8bit	文字コード種別 (1,2,4)
 *						file://../../doc/ja/html/setup/teraterm-term.html 参照
 *			0			変換できなかった
 */
unsigned short UTF32ToDecSp(unsigned int u32)
{
#include "../teraterm/unisym2decsp.map"		// mapUnicodeSymbolToDecSp[]
	unsigned short cset;
	if (u32 > 0x10000) {
		cset = 0;
	} else {
		const unsigned short u16 = (unsigned short)u32;
		cset = _ConvertUnicode(u16, mapUnicodeSymbolToDecSp, _countof(mapUnicodeSymbolToDecSp));
	}
	return cset;
}

/**
 *	code page の mulit byte 文字を UTF-32へ変換する
 *	@param mb_code		マルチバイトの文字コード(0x0000-0xffff)
 *	@param code_page	マルチバイトのコードページ
 *	@retval				unicode(UTF-32文字コード)
 *						0=エラー(変換できなかった)
 */
unsigned int MBCP_UTF32(unsigned short mb_code, int code_page)
{
	unsigned int c;

	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	if (code_page == 932) {
		c = CP932ToUTF32(mb_code);
	} else {
		char buf[2];
		wchar_t wchar;
		int ret;
		int len = 0;
		if (mb_code < 0x100) {
			buf[0] = mb_code & 0xff;
			len = 1;
		} else {
			buf[0] = mb_code >> 8;
			buf[1] = mb_code & 0xff;
			len = 2;
		}
		ret = ::MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, buf, len, &wchar, 1);
		if (ret <= 0) {
			c = 0;
		} else {
			c = (unsigned int)wchar;
		}
	}
	return c;
}

/**
 * UTF-32文字をCP932文字(Shift_JIS) 1文字へ変換する
 * @retval		使用したCP932文字
 *				0=エラー(変換できなかった)
 */
unsigned short UTF32_CP932(unsigned int u32)
{
#include "../teraterm/uni2sjis.map"		// mapUnicodeToSJIS[]
	char mbstr[2];
	unsigned short mb;
	DWORD mblen;
	wchar_t u16_str[2];
	size_t u16_len;
	BOOL use_default_char;

	if (u32 < 0x80) {
		return (unsigned short)u32;
	}

#if defined(PRIORITY_CP932_TABLE)
	if (u32 < 0x10000) {
		wchar_t u16 = (wchar_t)u32;
		// Tera Termの変換テーブルで Unicode -> Shift_JISへ変換
		mb = _ConvertUnicode(u16, mapUnicodeToSJIS, _countof(mapUnicodeToSJIS));
		if (mb != 0) {
			// 変換できた
			return mb;
		}
	}
#endif
	u16_len = UTF32ToUTF16(u32, u16_str, 2);
	if (u16_len == 0) {
		return 0;
	}
	use_default_char = FALSE;
	mblen = ::WideCharToMultiByte(932, 0, u16_str, (int)u16_len, mbstr, 2, NULL, &use_default_char);
	if (use_default_char) {
		// 変換できず、既定の文字を使った
		goto next_convert;
	}
	switch (mblen) {
	case 0:
		// 変換失敗
		goto next_convert;
	case 1:
		mb = (unsigned char)mbstr[0];
		return mb;
	case 2:
		mb = (((unsigned char)mbstr[0]) << 8) | (unsigned char)mbstr[1];
		return mb;
	}

next_convert:
#if !defined(PRIORITY_CP932_TABLE)
	if (u32 < 0x10000) {
		wchar_t u16 = (wchar_t)u32;
		// Tera Termの変換テーブルで Unicode -> Shift_JISへ変換
		mb = _ConvertUnicode(u16, mapUnicodeToSJIS, _countof(mapUnicodeToSJIS));
		if (mb != 0) {
			// 変換できた
			return mb;
		}
	}
#endif
	return 0;
}

/**
 * UTF-8文字列からUTF-32を1文字取り出す
 * @param[in]	u8_ptr	UTF-8文字列へのポインタ
 * @param[in]	u8_len	UTF-8文字列長さ
 * @param[out]	u32		変換したUTF-32文字
 * @retval		使用したUTF-8文字数(byte数）
 *				0=エラー(変換できなかった)
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
	} else if (0xf0 <= c1 && c1 <= 0xf7) {
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

/**
 *	wchar_t文字列からunicode(UTF-32)を1文字取り出す
 *	@retval	0	文字として扱えない(文字コードがおかしい)
 *	@retval	1	1キャラクタで1文字として扱える
 *	@retval	2	2キャラクタで1文字として扱える
 */
size_t UTF16ToUTF32(const wchar_t *wstr_ptr, size_t wstr_len, unsigned int *u32)
{
	assert(wstr_ptr != NULL);
	if (wstr_len == 0) {
		*u32 = 0;
		return 0;
	}
	const wchar_t u16 = *wstr_ptr++;
	// サロゲート high?
	if (IsHighSurrogate(u16)) {
		if (wstr_len >= 2) {
			const wchar_t u16_lo = *wstr_ptr++;
			if (IsLowSurrogate(u16_lo)) {
				// サロゲートペア デコード
				*u32 = 0x10000 + (u16 - 0xd800) * 0x400 + (u16_lo - 0xdc00);
				return 2;
			} else {
				*u32 = 0;
				return 0;
			}
		} else {
			*u32 = 0;
			return 0;
		}
	} else if (IsLowSurrogate(u16)) {
		*u32 = 0;
		return 0;
	} else {
		*u32 = u16;
		return 1;
	}
}

/**
 *	マルチバイト文字(code_page) からunicode(UTF-32)を1文字取り出す
 *	@retval	0	文字として扱えない(文字コードがおかしい)
 *	@retval	1	1キャラクタで1文字として扱える
 *	@retval	2	2キャラクタで1文字として扱える
 */
size_t MBCPToUTF32(const char *mb_ptr, size_t mb_len, int code_page, unsigned int *u32)
{
	size_t input_len;
	wchar_t u16_str[2];
	size_t u16_len;

	assert(mb_ptr != NULL);
	if (mb_len == 0) {
		*u32 = 0;
		return 0;
	}
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}

	input_len = 1;
	while(1) {
		u16_len = ::MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS,
										mb_ptr, (int)input_len,
										u16_str, 2);
		if (u16_len != 0) {
			size_t r = UTF16ToUTF32(u16_str, u16_len, u32);
			assert(r != 0);
			if (r == 0) {
				// ないはず
				return 0;
			} else {
				return input_len;
			}
		}

		input_len++;
		if (input_len > mb_len) {
			*u32 = 0;
			return 0;
		}
	}
}

/**
 * UTF-32文字 から UTF-8 へ変換する
 * @param[in]		u32		変換するUTF-32
 * @param[in,out]	u8_ptr	変換後UTF-8文字列出力先(NULLのとき出力しない)
 * @param[in]		u8_len	UTF-8出力先文字数(バッファ長,byte数)
 * @retval			出力したutf8文字数(byte数）
 *					0=エラー
 */
size_t UTF32ToUTF8(uint32_t u32, char *u8_ptr_, size_t u8_len)
{
	size_t out_len = 0;
	uint8_t *u8_ptr = (uint8_t *)u8_ptr_;
	if (u8_ptr == NULL) {
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
 * UTF-32 から UTF-16 へ変換する
 * @param[in]		u32			変換するUTF-32
 * @param[in,out]	wstr_ptr	変換後UTF-16文字列出力先(NULLのとき出力しない)
 * @param[in]		wstr_len	UTF-16出力先文字数(文字数,sizeof(wchar_t)*wstr_len bytes)
 * @retval			出力したUTF-16文字数(sizeof(wchar_t)倍するとbyte数)
 *					0=エラー(変換できなかった)
 */
size_t UTF32ToUTF16(uint32_t u32, wchar_t *wstr_ptr, size_t wstr_len)
{
	size_t u16_out;
	if (wstr_ptr == NULL) {
		wstr_len = 2;
	}
	if (wstr_len == 0) {
		return 0;
	}
	if (u32 < 0x10000) {
		if (wstr_len >= 1) {
			if (wstr_ptr != NULL) {
				*wstr_ptr++ = (uint16_t)u32;
			}
			u16_out = 1;
		} else {
			u16_out = 0;
		}
	} else if (u32 <= 0x10ffff) {
		if (wstr_len >= 2) {
			if (wstr_ptr != NULL) {
				// サロゲート エンコード
				*wstr_ptr++ = uint16_t((u32 - 0x10000) / 0x400) + 0xd800;
				*wstr_ptr++ = uint16_t((u32 - 0x10000) % 0x400) + 0xdc00;
			}
			u16_out = 2;
		} else {
			u16_out = 0;
		}
	} else {
		u16_out = 0;
	}
	return u16_out;
}

/**
 * UTF-32 から CP932 へ変換する
 * @param[in]		u32			変換するUTF-32
 * @param[in,out]	mb_ptr		変換後CP932文字列出力先(NULLのとき出力しない)
 * @param[in]		mb_len		CP932出力先文字数(文字数,sizeof(wchar_t)*wstr_len bytes)
 * @retval			出力したCP932文字数(byte数)
 *					0=エラー(変換できなかった)
 */
size_t UTF32ToCP932(uint32_t u32, char *mb_ptr, size_t mb_len)
{
	uint16_t cp932;
	size_t cp932_out;
	if (mb_ptr == NULL) {
		mb_len = 2;		// 2byteあれば足りるはず
	}
	if (mb_len == 0) {
		// 出力先サイズが0
		return 0;
	}
	if (u32 == 0) {
		if (mb_ptr != NULL) {
			*mb_ptr = 0;
		}
		return 1;
	}
	cp932 = UTF32_CP932(u32);
	if (cp932 == 0) {
		// 変換できなかった
		return 0;
	}
	if (mb_ptr == NULL) {
		mb_len = 2;
	}
	if (cp932 < 0x100) {
		if (mb_len >= 1) {
			if (mb_ptr != NULL) {
				*mb_ptr = cp932 & 0xff;
			}
			cp932_out = 1;
		} else {
			cp932_out = 0;
		}
	} else {
		if (mb_len >= 2) {
			if (mb_ptr != NULL) {
				mb_ptr[0] = (cp932 >> 8) & 0xff;
				mb_ptr[1] = cp932 & 0xff;
			}
			cp932_out = 2;
		} else {
			cp932_out = 0;
		}
	}
	return cp932_out;
}

/**
 * UTF-32 から MultiByte文字(code_page) へ変換する
 * @param[in]		u32			変換元UTF-32
 * @param[in]		code_page	変換先codepage
 * @param[in,out]	mb_ptr		変換先文字列出力先(NULLのとき出力しない)
 * @param[in]		mb_len		CP932出力先文字数(文字数,sizeof(wchar_t)*wstr_len bytes)
 * @retval			出力したmultibyte文字数(byte数)
 *					0=エラー(変換できなかった)
 */
size_t UTF32ToMBCP(unsigned int u32, int code_page, char *mb_ptr, size_t mb_len)
{
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	if (code_page == 932) {
		return UTF32ToCP932(u32, mb_ptr, mb_len);
	} else {
		BOOL use_default_char;
		wchar_t u16_str[2];
		size_t u16_len;
		u16_len = UTF32ToUTF16(u32, u16_str, 2);
		if (u16_len == 0) {
			return 0;
		}
		use_default_char = FALSE;
		mb_len = ::WideCharToMultiByte(code_page, 0, u16_str, (int)u16_len, mb_ptr, (int)mb_len, NULL, &use_default_char);
		if (use_default_char) {
			// 変換できず、既定の文字を使った
			return 0;
		}
		return mb_len;
	}
}

/**
 *	wchar_t(UTF-16)文字列をマルチバイト文字列に変換する
 *	変換できない文字は '?' で出力する
 *
 *	@param[in]		*wstr_ptr	wchar_t文字列
 *	@param[in,out]	*wstr_len	wchar_t文字列長
 *								NULLまたは0のとき自動(L'\0'でターミネートすること)
 *								NULL以外のとき入力した文字数を返す
 *	@param[in]		*mb_ptr		変換した文字列を収納するポインタ
 *								(NULLのとき変換せずに文字数をカウントする)
 *	@param[in,out]	*mb_len		変換した文字列を収納できるサイズ,byte数,
 *								mb_ptrがNULLのとき出力可能サイズは不要
 *								変換したマルチバイト文字列の長さを返す
 *								L'\0'を変換したら'\0'も含む
 *								mb_ptrがNULLのときでも長さは返す
 *	@param[in]		UTF32ToMB	UTF32をマルチバイトに変換する関数へのポインタ
 */
static void WideCharToMB(const wchar_t *wstr_ptr, size_t *wstr_len_,
						 char *mb_ptr, size_t *mb_len_,
						 size_t (*UTF32ToMB)(uint32_t u32, char *mb_ptr, size_t mb_len))
{
	size_t wstr_len;
	size_t mb_len;
	size_t mb_out_sum = 0;
	size_t wstr_in = 0;

	assert(wstr_ptr != NULL);
	if (mb_ptr == NULL) {
		// 変換文字列を書き出さない
		mb_len = 4;		// 1文字4byteには収まるはず
	} else {
		mb_len = *mb_len_;
	}
	if (wstr_len_ == NULL || *wstr_len_ == 0) {
		wstr_len = (int)wcslen(wstr_ptr) + 1;
	} else {
		wstr_len = *wstr_len_;
	}

	while(mb_len > 0 && wstr_len > 0) {
		size_t mb_out;
		uint32_t u32;
		size_t wb_in = UTF16ToUTF32(wstr_ptr, wstr_len, &u32);
		if (wb_in == 0) {
			wstr_len -= 1;
			wstr_in += 1;
			wstr_ptr++;
			goto unknown_code;
		}
		wstr_len -= wb_in;
		wstr_in += wb_in;
		wstr_ptr += wb_in;
		mb_out = UTF32ToMB(u32, mb_ptr, mb_len);
		if (mb_out == 0) {
		unknown_code:
			if (mb_ptr != NULL) {
				// 変換できなかった場合
				*mb_ptr = '?';
			}
			mb_out = 1;
		}
		mb_out_sum += mb_out;
		if (mb_ptr != NULL) {
			mb_ptr += mb_out;
			mb_len -= mb_out;
		}
	}

	if (wstr_len_ != NULL) {
		*wstr_len_ = wstr_in;
	}
	*mb_len_ = mb_out_sum;
}

// WideCharToMultiByteのUTF8特化版
void WideCharToUTF8(const wchar_t *wstr_ptr, size_t *wstr_len, char *u8_ptr, size_t *u8_len)
{
	WideCharToMB(wstr_ptr, wstr_len, u8_ptr, u8_len, UTF32ToUTF8);
}

void WideCharToCP932(const wchar_t *wstr_ptr, size_t *wstr_len, char *cp932_ptr, size_t *cp932_len)
{
	WideCharToMB(wstr_ptr, wstr_len, cp932_ptr, cp932_len, UTF32ToCP932);
}

void WideCharToMBCP(const wchar_t *wstr_ptr, size_t *wstr_len, char *mb_ptr, size_t *mb_len,
					int code_page)
{
	size_t (*utf32_to_mb)(uint32_t u32, char *mb_ptr, size_t mb_len);
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	switch (code_page) {
	case CP_UTF8:
		utf32_to_mb = UTF32ToUTF8;
		break;
	case 932:
		utf32_to_mb = UTF32ToCP932;
		break;
	default:
		*mb_len = 0;
		return;
	}

	WideCharToMB(wstr_ptr, wstr_len,
				 mb_ptr, mb_len,
				 utf32_to_mb);
}

// MultiByteToWideCharのUTF8特化版
int UTF8ToWideChar(const char *u8_ptr, int u8_len_, wchar_t *wstr_ptr, int wstr_len_)
{
	size_t u8_len;
	size_t wstr_len = wstr_len_;
	size_t u16_out_sum = 0;
	if (u8_len_ < 0) {
		u8_len = strlen(u8_ptr) + 1;
	} else {
		u8_len = u8_len_;
	}
	if (wstr_ptr == NULL) {
		wstr_len = 1;
	}

	while(wstr_len > 0 && u8_len > 0) {
		uint32_t u32;
		size_t u16_out;
		size_t u8_in;
		if (*u8_ptr == 0) {
			u32 = 0;
			u8_in = 1;
		} else {
			u8_in = UTF8ToUTF32(u8_ptr, u8_len, &u32);
			if (u8_in == 0) {
				u32 = '?';
				u8_in = 1;
			}
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
				if (wstr_ptr != NULL) {
					*wstr_ptr++ = '?';
				}
				u16_out = 1;
			}
		} else {
			if (wstr_ptr != NULL) {
				*wstr_ptr++ = '?';
			}
			u16_out = 1;
		}

		if (wstr_ptr != NULL) {
			wstr_len -= u16_out;
		}
		u16_out_sum += u16_out;
	}
	return (int)u16_out_sum;
}

/**
 *	wchar_t文字列をマルチバイト文字列へ変換
 *	変換できない文字は '?' で出力する
 *
 *	@param[in]	*wstr_ptr	wchar_t文字列
 *	@param[in]	wstr_len	wchar_t文字列長(0のとき自動、自動のときはL'\0'でターミネートすること)
 *	@param[in]	code_page	変換先コードページ
 *	@param[out]	*mb_len_	変換した文字列長,byte数,L'\0'を変換したら'\0'も含む
 *							(NULLのとき文字列長を返さない)
 *	@retval		mb文字列へのポインタ(NULLの時変換エラー)
 *				使用後 free() すること
 */
char *_WideCharToMultiByte(const wchar_t *wstr_ptr, size_t wstr_len, int code_page, size_t *mb_len_)
{
	const DWORD flags = 0;
	char *mb_ptr;
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	if (mb_len_ != NULL) {
		*mb_len_ = 0;
	}
	if (wstr_len == 0) {
		wstr_len = wcslen(wstr_ptr) + 1;
	}
    size_t len;
	if (code_page == CP_UTF8 || code_page == 932) {
		size_t wl = wstr_len;
		size_t ml;
		WideCharToMBCP(wstr_ptr, &wl, NULL, &ml, code_page);
		len = ml;
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
	if (code_page == CP_UTF8 || code_page == 932) {
		size_t wl = wstr_len;
		size_t ml = len;
		WideCharToMBCP(wstr_ptr, &wl, mb_ptr, &ml, code_page);
		len = ml;
	} else {
		len = ::WideCharToMultiByte(code_page, flags,
									wstr_ptr, (DWORD)wstr_len,
									mb_ptr, (int)len,
									NULL,NULL);
	}
	if (len == 0) {
		free(mb_ptr);
		return NULL;
	}
	if (mb_len_ != NULL) {
		// 変換した文字列数(byte数)を返す
		*mb_len_ = len;
	}
    return mb_ptr;
}

/**
 *	マルチバイト文字列をwchar_t文字列へ変換
 *	@param[in]	*str_ptr	mb(char)文字列
 *	@param[in]	str_len		mb(char)文字列長(0のとき自動、自動のときは'\0'でターミネートすること)
 *	@param[in]	code_page	変換元コードページ
 *	@param[out]	*w_len_		wchar_t文字列長,wchar_t数,'\0'を変換したらL'\0'も含む
 *							(NULLのとき文字列長を返さない)
 *	@retval		wchar_t文字列へのポインタ(NULLの時変換エラー)
 *				使用後 free() すること
 */
wchar_t *_MultiByteToWideChar(const char *str_ptr, size_t str_len, int code_page, size_t *w_len_)
{
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
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
		// 変換した文字列数(wchar_t数)を返す
		*w_len_ = len;
	}
	return wstr_ptr;
}

char *ToCharW(const wchar_t *strW)
{
	char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
}

char *ToCharA(const char *strA)
{
	return _strdup(strA);
}

char *ToCharU8(const char *strU8)
{
	wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	if (strW == NULL) {
		return NULL;
	}
	char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	free(strW);
	return strA;
}

wchar_t *ToWcharA(const char *strA)
{
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	return strW;
}

wchar_t *ToWcharW(const wchar_t *strW)
{
	return _wcsdup(strW);
}

wchar_t *ToWcharU8(const char *strU8)
{
	wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	return strW;
}

char *ToU8W(const wchar_t *strW)
{
	char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	return strU8;
}

char *ToU8A(const char *strA)
{
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	if (strW == NULL) {
		return NULL;
	}
	char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	free(strW);
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
	u8str_ = NULL;
	copy(obj);
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
u8::u8(u8 &&obj) noexcept
{
	u8str_ = NULL;
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
	tstr_ = NULL;
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
	wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	tc _tc = strW;
	free(strW);
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
