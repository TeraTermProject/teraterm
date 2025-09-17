/*
 * (C) 2020- TeraTerm Project
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

#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>
#include <shlobj.h>	// for SHGetSpecialFolderPathW()
#include <intrin.h> // for __debugbreak()

#include "compat_win.h"
#include "asprintf.h"
#include "svnversion.h"	// for SVNVERSION

#include "ttdebug.h"
#include "ttlib.h"

/**
 *	コンソールウィンドウを表示するデバグ用
 *		デバグ用に埋め込んだprintf()系を表示することができるようになる
 *
 *	@retval	コンソールのWindow Handle
 */
HWND DebugConsoleOpen(void)
{
	FILE *fp;
	HWND hWnd = pGetConsoleWindow();
	if (hWnd != NULL) {
		return hWnd;
	}
	AllocConsole();
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	// 閉じるボタンを無効化
	hWnd = pGetConsoleWindow();
	HMENU hmenu = GetSystemMenu(hWnd, FALSE);
	RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);

	return hWnd;
}

//
// 例外ハンドラのフック（スタックトレースのダンプ）
//
// cf. http://svn.collab.net/repos/svn/trunk/subversion/libsvn_subr/win32_crashrpt.c
// (2007.9.30 yutaka)
//
// 例外コードを文字列へ変換する
#if defined(_M_IX86)	// WIN32
static const char *GetExceptionString(DWORD exception)
{
#define EXCEPTION(x) case EXCEPTION_##x: return (#x);
	static char buf[16];

	switch (exception)
	{
		EXCEPTION(ACCESS_VIOLATION)
		EXCEPTION(DATATYPE_MISALIGNMENT)
		EXCEPTION(BREAKPOINT)
		EXCEPTION(SINGLE_STEP)
		EXCEPTION(ARRAY_BOUNDS_EXCEEDED)
		EXCEPTION(FLT_DENORMAL_OPERAND)
		EXCEPTION(FLT_DIVIDE_BY_ZERO)
		EXCEPTION(FLT_INEXACT_RESULT)
		EXCEPTION(FLT_INVALID_OPERATION)
		EXCEPTION(FLT_OVERFLOW)
		EXCEPTION(FLT_STACK_CHECK)
		EXCEPTION(FLT_UNDERFLOW)
		EXCEPTION(INT_DIVIDE_BY_ZERO)
		EXCEPTION(INT_OVERFLOW)
		EXCEPTION(PRIV_INSTRUCTION)
		EXCEPTION(IN_PAGE_ERROR)
		EXCEPTION(ILLEGAL_INSTRUCTION)
		EXCEPTION(NONCONTINUABLE_EXCEPTION)
		EXCEPTION(STACK_OVERFLOW)
		EXCEPTION(INVALID_DISPOSITION)
		EXCEPTION(GUARD_PAGE)
		EXCEPTION(INVALID_HANDLE)

	default:
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "0x%x", exception);
		return buf;
		//return "UNKNOWN_ERROR";
	}
#undef EXCEPTION
}

