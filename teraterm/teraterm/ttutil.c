/*
 * (C) 2018 TeraTerm Project
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

/* misc. routines  */
#include <windows.h>
#include <stdio.h>
#include "ttutil.h"

BOOL GetProcAddressses(HMODULE hModule, const GetProcAddressList list[], int count)
{
	int i;
	for (i = 0; i < count; i++) {
		void **pfunc = list[i].func;
		const char *name = list[i].name;
		static const char *symbol_templates[] = {
			"%s",
#if defined(_MSC_VER)
			"_%s@%d"
#endif
#if defined(__MINGW32__)
			"%s@%d"
#endif
		};
		void *func = NULL;
		int j;
		for (j = 0; j < _countof(symbol_templates); j++) {
			char buf[64];
			sprintf_s(buf, sizeof(buf), symbol_templates[j], name, list[i].arg_bytes);
			func = GetProcAddress(hModule, buf);
			if (func != NULL) break;
		}
		if (func == NULL) {
			return FALSE;
		}
		*pfunc = func;
	}
	return TRUE;
}

void ClearProcAddressses(const GetProcAddressList list[], int count)
{
	int i;
	for (i = 0; i < count; i++) {
		void **pfunc = list[i].func;
		*pfunc = NULL;
	}
}
