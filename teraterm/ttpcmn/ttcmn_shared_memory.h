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

#pragma once

#include "tttypes.h"	// for MAXNWIN

/* shared memory */
typedef struct {
	size_t size_tmap;		/* sizeof TMap */
	size_t size_tttset;		/* sizeof TTTSet */
	// Window list
	int NWin;
	HWND WinList[MAXNWIN];
	/* COM port use flag
	 *           bit 8  7  6  5  4  3  2  1
	 * char[0] : COM 8  7  6  5  4  3  2  1
	 * char[1] : COM16 15 14 13 12 11 10  9 ...
	 */
	unsigned char ComFlag[(MAXCOMPORT-1)/CHAR_BIT+1];
	/* Previous window rect (Tera Term 4.78 or later) */
	WINDOWPLACEMENT WinPrevRect[MAXNWIN];
	BOOL WinUndoFlag;
	int WinUndoStyle;
	// Duplicate Teraterm data
	HANDLE DuplicateDataHandle;
	DWORD DuplicateDataSizeLow;
} TMap;
typedef TMap *PMap;
