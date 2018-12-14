
#pragma once

#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

char *_WideCharToMultiByte(const wchar_t *wstr_ptr, size_t wstr_len, int code_page, size_t *mb_len_);
wchar_t *_MultiByteToWideChar(const char *str_ptr, size_t str_len, int code_page, size_t *w_len_);
const TCHAR *ToTcharA(const char *strA);
const TCHAR *ToTcharU8(const char *strU8);
const TCHAR *ToTcharW(const wchar_t *strW);
const char *ToCharW(const wchar_t *strW);
const char *ToCharA(const char *strA);
const wchar_t *ToWcharA(const char *strA);
const char *ToU8A(const char *strA);
const char *ToU8W(const wchar_t *strW);

#if defined(_UNICODE)
#define ToCharT(s)	ToCharW(s)
#define	ToU8T(s)	ToU8W(s)
#else
#define ToCharT(s)	ToCharA(s)
#define	ToU8T(s)	ToU8A(s)
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
TCHAR *ToTchar(const char *strA);
TCHAR *ToTchar(const wchar_t *strW);
#endif


#ifdef __cplusplus

#if defined(__GNUC__) || (defined(_MSC_VER) && (_MSC_VER > 1910))
#define	MOVE_CONSTRUCTOR_ENABLE
#endif

class u8
{
public:
	u8();
	u8(const char *astr);
	u8(const char *astr, int code_page);
	u8(const wchar_t *wstr);
	u8(const u8 &obj);
#if defined(MOVE_CONSTRUCTOR_ENABLE)
	u8(u8 &&obj) noexcept;
#endif
	~u8();
	u8& operator=(const char *astr);
	u8& operator=(const wchar_t *wstr);
	u8& operator=(const u8 &obj);
#if defined(MOVE_CONSTRUCTOR_ENABLE)
	u8& operator=(u8 &&obj) noexcept;
#endif
	operator const char *() const;
	const char *cstr() const;
private:
	char *u8str_;
	void assign(const char *astr, int code_page);
	void assign(const wchar_t *wstr);
	void copy(const u8 &obj);
	void move(u8 &obj);
};

class tc
{
public:
	tc();
	tc(const char *astr);
	tc(const char *astr, int code_page);
	tc(const wchar_t *wstr);
	tc(const wchar_t *wstr, int code_page);
	tc(const tc &obj);
#if defined(MOVE_CONSTRUCTOR_ENABLE)
	tc(tc &&obj) noexcept;
#endif
	~tc();
	tc& operator=(const char *astr);
	tc& operator=(const wchar_t *wstr);
	tc& operator=(const tc &obj);
#if defined(MOVE_CONSTRUCTOR_ENABLE)
	tc& operator=(tc &&obj) noexcept;
#endif
	const TCHAR *fromUtf8(const char *u8str);
	operator const TCHAR *() const;
	const TCHAR *cstr() const;
private:
	TCHAR *tstr_;
	void assign(const char *astr, int code_page);
	void assign(const wchar_t *astr, int code_page);
	void copy(const tc &obj);
	void move(tc &obj);
};
#endif


