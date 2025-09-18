/*
 * $Id: common.h,v 1.5 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_COMMON_H_
#define _YCL_COMMON_H_

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <malloc.h>
#include <stdlib.h>
#include <crtdbg.h>

#ifndef countof
#define countof(a) (sizeof (a) / sizeof (a)[0])
#endif

#ifdef __cplusplus
namespace yebisuya {
#endif

HINSTANCE GetInstanceHandle();

#ifdef __cplusplus
}
#endif

#ifdef _DEBUG
//{

inline void OutputDebugStringF(const char* format, ...) {
	char buffer[1025];
	va_list arglist;
	va_start(arglist, format);
	wvsprintf(buffer, format, arglist);
	va_end(arglist);
	OutputDebugString(buffer);
}

#define YCLTRACE(s)                      OutputDebugString(s)
#define YCLTRACE1(s, a1)                 OutputDebugStringF(s, a1)
#define YCLTRACE2(s, a1, a2)             OutputDebugStringF(s, a1, a2)
#define YCLTRACE3(s, a1, a2, a3)         OutputDebugStringF(s, a1, a2, a3)
#define YCLTRACE4(s, a1, a2, a3, a4)     OutputDebugStringF(s, a1, a2, a3, a4)
#define YCLTRACE5(s, a1, a2, a3, a4, a5) OutputDebugStringF(s, a1, a2, a3, a4, a5)

inline bool YclAssert(bool condition, const char* message) {
	if (!condition) {
		char buffer[1025];
		wsprintf(buffer, "Assertion Failed!!\n\n"
						 "  %s\n\n"
						 "if ABORT button pushed then exit this program,\n"
						 "if RETRY button pushed then enter debug mode,\n"
						 "and if IGNORE button pushed then continue program.", message);
		switch (MessageBox(NULL, buffer, "YEBISUYA Class Library", MB_ABORTRETRYIGNORE | MB_ICONWARNING)) {
		case IDABORT:
			ExitProcess(-1);
			break;
		case IDRETRY:
			return true;
			break;
		case IDIGNORE:
			break;
		}
	}
	return false;
}

#define YCLASSERT(condition, message) if (YclAssert(condition, message)) {__debugbreak();}
#define YCLVERIFY(condition, message) if (YclAssert(condition, message)) {__debugbreak();}

#define malloc(l)     _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, l) _realloc_dbg((p), (l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define calloc(c, s)  _calloc_dbg((c), (s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)

//}
#else
//{

#define YCLTRACE(s)
#define YCLTRACE1(s, a1)
#define YCLTRACE2(s, a1, a2)
#define YCLTRACE3(s, a1, a2, a3)
#define YCLTRACE4(s, a1, a2, a3, a4)
#define YCLTRACE5(s, a1, a2, a3, a4, a5)

#define YCLASSERT(condition, message) 
#define YCLVERIFY(condition, message) (condition)

//}
#endif

// テンプレート使用時に警告が出るので抑制する
#pragma warning(disable: 4786)

#endif//_YCL_COMMON_H_
