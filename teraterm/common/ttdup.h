/*
 * (C) 2025- TeraTerm Project
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

typedef enum {
	TTDUP_TELNET,
	TTDUP_SSH_CHALLENGE,
	TTDUP_SSH_PAGEANT,
	TTDUP_SSH_PASSWORD,
	TTDUP_SSH_PUBLICKEY,
	TTDUP_COMMANDLINE,
} TTDupMode;

typedef struct {
	TTDupMode mode;

	const wchar_t *szHostName;		// ホスト名
	int port;						// ポート番号
	const wchar_t *szUsername;		// ユーザ名
	const wchar_t *szPasswordW;		// パスワード(NULL時はユーザーがパスワードを入力)

	const wchar_t *szOption;		// 追加オプション/引数

	const wchar_t *szLog;			// ログファイル名

	// for telnet
	const wchar_t *szLoginPrompt;	// ログインプロンプト
	const wchar_t *szPasswdPrompt;	// パスワードプロンプト

	// for ssh MODE_SSH_PUBLICKEY
	const wchar_t *PrivateKeyFile;   // 秘密鍵ファイル
}  TTDupInfo;

DWORD ConnectHost(HINSTANCE hInstance, HWND hWnd, const TTDupInfo *jobInfo);

#ifdef __cplusplus
}
#endif
