
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

