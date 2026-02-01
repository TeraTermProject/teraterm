/*
 * (C) 2024- TeraTerm Project
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

/* TTTSet related funcs */

#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>

#include "win32helper.h"
#include "compat_win.h"
#include "tttypes.h"

#include "ttlib_types.h"

wchar_t *GetFileDir(const TTTSet *pts)
{
	wchar_t *dir = NULL;
	if (pts->FileDirW != NULL && pts->FileDirW[0] != 0) {
		// ファイル転送用フォルダが設定されている
		// 環境変数を展開
		hExpandEnvironmentStringsW(pts->FileDirW, &dir);
		if (DoesFolderExistW(dir)) {
			// フォルダが存在
			return dir;
		}
		free(dir);
	}

	// Windowsのデフォルトのダウンロードフォルダを返す
	_SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, NULL, &dir);
	return dir;
}

wchar_t *GetTermLogDir(const TTTSet *pts)
{
	if (pts->LogDefaultPathW != NULL && pts->LogDefaultPathW[0] != '\0') {
		// 端末ログフォルダが指定されている場合
		return _wcsdup(pts->LogDefaultPathW);
	}

	// ファイル転送用フォルダ
	if (pts->FileDirW != NULL && pts->FileDirW[0] != 0) {
		// ファイル転送用フォルダが設定されている
		// 環境変数を展開
		wchar_t *dir;
		hExpandEnvironmentStringsW(pts->FileDirW, &dir);
		if (DoesFolderExistW(dir)) {
			// フォルダが存在
			return dir;
		}
		free(dir);
	}

	// %LOCALAPPDATA%\teraterm5
	return GetLogDirW(NULL);
}
