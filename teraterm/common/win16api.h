/*
 * Copyright (C) 2018- TeraTerm Project
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

#ifdef __cplusplus
extern "C" {
#endif

HANDLE win16_lcreatW(const wchar_t *FileName, int iAttribute);
HANDLE win16_lcreat(const char *FileName, int iAttribute);
HANDLE win16_lopenW(const wchar_t *FileName, int iReadWrite);
HANDLE win16_lopen(const char *FileName, int iReadWrite);
void win16_lclose(HANDLE hFile);
UINT win16_lread(HANDLE hFile, void *buf, UINT uBytes);
UINT win16_lwrite(HANDLE hFile, const void *buf, UINT length);
LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin);

#ifdef __cplusplus
}
#endif

#define _lcreat(p1,p2)		win16_lcreat(p1,p2)
#define	_lopen(p1,p2)		win16_lopen(p1,p2)
#define _lclose(p1)			win16_lclose(p1)
#define _lread(p1,p2,p3)	win16_lread(p1,p2,p3)
#define _lwrite(p1,p2,p3)	win16_lwrite(p1,p2,p3)
#define	_llseek(p1,p2,p3)	win16_llseek(p1,p2,p3)
