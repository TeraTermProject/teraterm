/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2021- TeraTerm Project
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

#include <direct.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <crtdbg.h>
#include <wchar.h>
#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#endif

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "tt_res.h"
#include "codeconv.h"
#include "compat_win.h"
#include "asprintf.h"
#include "ttcommon.h"

#include "ttcmn_dup.h"

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char uint8_t;
#endif

/**
 *	データシリアライズ情報構造体
 */
typedef struct {
	size_t size;		// データのサイズ(0で終了)
	size_t offset;		// 先頭からのオフセット
	enum {
		COPY = 0,				// データをコピーする
		MALLOCED_WSTRING = 1,	// mallocした領域の
	} type;
} TSerializeInfo;

/**
 *	データをシリアライズする
 *
 *	@param		data	シリアライズするデータ
 *	@param		info	シリアライズ情報
 *	@param[out]	size_	シリアライズしたblobサイズ(byte)
 *	@return				シリアライズしたblobデータへのポインタ
 *						不要になったらfree()すること
 */
static uint8_t *SerializeData(const void *data, const TSerializeInfo *info, size_t *size_)
{
	const unsigned char *src = (unsigned char *)data;
	uint8_t *blob = NULL;
	size_t size = 0;
	while (info->size != 0) {
		switch (info->type) {
		default:
			assert(FALSE);
			// fall torught
		case TSerializeInfo::COPY: {
			const size_t l = info->size;
			unsigned char *d;
			const unsigned char *s = (unsigned char *)src  + info->offset;
			blob = (uint8_t *)realloc(blob, size + l);
			d = blob + size;
			memcpy(d, s, l);
			size += l;
			break;
		}
		case TSerializeInfo::MALLOCED_WSTRING: {
			wchar_t **s = (wchar_t **)((unsigned char *)src  + info->offset);
			const wchar_t *src_str = *s;
			size_t str_len = 0;
			if (src_str != NULL) {
				str_len = wcslen(src_str) + 1;
			}
			const size_t str_len_byte = str_len * sizeof(wchar_t);
			const size_t alloc_len = sizeof(size_t) + str_len_byte;
			blob = (uint8_t *)realloc(blob, size + alloc_len);
			uint8_t *d = blob + size;
			*(size_t *)d = str_len;
			d += sizeof(size_t);
			wmemcpy((wchar_t *)d, src_str, str_len);
			size += alloc_len;
			break;
		}
		}
		info++;
	}
	*size_ = size;
	return blob;
}

/**
 *	アンシリアライズする
 *
 *	@param		blob	シリアライズされたデータへのポインタ
 *	@param		size	シリアライズされたデータのサイズ
 *	@param		info	シリアライズ情報
 *	@param[out]	data	書き戻すデータ
 */
static void UnserializeData(const void *blob, size_t size, const TSerializeInfo *info,
							void *data)
{
	const unsigned char *src = (const unsigned char *)blob;
	const unsigned char *end = src + size;
	while (info->size != 0) {
		switch (info->type) {
		default:
			assert(FALSE);
			break;
		case TSerializeInfo::COPY: {
			unsigned char *d = (unsigned char *)data  + info->offset;
			const size_t l = info->size;
			memcpy(d, src, l);
			src += l;
			break;
		}
		case TSerializeInfo::MALLOCED_WSTRING: {
			wchar_t **d = (wchar_t **)((unsigned char *)data  + info->offset);
			assert(info->size == sizeof(wchar_t *));
			const size_t l = *(size_t *)src;
			src += sizeof(size_t);
			if (l == 0) {
				*d = NULL;
			}
			else {
				const size_t byte_len = l * sizeof(wchar_t);
				wchar_t *str = (wchar_t *)malloc(byte_len);
				wmemcpy(str, (wchar_t *)src, l);
				*d = str;
				src += byte_len;
			}
			break;
		}
		};
		if (src > end) {
			// データがおかしい?
			assert(FALSE);
			break;
		}
		info++;
	}
}

