/*
 * Copyright (C) 2018 TeraTerm Project
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

#include <stdafx.h>
#include <windows.h>
#include <string.h>
#include <crtdbg.h>

#include "codeconv.h"

#ifdef _DEBUG
#define malloc(l)     _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)
#endif

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
    int len = ::WideCharToMultiByte(code_page, flags,
									wstr_ptr, (DWORD)wstr_len,
									NULL, 0,
									NULL, NULL);
	if (len == 0) {
		return NULL;
	}
	mb_ptr = (char *)malloc(len);
	if (mb_ptr == NULL) {
		return NULL;
	}
	len = ::WideCharToMultiByte(code_page, flags,
								wstr_ptr, (DWORD)wstr_len,
								mb_ptr, len,
								NULL,NULL);
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
 *	@param[in]	code_page	変換先コードページ
 *	@param[out]	*w_len_		wchar_t文字列長
 *	@retval		mb文字列へのポインタ(NULLの時変換エラー)
 */
wchar_t *_MultiByteToWideChar(const char *str_ptr, size_t str_len, int code_page, size_t *w_len_)
{
	const DWORD flags = MB_ERR_INVALID_CHARS;
	wchar_t *wstr_ptr;
	if (w_len_ != NULL) {
		*w_len_ = 0;
	}
	if (str_len == 0) {
		str_len = strlen(str_ptr) + 1;
	}
	int len = ::MultiByteToWideChar(code_page, flags,
									str_ptr, (int)str_len,
									NULL, 0);
	if (len == 0) {
		return NULL;
	}
	wstr_ptr = (wchar_t *)malloc(len*sizeof(wchar_t));
	if (wstr_ptr == NULL) {
		return NULL;
	}
	len = ::MultiByteToWideChar(code_page, flags,
								str_ptr, (int)str_len,
								wstr_ptr, len);
	if (len == 0) {
		free(wstr_ptr);
		return NULL;
	}
	if (w_len_ != NULL) {
		*w_len_ = len - 1;
	}
	return wstr_ptr;
}

//#if defined(UNICODE)
const char *ToCharW(const wchar_t *strW)
{
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
}
//#endif

const char *ToCharA(const char *strA)
{
	return _strdup(strA);
}

const char *ToCharU8(const char *strU8)
{
	const wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	free((void *)strW);
	return strA;
}

const TCHAR *ToTcharA(const char *strA)
{
#if defined(UNICODE)
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	return strW;
#else
	return ToCharA(strA);
#endif
}

const TCHAR *ToTcharW(const wchar_t *strW)
{
#if defined(UNICODE)
	return _wcsdup(strW);
#else
	return ToCharW(strW);
#endif
}

const TCHAR *ToTcharU8(const char *strU8)
{
	const wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
#if defined(UNICODE)
	return strW;
#else
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	free((void *)strW);
	return strA;
#endif
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
