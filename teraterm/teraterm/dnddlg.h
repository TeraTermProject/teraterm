/*
 * (C) 2005-2018 TeraTerm Project
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
#include "tttypes.h"	// for TTTSet

enum drop_type {
	DROP_TYPE_CANCEL,
	DROP_TYPE_SCP,
	DROP_TYPE_SEND_FILE,		// past contents of file
	DROP_TYPE_SEND_FILE_BINARY,
	DROP_TYPE_PASTE_FILENAME,
};

#define DROP_TYPE_PASTE_ESCAPE	0x01
#define	DROP_TYPE_PASTE_NEWLINE	0x02

enum drop_type ShowDropDialogBox(
	HINSTANCE hInstance, HWND hWndParent,
	const char *TargetFilename,
	enum drop_type DefaultDropType,
	int RemaingFileCount,
	bool EnableSCP,
	bool EnableSendFile,
	TTTSet *pts,
	unsigned char *DropTypePaste,
	bool *DoSameProcess,
	bool *DoSameProcessNextDrop,
	bool *DoNotShowDialog);
