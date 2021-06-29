
#include <stdio.h>
#include <windows.h>

#include "ttlib.h"
#include "compat_win.h"

void OutputDebugPrintf(const char *fmt, ...)
{
	char tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringA(tmp);
}

void OutputDebugPrintfW(const wchar_t *fmt, ...)
{
	wchar_t tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnwprintf_s(tmp, _countof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringW(tmp);
}
