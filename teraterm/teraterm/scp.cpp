/*
 * (C) 2022- TeraTerm Project
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

/*
 *	ttxssh.dll へのインターフェイス
 *	- ttssh2/ttxssh/ttxssh.def 参照
 *
 *	TODO
 *	- unicode(wchar_t) filename
 *	- init()/uninit() per ssh connect/disconnect
 */

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>

#include "codeconv.h"

#include "scp.h"

typedef int (CALLBACK *PSSH_start_scp)(char *, char *);
typedef int (CALLBACK * PSSH_scp_sending_status)(void);
typedef size_t (CALLBACK *PSSH_GetKnownHostsFileName)(wchar_t *, size_t);

static HMODULE h = NULL;
static PSSH_start_scp start_scp = NULL;
static PSSH_start_scp receive_file = NULL;
static PSSH_scp_sending_status scp_sending_status = NULL;
static PSSH_GetKnownHostsFileName GetKnownHostsFileName;

/**
 * @brief SCP関数のアドレスを取得
 * @retval TRUE ok
 * @retval FALSE dllがない/dllがscp送信に対応していない
 */
static BOOL ScpInit(void)
{
	if (h == NULL) {
		if ((h = GetModuleHandle("ttxssh.dll")) == NULL) {
			return FALSE;
		}
	}

	if (start_scp == NULL) {
		start_scp = (PSSH_start_scp)GetProcAddress(h, "TTXScpSendfile");
		if (start_scp == NULL) {
			return FALSE;
		}
	}
	if (scp_sending_status == NULL) {
		scp_sending_status = (PSSH_scp_sending_status)GetProcAddress(h, "TTXScpSendingStatus");
		if (scp_sending_status == NULL) {
			return FALSE;
		}
	}

	if (receive_file == NULL) {
		receive_file = (PSSH_start_scp)GetProcAddress(h, "TTXScpReceivefile");
		if (receive_file == NULL) {
			return FALSE;
		}
	}

	if (GetKnownHostsFileName == NULL) {
		GetKnownHostsFileName = (PSSH_GetKnownHostsFileName)GetProcAddress(h, "TTXReadKnownHostsFile");
		if (GetKnownHostsFileName == NULL) {
			return FALSE;
		}
	}

	return TRUE;
}

/**
 *	ファイルを送信する
 *	@param	local	ローカル(PC,Windows)上のファイル
 *					フォルダは指定できない
 *	@param	remote	リモート(sshサーバー)上のフォルダ (or ファイル名?)
 *					L""でホームディレクトリ
 *	@return TRUE	ok(リクエストできた)
 *	@return FALSE	ng
 */
BOOL ScpSend(const wchar_t *local, const wchar_t *remote)
{
	if (start_scp == NULL) {
		ScpInit();
	}
	if (start_scp == NULL) {
		return FALSE;
	}
	char *localU8 = ToU8W(local);
	char *remoteU8 = ToU8W(remote);
	BOOL r = (BOOL)start_scp(localU8, remoteU8);
	free(localU8);
	free(remoteU8);
	return r;
}

/**
 *	ファイル送信状態
 *	@retval	FALSE	送信していない
 *	@retval	TRUE	送信中
 */
BOOL ScpGetStatus(void)
{
	if (scp_sending_status == NULL) {
		ScpInit();
	}
	if (scp_sending_status == NULL) {
		return FALSE;
	}
	BOOL r = (BOOL)scp_sending_status();
	return r;
}

/**
 *	ファイルを受信する
 */
BOOL ScpReceive(const wchar_t *remotefile, const wchar_t *localfile)
{
	if (receive_file == NULL) {
		ScpInit();
	}
	if (receive_file == NULL) {
		return FALSE;
	}
	char *localU8 = ToU8W(localfile);
	char *remoteU8 = ToU8W(remotefile);
	BOOL r = (BOOL)receive_file(remoteU8, localU8);
	free(localU8);
	free(remoteU8);
	return r;
}

/**
 *	knownhostファイル名を取得
 *	不要になったらfree()すること
 */
BOOL TTXSSHGetKnownHostsFileName(wchar_t **filename)
{
	if (GetKnownHostsFileName == NULL) {
		ScpInit();
	}
	if (GetKnownHostsFileName == NULL) {
		*filename = NULL;
		return FALSE;
	}

	size_t size = GetKnownHostsFileName(NULL, 0);
	if (size == 0) {
		*filename = NULL;
		return FALSE;
	}
	wchar_t *f = (wchar_t *)malloc(sizeof(wchar_t) * size);
	if (f == NULL) {
		*filename = NULL;
		return FALSE;
	}
	GetKnownHostsFileName(f, size);

	*filename = f;
	return TRUE;
}
