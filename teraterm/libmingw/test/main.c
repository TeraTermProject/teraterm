//gcc -g -o test main.c -Wall -Wl,-wrap=_imp__fopen_s -Wl,-Map=test.map
//gcc -g -o test main.c -Wall -Wl,-Map=test.map,--allow-multiple-definition

//#define __USE_MINGW_ANSI_STDIO 0
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <windows.h>
#include <locale.h>

#include "../../common/ttlib.h"
#include "../../common/asprintf.h"

int vaswprintf_test(const wchar_t *fmt, ...)
{
	wchar_t *message;
	va_list ap;
	va_start(ap, fmt);
	vaswprintf(&message, fmt, ap);
	wprintf(L"message='%ls'\n", message);
	wprintf(L"message='%s'\n", message);
	free(message);
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *fp;
	printf("1\n");
	fopen_s(&fp, "test.txt", "w");
	fclose(fp);
	printf("2\n");

	vaswprintf_test(L"test %s %ls %hs", L"wide", L"wide", "narrow");

	{
	wchar_t buf[128];
	swprintf(buf, _countof(buf), L"test %s %ls %hs", L"wide", L"wide", "narrow");
	wprintf(L"buf='%s'\n", buf);
	__ms_wprintf(L"buf='%s'\n", buf);
	}

	return 0;
}
