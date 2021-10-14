/*
 * (C) 2021- TeraTerm Project
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

#include <stdlib.h>		// for size_t
#include <windows.h>	// for BOOL

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ProtoLog {
	// public
	BOOL (*Open)(struct ProtoLog *pv, const char *file);
	BOOL (*OpenA)(struct ProtoLog *pv, const char *file);
	BOOL (*OpenW)(struct ProtoLog *pv, const wchar_t *file);
	BOOL (*OpenU8)(struct ProtoLog *pv, const char *fileU8);
	void (*Close)(struct ProtoLog *pv);
	/**
	 *	フォルダをセットする
	 *	@param[in]	フォルダ(NULLのときフォルダ未設定となる)
	 */
	void (*SetFolderW)(struct ProtoLog *pv, const wchar_t *folder);
	size_t (*WriteRaw)(struct ProtoLog *pv, const void *data, size_t len);
	size_t (*WriteStr)(struct ProtoLog *pv, const char *str);
	void (*DumpByte)(struct ProtoLog *pv, BYTE b);
	void (*DumpFlush)(struct ProtoLog *pv);
	void (*Destory)(struct ProtoLog *pv);
	// private
	void *private_data;
} TProtoLog;

TProtoLog *ProtoLogCreate(void);

#ifdef __cplusplus
}
#endif
