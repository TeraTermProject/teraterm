/*
 * Copyright (C) 2021- TeraTerm Project
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

#include "codeconv.h"

#define ALWAYS_ANSI	1

static bool IsWindowsNTKernel(void)
{
#if ALWAYS_ANSI
	return false;
#elif defined(_MSC_VER) && _MSC_VER > 1400
	// VS2005よりあたらしい場合は、NTのみをターゲットとする
	return true;
#else
	static bool kernel_checked;
	static bool nt_kernel;

	if (kernel_checked) {
		return nt_kernel;
	}
	kernel_checked = true;

	// VS2005 or MinGW
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExA(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		// Windows 9x
		nt_kernel = false;
	}
	else {
		nt_kernel = true;
	}
	return nt_kernel;
#endif
}

static HMODULE _GetModuleHandleW(LPCWSTR lpModuleName)
{
	char* lpStringA = ToCharW(lpModuleName);
	HMODULE h = GetModuleHandleA(lpStringA);
	free(lpStringA);
	return h;
}

static DWORD DLLGetApiAddress(const wchar_t *dllPath,
							  const char *ApiName, void **pFunc)
{
	HMODULE hDll = _GetModuleHandleW(dllPath);
	if (hDll == NULL) {
		*pFunc = NULL;
		return ERROR_FILE_NOT_FOUND;
	} else {
		*pFunc = (void *)GetProcAddress(hDll, ApiName);
		if (*pFunc == NULL) {
			return ERROR_PROC_NOT_FOUND;
		}
		return NO_ERROR; // = 0
	}
}

extern "C" void init_ptr(const wchar_t *dll, const char *func_name, void *wrap_func, void **func_ptr)
{
	if (!IsWindowsNTKernel()) {
		*func_ptr = wrap_func;
		return;
	}

	void *ptr;
	DWORD r = DLLGetApiAddress(dll, func_name, &ptr);
	if (r == NO_ERROR) {
		*func_ptr = ptr;
	}
}
