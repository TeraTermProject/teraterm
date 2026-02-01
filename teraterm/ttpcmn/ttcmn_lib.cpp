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

#include "tt_res.h"
#include "dlglib.h"
#include "asprintf.h"
#include "compat_win.h"
#include "win32helper.h"
#include "codeconv.h"

#include "ttcmn_lib.h"

/**
 *	VT Window のアイコンとをセットする
 *
 *	@param[in]	cv			設定する Tera Term の cv
 *	@param[in]	hInstance	アイコンを保持しているモジュールのインスタンス
 *	@param[in]	IconID		アイコンのID
 *
 *	hInstance = NULL, IconID = 0 とすると、
 *  コマンドラインで指定されたアイコン、
 *	指定がない時は標準のVTアイコンがセットされる
 *
 *	通知領域のアイコンをセットするときは NotifySetIconID() を使用する
 */
void WINAPI SetVTIconID(TComVar *cv, HINSTANCE hInstance, WORD IconID)
{
	HINSTANCE icon_inst;
	WORD icon_id;
	TTTSet *ts = cv->ts;

	ts->PluginVTIconInstance = hInstance;
	ts->PluginVTIconID = IconID;

	icon_inst = (ts->PluginVTIconInstance == NULL) ? ts->TeraTermInstance : ts->PluginVTIconInstance;
	icon_id = (ts->PluginVTIconID != 0) ? ts->PluginVTIconID :
	                                      (ts->VTIcon != IdIconDefault) ? ts->VTIcon
	                                                                    : IDI_VT;
	TTSetIcon(icon_inst, cv->HWin, MAKEINTRESOURCEW(icon_id), 0);
}

static wchar_t *GetCHMFile(const wchar_t *exe_dir, const wchar_t *UILanguageFile)
{
	wchar_t *chm;
	wchar_t *chm_fname;

	GetI18nStrWW("Tera Term", "HELPFILE", L"teraterm.chm", UILanguageFile, &chm_fname);
	if(!IsRelativePathW(chm_fname)) {
		return chm_fname;
	}
	aswprintf(&chm, L"%s\\%s", exe_dir, chm_fname);
	free(chm_fname);
	return chm;
}

/**
 *	ヘルプを開く
 *
 *	@param[in]	Command			HtmlHelp() API の第3引数
 *	@param[in]	Data			HtmlHelp() API の第4引数
 *	@param[in]	ExeDirW
 *	@param[in]	UILanguageFileW
 *
 */
void WINAPI OpenHelpW(UINT Command, DWORD Data, const wchar_t *ExeDirW, wchar_t *UILanguageFileW)
{
	HWND HWin;
	wchar_t *chm;

	chm = GetCHMFile(ExeDirW, UILanguageFileW);

	HWin = GetDesktopWindow();
	if (_HtmlHelpW(HWin, chm, Command, Data) == NULL) {
		// ヘルプが開けなかった
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			NULL, L"Tera Term: HTML help",
			"MSG_OPENHELP_ERROR", L"Can't open HTML help file(%s).",
			MB_OK | MB_ICONERROR };
		TTMessageBoxW(HWin, &info, UILanguageFileW, chm);
	}
	free(chm);
}

/**
 *	ヘルプを開く
 *
 *	@param[in]	Command		HtmlHelp() API の第3引数
 *	@param[in]	Data		HtmlHelp() API の第4引数
 *
 *	次のコードは
 *		HWND HVTWin = GetParent(hDlgWnd);
 *		PostMessage(HVTWin, WM_USER_DLGHELP2, help_id, 0);
 *	次の関数呼び出しと同等
 *		OpenHelpCV(&cv, HH_HELP_CONTEXT, help_id);
 *
 */
void WINAPI OpenHelpCV(TComVar *cv, UINT Command, DWORD Data)
{
	TTTSet *ts = cv->ts;
	return OpenHelpW(Command, Data, ts->ExeDirW, ts->UILanguageFileW);
}

/**
 *	ヘルプを開く
 *
 *	互換維持のため存在
 *	OpenHelpCV() へ切り替えをおすすめ
 */
void WINAPI OpenHelp(UINT Command, DWORD Data, char *UILanguageFileA)
{
	wchar_t *Temp;
	wchar_t *HomeDirW;
	wchar_t *UILanguageFileW;

	hGetModuleFileNameW(NULL, &Temp);
	HomeDirW = ExtractDirNameW(Temp);
	UILanguageFileW = ToWcharA(UILanguageFileA);
	OpenHelpW(Command, Data, HomeDirW, UILanguageFileW);
	free(UILanguageFileW);
	free(HomeDirW);
	free(Temp);
}