/* 例外発生時に関数の呼び出し履歴を表示する、例外フィルタ関数 */
static void CALLBACK ApplicationFaultHandler(EXCEPTION_POINTERS *ExInfo)
{
	HGLOBAL gptr;
	STACKFRAME sf;
	BOOL bResult;
	PIMAGEHLP_SYMBOL pSym;
	DWORD Disp;
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	IMAGEHLP_MODULE	ih_module;
	IMAGEHLP_LINE	ih_line;
	int frame;
	char msg[3072], buf[256];

	if (pSymGetLineFromAddr == NULL) {
		goto error;
	}

	/* シンボル情報格納用バッファの初期化 */
	gptr = GlobalAlloc(GMEM_FIXED, 10000);
	if (gptr == NULL) {
		goto error;
	}
	pSym = (PIMAGEHLP_SYMBOL)GlobalLock(gptr);
	ZeroMemory(pSym, sizeof(IMAGEHLP_SYMBOL));
	pSym->SizeOfStruct = 10000;
	pSym->MaxNameLength = 10000 - sizeof(IMAGEHLP_SYMBOL);

	/* スタックフレームの初期化 */
	ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = ExInfo->ContextRecord->Eip;
	sf.AddrStack.Offset = ExInfo->ContextRecord->Esp;
	sf.AddrFrame.Offset = ExInfo->ContextRecord->Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	/* シンボルハンドラの初期化 */
	SymInitialize(hProcess, NULL, TRUE);

	// レジスタダンプ
	msg[0] = '\0';
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X\r\n"
		   "ebp=%08X esp=%08X eip=%08X efl=%08X\r\n"
		   "cs=%04X ss=%04X ds=%04X es=%04X fs=%04X gs=%04X\r\n",
		   ExInfo->ContextRecord->Eax,
		   ExInfo->ContextRecord->Ebx,
		   ExInfo->ContextRecord->Ecx,
		   ExInfo->ContextRecord->Edx,
		   ExInfo->ContextRecord->Esi,
		   ExInfo->ContextRecord->Edi,
		   ExInfo->ContextRecord->Ebp,
		   ExInfo->ContextRecord->Esp,
		   ExInfo->ContextRecord->Eip,
		   ExInfo->ContextRecord->EFlags,
		   ExInfo->ContextRecord->SegCs,
		   ExInfo->ContextRecord->SegSs,
		   ExInfo->ContextRecord->SegDs,
		   ExInfo->ContextRecord->SegEs,
		   ExInfo->ContextRecord->SegFs,
		   ExInfo->ContextRecord->SegGs
	);
	strncat_s(msg, sizeof(msg), buf, _TRUNCATE);

	if (ExInfo->ExceptionRecord != NULL) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Exception: %s\r\n", GetExceptionString(ExInfo->ExceptionRecord->ExceptionCode));
		strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
	}

	/* スタックフレームを順に表示していく */
	frame = 0;
	for (;;) {
		/* 次のスタックフレームの取得 */
		bResult = StackWalk(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			hThread,
			&sf,
			NULL,
			NULL,
			SymFunctionTableAccess,
			SymGetModuleBase,
			NULL);

		/* 失敗ならば、ループを抜ける */
		if (!bResult || sf.AddrFrame.Offset == 0)
			break;

		frame++;

		/* プログラムカウンタ（仮想アドレス）から関数名とオフセットを取得 */
		bResult = SymGetSymFromAddr(hProcess, sf.AddrPC.Offset, &Disp, pSym);

		/* 取得結果を表示 */
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "#%d  0x%08x in ", frame, sf.AddrPC.Offset);
		strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		if (bResult) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s() + 0x%x ", pSym->Name, Disp);
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, " --- ");
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		// 実行ファイル名の取得
		ZeroMemory( &(ih_module), sizeof(ih_module) );
		ih_module.SizeOfStruct = sizeof(ih_module);
		bResult = SymGetModuleInfo( hProcess, sf.AddrPC.Offset, &(ih_module) );
		strncat_s(msg, sizeof(msg), "at ", _TRUNCATE);
		if (bResult) {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s ", ih_module.ImageName );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		} else {
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s ", "<Unknown Module>" );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		// ファイル名と行番号の取得
		ZeroMemory( &(ih_line), sizeof(ih_line) );
		ih_line.SizeOfStruct = sizeof(ih_line);
		bResult = pSymGetLineFromAddr( hProcess, sf.AddrPC.Offset, &Disp, &ih_line );
		if (bResult)
		{
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s:%lu", ih_line.FileName, ih_line.LineNumber );
			strncat_s(msg, sizeof(msg), buf, _TRUNCATE);
		}

		strncat_s(msg, sizeof(msg), "\n", _TRUNCATE);
	}

	/* 後処理 */
	SymCleanup(hProcess);
	GlobalUnlock(pSym);
	GlobalFree(pSym);

	// 例外処理中なので、APIを直接呼び出す
	::MessageBoxA(NULL, msg, "Tera Term: Application fault", MB_OK | MB_ICONEXCLAMATION);

