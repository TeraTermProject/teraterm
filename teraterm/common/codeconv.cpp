
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