static const uint8_t signature[] = {
	'T', 'E', 'R', 'A', 'T' , 'E', 'R', 'M',
	TT_VERSION_MAJOR,
	TT_VERSION_MINOR,
	sizeof(void *),		// 4 or 8 = 32bit or 64bit
	0,	// reserver
	(uint8_t)((sizeof(TTTSet) >> (8 * 0)) & 0xff),
	(uint8_t)((sizeof(TTTSet) >> (8 * 1)) & 0xff),
	(uint8_t)((sizeof(TTTSet) >> (8 * 2)) & 0xff),
	(uint8_t)((sizeof(TTTSet) >> (8 * 3)) & 0xff),
};

/**
 *	@return		0以外	スキップするサイズ
 *				0		error(異なった)
 */
static uint8_t *MakeSignature(size_t *size)
{
	uint8_t *p = (uint8_t *)malloc(sizeof(signature));
	memcpy(p, signature, sizeof(signature));
	*size = sizeof(signature);
	return p;
}

/**
 *	@return		0以外	スキップするサイズ
 *				0		error(異なった)
 */
static size_t CheckSignature(const uint8_t *ptr, size_t size)
{
	if (size < sizeof(signature)) {
		return 0;
	}
	if (memcmp(ptr, signature, sizeof(signature)) != 0) {
		return 0;
	}
	return sizeof(signature);
}

#if !defined(offsetof)
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#define MALLOCED_WSTRING_INFO(st, member) \
	sizeof(((st *)0)->member),			  \
	offsetof(st, member),			  \
	TSerializeInfo::MALLOCED_WSTRING

/**
 *	TTTsetを1つのバイナリにするための情報
 *	  方針
 *		- 全体をコピー
 *		- 動的確保された文字列を追加
 */
static const TSerializeInfo serialize_info[] = {
	{ sizeof(TTTSet), 0, TSerializeInfo::COPY},	// 全体をコピー
	{ MALLOCED_WSTRING_INFO(TTTSet, HomeDirW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, SetupFNameW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, KeyCnfFNW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, LogFNW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, MacroFNW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, UILanguageFileW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, UILanguageFileW_ini) },
	{ MALLOCED_WSTRING_INFO(TTTSet, ExeDirW) },
	{ MALLOCED_WSTRING_INFO(TTTSet, LogDirW) },
	{ 0, 0, TSerializeInfo::COPY },
};

/**
 *	TTTSet 構造体をバイナリデータに変換
 *
 *	@param		ts	TTTSet構造体へのポインタ
 *	@return		バイナリへのデータのサイズ
 *	@return		バイナリへのデータへポインタ
 */
void *TTCMNSerialize(const TTTSet *ts, size_t *size)
{
	size_t signature_size;
	uint8_t *signature_data = MakeSignature(&signature_size);
	size_t data_size;
	uint8_t *data = SerializeData(ts, serialize_info, &data_size);

	uint8_t *dest = (uint8_t *)malloc(signature_size + data_size);
	memcpy(dest, signature_data, signature_size);
	memcpy(dest + signature_size, data, data_size);
	free(data);
	free(signature_data);
	*size = signature_size + data_size;
	return dest;
}

/**
 *	バイナリデータをTTTSet 構造体に変換、
 *
 *	@param[in]	ptr		バイナリデータへのポインタ
 *	@param[in]	size	バイナリデータのサイズ
 *	@param[out]	ts		TTTSet構造体へのポインタ
 */
void TTCMNUnserialize(const void *ptr, size_t size, TTTSet *ts)
{
	const uint8_t *data = (uint8_t *)ptr;
	size_t signature_size = CheckSignature(data, size);
	if (signature_size != 0) {
		data += signature_size;
		size -= signature_size;
		UnserializeData(data, size, serialize_info, ts);
	}
}
