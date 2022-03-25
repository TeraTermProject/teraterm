/*
 * Copyright (C) 2022- TeraTerm Project
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

// cygtool.c ‚©‚ç•ª—£

#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#include "cygtool.h"

int main(void)
{
	wchar_t file[MAX_PATH];
	size_t file_len = _countof(file);
	int version_major, version_minor;
	int res;

	printf("FindCygwinPath()\n");
	res = FindCygwinPath(L"", file, file_len);
	printf("  result => %d\n", res);
	if (!res) {
		printf("\n");
		return -1;
	}
	printf("  Cygwin directory => %ls\n", file);
	printf("\n");

	printf("PortableExecutableMachine()\n");
	wcsncat_s(file, _countof(file), L"\\bin\\cygwin1.dll", _TRUNCATE);
	printf("  Cygwin DLL => %ls\n", file);
	res = PortableExecutableMachine(file);
	printf("  Machine => x%04x", res);
	switch (res) {
		case IMAGE_FILE_MACHINE_I386:
			printf(" = %s\n", "IMAGE_FILE_MACHINE_I386");
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			printf(" = %s\n", "IMAGE_FILE_MACHINE_AMD64");
			break;
		default:
			printf("\n");
			return -1;
			break;
	}
	printf("\n");

	printf("CygwinVersion()\n");
	printf("  Cygwin DLL => %ls\n", file);
	res = CygwinVersion(file, &version_major, &version_minor);
	printf("  result => %d\n", res);
	if (!res) {
	printf("\n");
		return -1;
	}
	printf("  version_major => %d\n", version_major);
	printf("  version_minor => %d\n", version_minor);
	printf("\n");

	return 0;
}
