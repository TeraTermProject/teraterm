/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2019 TeraTerm Project
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
/* Tera Term */
/* TERATERM.EXE, IME interface */

#ifdef __cplusplus
extern "C" {
#endif

/* proto types */
BOOL LoadIME(void);
void FreeIME(HWND hWnd);
BOOL CanUseIME(void);
void SetConversionWindow(HWND HWnd, int X, int Y);
void SetConversionLogFont(HWND HWnd, PLOGFONTA lf);
BOOL GetIMEOpenStatus(HWND hWnd);
void SetIMEOpenStatus(HWND hWnd, BOOL stat);
const wchar_t *GetConvStringW(HWND hWnd, LPARAM lParam, size_t *len);
//const char *GetConvStringA(HWND hWnd, LPARAM lParam, size_t *len);
void *CreateReconvStringStW(HWND hWnd,
							const wchar_t *str_ptr, size_t str_count,
							size_t cx, size_t *st_size_);
void *CreateReconvStringStA(HWND hWnd,
							const char *str_ptr, size_t str_count,
							size_t cx, size_t *st_size_);

#ifdef __cplusplus
}
#endif
