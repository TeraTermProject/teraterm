
#include <stdio.h>
#include <windows.h>
#include <memory>

#include "codeconv.h"

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

const TCHAR *ToTcharA(const char *strA)
{
#if defined(UNICODE)
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	return strW;
#else
	return _strdup(strA);
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

const TCHAR *ToTcharW(const wchar_t *strW)
{
#if defined(UNICODE)
	return _wcsdup(strW);
#else
	char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
#endif
}

#if defined(UNICODE)
const char *ToCharW(const wchar_t *strW)
{
	const char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
}
#endif

const char *ToCharA(const char *strA)
{
	return _strdup(strA);
}

const char *ToU8W(const wchar_t *strW)
{
	const char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	return strU8;
}

const char *ToU8A(const char *strA)
{
	const wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	const char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	free((void *)strW);
	return strU8;
}

//////////////////////////////////////////////////////////////////////////////

u8::u8()
{
	u8str_ = NULL;
}

u8::u8(const char *astr)
{
	u8str_ = NULL;
	assign(astr, CP_ACP);
}

u8::u8(const char *astr, int cpage)
{
	u8str_ = NULL;
	assign(astr, cpage);
}

u8::u8(const wchar_t *wstr)
{
	u8str_ = NULL;
	assign(wstr);
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

u8& u8::operator=(const char *astr)
{
	assign(astr, CP_ACP);
	return *this;
}

u8& u8::operator=(const wchar_t *wstr)
{
	assign(wstr);
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

void u8::assign(const char *astr, int code_page)
{
	if (u8str_ != NULL) {
		free(u8str_);
	}
	wchar_t *wstr = _MultiByteToWideChar(astr, 0, code_page, NULL);
	if (wstr != NULL) {
		assign(wstr);
		free(wstr);
	} else {
		u8str_ = NULL;
	}
}

void u8::assign(const wchar_t *wstr)
{
	if (u8str_ != NULL) {
		free(u8str_);
	}
	char *u8str = _WideCharToMultiByte(wstr, 0, CP_UTF8, NULL);
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

tc::tc(const char *astr)
{
	tstr_ = NULL;
	assign(astr, CP_ACP);
}

tc::tc(const char *astr, int code_page)
{
	tstr_ = NULL;
	assign(astr, code_page);
}

tc::tc(const wchar_t *wstr)
{
	tstr_ = NULL;
	assign(wstr, CP_ACP);
}

tc::tc(const wchar_t *wstr, int code_page)
{
	tstr_ = NULL;
	assign(wstr, code_page);
}

#if defined(MOVE_CONSTRUCTOR_ENABLE)
tc::tc(tc &&obj) noexcept
{
	move(obj);
}
#endif

tc::~tc()
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
}

tc& tc::operator=(const char *astr)
{
	assign(astr, CP_ACP);
	return *this;
}

tc& tc::operator=(const wchar_t *wstr)
{
	assign(wstr, CP_ACP);
	return *this;
}

const TCHAR *tc::fromUtf8(const char *u8str)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
	tstr_ = NULL;
	wchar_t *wstr = _MultiByteToWideChar(u8str, 0, CP_UTF8, NULL);
	if (wstr != NULL) {
		assign(wstr, CP_ACP);
		free(wstr);
	}
	return cstr();
}

tc::operator const TCHAR *() const
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

void tc::assign(const char *astr, int code_page)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
#if !defined(UNICODE)
	(void)code_page;
	tstr_ = _strdup(astr);
#else
	wchar_t *wstr = _MultiByteToWideChar(astr, 0, code_page, NULL);
	if (wstr != NULL) {
		tstr_ = wstr;
	} else {
		tstr_ = NULL;
	}
#endif
}

void tc::assign(const wchar_t *wstr, int code_page)
{
	if (tstr_ != NULL) {
		free(tstr_);
	}
#if defined(UNICODE)
	(void)code_page;
	tstr_ = _wcsdup(wstr);
#else
	char *astr = _WideCharToMultiByte(wstr, 0, code_page, NULL);
	if (astr != NULL) {
		tstr_ = astr;
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