error:
	;
}
#endif // defined(_M_IX86)

static wchar_t *CreateDumpFilename()
{
	SYSTEMTIME local_time;
	GetLocalTime(&local_time);

#if defined(GITVERSION)
	char *version;
	asprintf(&version, "%s", GITVERSION);
#elif defined(SVNVERSION)
	char *version;
	asprintf(&version, "r%d", SVNVERSION);
#else
	char *version = _strdup("unknown");
#endif
#if defined(_M_IX86)
	const char *platform = "x86";
#elif defined(_M_X64)
	const char *platform = "x64";
#elif defined(_M_ARM64)
	const char *platform = "arm64";
#else
	const char *platform = "unknown";
#endif
	wchar_t *dump_file;
	aswprintf(&dump_file, L"teraterm_%hs_%04u%02u%02u-%02u%02u%02u_%hs.dmp",
			  platform,
			  local_time.wYear, local_time.wMonth, local_time.wDay,
			  local_time.wHour, local_time.wMinute, local_time.wSecond,
			  version);
	free(version);
	return dump_file;
}

/**
 *  ミニダンプファイルを出力
 */
static bool DumpMiniDump(const wchar_t *filename, struct _EXCEPTION_POINTERS* pExceptionPointers)
{
	if (pMiniDumpWriteDump == NULL) {
		// MiniDumpWriteDump() がサポートされていない。XPより前
		return false;
	}

	HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION mdei;
	mdei.ThreadId           = GetCurrentThreadId();
	mdei.ExceptionPointers  = pExceptionPointers;
	mdei.ClientPointers     = TRUE;

	BOOL r = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
								(MINIDUMP_TYPE)(MiniDumpNormal|MiniDumpWithHandleData),
								&mdei, NULL, NULL);

	CloseHandle(file);

	if (!r) {
		DeleteFileW(filename);
		return false;
	}
	return true;
}

static BOOL DumpFile = TRUE;

static LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS* pExceptionPointers)
{
	bool dumped = false;
	if (DumpFile) {
		wchar_t *dumpfile = NULL;
		wchar_t *fname = CreateDumpFilename();
		wchar_t *logdir = GetLogDirW(NULL);
		aswprintf(&dumpfile, L"%s\\%s", logdir, fname);

		dumped = DumpMiniDump(dumpfile, pExceptionPointers);
		if (dumped) {
			const wchar_t *msg_base =
				L"minidump '%s'\r\n"
				L" refer to https://teratermproject.github.io/manual/5/ja/reference/develop.html";
			wchar_t *msg;
			aswprintf(&msg, msg_base, dumpfile);
			MessageBoxW(NULL, msg, L"Tera Term", MB_OK);
			free(msg);
		}
		free(fname);
		free(dumpfile);
	}

#if defined(_M_IX86)
	if (!dumped) {
		ApplicationFaultHandler(pExceptionPointers);
	}
#endif

//	return EXCEPTION_EXECUTE_HANDLER;  /* そのままプロセスを終了させる */
	return EXCEPTION_CONTINUE_SEARCH;  /* 引き続き［アプリケーションエラー］ポップアップメッセージボックスを呼び出す */
}

void DebugTestCrash(void)
{
	__debugbreak();
}

static void InvalidParameterHandler(const wchar_t* /*expression*/,
									const wchar_t* /*function*/,
									const wchar_t* /*file*/,
									unsigned int /*line*/,
									uintptr_t /*pReserved*/)
{
	DebugTestCrash();
}

/**
 *  例外ハンドラのフック
 */
void DebugSetException(void)
{
	SetUnhandledExceptionFilter(ExceptionFilter);

	// Cランタイム無効なパラメータエラーハンドラ
	_set_invalid_parameter_handler(InvalidParameterHandler);
}
