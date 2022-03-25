/*
 * Copyright (C) 2014- TeraTerm Project
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
#include <wchar.h>

#include "cygtool.h"

int __stdcall FindCygwinPath(const wchar_t *CygwinDirectory, wchar_t *Dir, size_t Dirlen)
{
	wchar_t file[MAX_PATH], *filename;
	char c;

	/* zero-length string from Inno Setup is NULL */
	if (CygwinDirectory == NULL) {
		goto search_path;
	}

	if (CygwinDirectory[0] != 0) {
		if (SearchPathW(CygwinDirectory, L"bin\\cygwin1", L".dll", _countof(file), file, &filename) > 0) {
			goto found_dll;
		}
	}

search_path:;
	if (SearchPathW(NULL, L"cygwin1", L".dll", _countof(file), file, &filename) > 0) {
		goto found_dll;
	}

	for (c = 'C' ; c <= 'Z' ; c++) {
		wchar_t tmp[MAX_PATH];
		swprintf(tmp, _countof(tmp), L"%c:\\cygwin\\bin;%c:\\cygwin64\\bin", c, c);
		if (SearchPathW(tmp, L"cygwin1", L".dll", _countof(file), file, &filename) > 0) {
			goto found_dll;
		}
	}

	return 0;

found_dll:;
	wmemset(Dir, '\0', Dirlen);
	if (Dirlen <= wcslen(file) - 16) {
		return 0;
	}
	wmemcpy(Dir, file, wcslen(file) - 16); // delete "\\bin\\cygwin1.dll"
	return 1;
}

int __stdcall PortableExecutableMachine(const wchar_t *file)
{
	FILE *fp;
	unsigned char buf[4];
	long e_lfanew;
	WORD Machine;

	if (_wfopen_s(&fp, file, L"rb") != 0) {
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}

	// IMAGE_DOS_HEADER
	if (fseek(fp, 0x3c, SEEK_SET) != 0) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	if (fread(buf, sizeof(char), 4, fp) < 4) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	e_lfanew = buf[0] + (buf[1] << 8) + (buf[1] << 16) + (buf[1] << 24);

	// IMAGE_NT_HEADERS32
	//   DWORD Signature;
	//   IMAGE_FILE_HEADER FileHeader;
	if (fseek(fp, e_lfanew + 4, SEEK_SET) != 0) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	if (fread(buf, sizeof(char), 2, fp) < 2) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	Machine = buf[0] + (buf[1] << 8);

	fclose(fp);

	return Machine;
}

int __stdcall CygwinVersion(const wchar_t *dll, int *major, int *minor)
{
	DWORD dwSize;
	DWORD dwHandle;
	LPVOID lpBuf;
	UINT uLen;
	VS_FIXEDFILEINFO *pFileInfo;

	dwSize = GetFileVersionInfoSizeW(dll, &dwHandle);
	if (dwSize == 0) {
		return 0;
	}

	lpBuf = malloc(dwSize);
	if (!GetFileVersionInfoW(dll, dwHandle, dwSize, lpBuf)) {
		free(lpBuf);
		return 0;
	}

	if (!VerQueryValueW(lpBuf, L"\\", (LPVOID*)&pFileInfo, &uLen)) {
		free(lpBuf);
		return 0;
	}

	*major = HIWORD(pFileInfo->dwFileVersionMS);
	*minor = LOWORD(pFileInfo->dwFileVersionMS);

	free(lpBuf);

	return 1;
}

BOOL WINAPI DllMain(HANDLE hInstance,
		    ULONG ul_reason_for_call,
		    LPVOID lpReserved)
{
  switch( ul_reason_for_call ) {
    case DLL_THREAD_ATTACH:
      /* do thread initialization */
      break;
    case DLL_THREAD_DETACH:
      /* do thread cleanup */
      break;
    case DLL_PROCESS_ATTACH:
      /* do process initialization */
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
