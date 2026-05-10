/*
 * Copyright (C) S.Hayakawa NTT-IT 1998-2002
 * (C) 2002- TeraTerm Project
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
#define		STRICT

#include	<windows.h>
#include	<commctrl.h>
#include	<stdio.h>
#include	<string.h>
#define _CRTDBG_MAP_ALLOC
#include	<stdlib.h>
#include	<crtdbg.h>
#include	<assert.h>

#include	"winmisc.h"
#include	"ttpmenu.h"
#include	"ttpmenu-version.h"
#include	"registry.h"
#include	"resource.h"

#include	"ttlib.h"
#include	"codeconv.h"
#include	"win32helper.h"
#include	"dlglib.h"
#include	"asprintf.h"

#include	"ttdup.h"

// TTLファイルを保存する場合定義する(デバグ用)
//#define KEEP_TTL_FILE L"C:\\tmp\\start.ttl"

// デフォルトインストール先はカレントディレクトリ
#define DEFAULT_PATH L"."

// グローバル変数
HWND		g_hWnd;				// メインのハンドル
HWND		g_hWndMenu = NULL;	// 設定ダイアログのハンドル
HWND		g_hWndTip;			// 設定ダイアログ内ツールチップのハンドル
HICON		g_hIcon;			// アプリケーションアイコンのハンドル
HICON		g_hIconSmall;		// アプリケーションアイコン(16x16)のハンドル
HMENU		g_hMenu;			// メニュー（非表示）のハンドル
HMENU		g_hSubMenu;			// ポップアップメニューのハンドル
HMENU		g_hListMenu;		// 設定一覧ポップアップメニューのハンドル
HMENU		g_hConfigMenu;		// 表示設定ポップアップメニューのハンドル
HHOOK		g_hHook = NULL;		// ツールチップ関連フックのハンドル
HINSTANCE	g_hI;				// アプリケーションインスタンス
HICON		g_hIconLeft;
HICON		g_hIconRight;

JobInfo		g_JobInfo;			// カレントの設定情報構造体（設定ダイアログ）
MenuData	g_MenuData;			// TeraTerm Menuの表示設定等の構造体

wchar_t		*SetupFNameW;		// TERATERM.INI
wchar_t		*UILanguageFileW;

// パスワード暗号化用パスワード
//   CryptProtectMemory() で暗号化するため、サイズは ENCRYPT2_PWD_MAX_LEN(161バイト) 以上で、
//   CRYPTPROTECTMEMORY_BLOCK_SIZE(16バイト) の倍数でなければならない。(実際に使用するのは ENCRYPT2_PWD_MAX_LEN まで)
#define CRYPTPROTECTMEMORYLEN 176
static char g_szLockBox[CRYPTPROTECTMEMORYLEN];

/* not used
#if (defined(__MINGW32__) && (__MINGW64_VERSION_MAJOR < 13)) || (defined(_MSC_VER) && (_MSC_VER == 1400))
// MinGW or VS2005(VC8.0)
static wchar_t* _wcstok(wchar_t *strToken, const wchar_t *strDelimit)
{
	return wcstok(strToken, strDelimit);
}
#endif
*/

/**
 *	フルパス化する
 *	相対パスの時は ttpmenu.exe のあるフォルダからのフルパスとなる
 *
 *	@param[in]	filename フルパス化するファイル名
 *	@return		フルパス化されたファイル名
 *				不要になったら free()すること
 */
static wchar_t *GetFullPath(const wchar_t *filename)
{
#if 0
	wchar_t *exe_fullpath = hGetModuleFileNameW(NULL);
	wchar_t *exe_dir = ExtractDirNameW(module_fullpath);
	free(exe_fullpath);
#endif
	wchar_t *exe_dir = GetExeDirW(NULL);
	wchar_t *fullpath = GetFullPathW(exe_dir, filename);
	free(exe_dir);
	return fullpath;
}

/**
 *	ttermpro.exe の存在するフォルダを返す
 *
 *	@param[in,out]	path	文字末には"/"がない
 *	@param[in]		size
 */
static void GetTeraTermDir(wchar_t *path, size_t size)
{
#if 1
	// 相対パス
	//	実行ファイルが置いてあるフォルダ(GetExeDirW())を
	//	カレントディレクトリに設定してある
	wcsncpy_s(path, size, DEFAULT_PATH, _TRUNCATE);
#else
	// 絶対パス
	wchar_t *exe_dir = GetExeDirW(NULL);
	wcsncpy_s(path, size, exe_dir, _TRUNCATE);
	free(exe_dir);
#endif
}

/**
 *	ttermpro.exe のパスを返す
 *
 *	@param[in,out]	path
 *	@param[in]		size
 */
static void GetTeraTermPath(wchar_t *path, size_t size)
{
	wchar_t szTTermPath[MAX_PATH];

	GetTeraTermDir(szTTermPath, MAX_PATH);
	_snwprintf_s(path, size, _TRUNCATE, L"%s\\%s", szTTermPath, TERATERM);
}

/* ==========================================================================
	Function Name	: (BOOL) ExecStartup()
	Outline			: スタートアップ設定のジョブを実行する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ExecStartup(HWND hWnd)
{
	wchar_t	szEntryName[MAX_PATH];
	HKEY	hKey;
	DWORD	dwIndex = 0;
	DWORD	dwSize = MAX_PATH;

	if ((hKey = RegOpen(HKEY_CURRENT_USER, TTERM_KEY)) != INVALID_HANDLE_VALUE) {
		while (dwIndex < MAXJOBNUM && RegEnumEx(hKey, dwIndex, szEntryName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			ConnectHost(hWnd, 0, szEntryName);
			dwIndex++;
			dwSize = MAX_PATH;
		}
		RegClose(hKey);
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (void) ErrorMessage()
	Outline			: 指定メッセージ＋システムのエラーメッセージを表示する。
	Arguments		: HWND			hWnd		(In) 親ウインドウのハンドル
					: const wchar_t *msg,...	(In) 任意メッセージ文字列
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
static void ErrorMessage(HWND hWnd, DWORD dwErr, const wchar_t *msg,...)
{
	wchar_t	szBuffer[MAX_PATH * 2] = L"";

	va_list ap;
	va_start(ap, msg);
	_vsnwprintf_s(szBuffer, _countof(szBuffer), _TRUNCATE, msg, ap);
	va_end(ap);

	size_t used = wcslen(szBuffer);
	size_t remaining = _countof(szBuffer) - used;
	if (remaining > 1) {
		::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						 NULL,
						 dwErr,
						 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						 szBuffer + used,
						 (DWORD)remaining,
						 NULL);
	}

	MessageBoxW(hWnd, szBuffer, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
}

/* ==========================================================================
	Function Name	: (BOOL) SetMenuFont()
	Outline			: フォント指定ダイアログを表示し、指定されたフォントを
					: 設定する。
	Arguments		: HWND			hWnd		(In) 親ウインドウのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: ダイアログの多重起動抑止(static int open)はシングルスレッド用
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SetMenuFont(HWND hWnd)
{
	HWND		hFontWnd;
	DWORD		rgbColors;
	LOGFONTW	lfFont;
	CHOOSEFONTW	chooseFont;
	static int	open = 0;

	if (open == 1) {
		while ((hFontWnd = ::FindWindowW(L"#32770" /* ダイアログボックスのクラス */, L"Font")) != NULL) {
			if (hWnd == ::GetParent(hFontWnd)) {
				::SetForceForegroundWindow(hFontWnd);
				break;
			}
		}
		return TRUE;
	}
	open = 1;

	lfFont		= g_MenuData.lfFont;
	rgbColors	= g_MenuData.crMenuTxt;

	memset((void *) &chooseFont, 0, sizeof(CHOOSEFONT));
	chooseFont.lStructSize	= sizeof(CHOOSEFONT);
	chooseFont.hwndOwner	= hWnd;
	chooseFont.lpLogFont	= &lfFont;
	chooseFont.Flags		= CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
	chooseFont.rgbColors	= rgbColors;
	chooseFont.nFontType	= SCREEN_FONTTYPE;

	if (::ChooseFontW(&chooseFont) == TRUE) {
		if (g_MenuData.hFont != NULL)
			::DeleteObject((HGDIOBJ) g_MenuData.hFont);
		g_MenuData.crMenuTxt	= chooseFont.rgbColors;
		g_MenuData.lfFont		= lfFont;
		g_MenuData.hFont		= ::CreateFontIndirectW(&lfFont);
		RedrawMenu(hWnd);
	}

	open = 0;

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) ExtractAssociatedIconEx()
	Outline			: アプリケーションに関連付けられたアイコンを取得する。
					: 設定する。
	Arguments		: const wchar_t	*szPath		(In) アプリケーション名
					: HICON			*hLargeIcon	(Out) 大きいアイコンのハンドル
					: HICON			*hSmallIcon	(Out) 小さいアイコンのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
static BOOL ExtractAssociatedIconEx(const wchar_t *szPath, HICON *hLargeIcon, HICON *hSmallIcon)
{
	SHFILEINFOW	sfi;
	*hLargeIcon = NULL;
	*hSmallIcon = NULL;

	if (::SHGetFileInfoW(szPath, 0, &sfi, sizeof(sfi), SHGFI_LARGEICON | SHGFI_ICON) != 0 && sfi.hIcon) {
		*hLargeIcon = ::CopyIcon(sfi.hIcon);
		::DestroyIcon(sfi.hIcon);
	}

	if (::SHGetFileInfoW(szPath, 0, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_ICON) != 0 && sfi.hIcon) {
		*hSmallIcon = ::CopyIcon(sfi.hIcon);
		::DestroyIcon(sfi.hIcon);
	}

	return (*hLargeIcon != NULL && *hSmallIcon != NULL);
}

/* ==========================================================================
	Function Name	: (BOOL) GetApplicationFilename()
	Outline			: レジストリより指定された設定のアプロケーション名を取得
					: する。
	Arguments		: const wchar_t		*szName		(In) 設定名
					: wchar_t			*szPath		(Out) アプリケーション名
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
static BOOL GetApplicationFilename(const wchar_t *szName, wchar_t *szPath)
{
	wchar_t	szSubKey[MAX_PATH * 2];

	BOOL	bRet;
	BOOL	bTtssh = FALSE;
	HKEY	hKey;

	_snwprintf_s(szSubKey, _countof(szSubKey), _TRUNCATE, L"%s\\%s", TTERM_KEY, szName);
	if ((hKey = RegOpen(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	bRet = RegGetStr(hKey, KEY_TERATERM, szPath, MAX_PATH);
	if (bRet == FALSE || wcslen(szPath) == 0) {
		RegGetBOOL(hKey, KEY_TTSSH, bTtssh);
		GetTeraTermPath(szPath, MAX_PATH);
	}

	RegClose(hKey);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) AddTooltip()
	Outline			: 指定されたコントロールにツールチップを関連付ける
	Arguments		: HWND		hWnd		(In) ウインドウのハンドル
					: int		idControl	(In) ウィンドウ上のコントロールID
					: wchar_t	*tip		(In) 表示するツールチップ
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL AddTooltip(HWND hWnd, int idControl, const wchar_t *tip)
{
	TOOLINFOW	ti = {};

	ti.cbSize	= sizeof(ti);
	ti.uFlags	= TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd		= hWnd;
	ti.uId		= (UINT_PTR)::GetDlgItem(hWnd, idControl); 
	ti.hinst	= 0; 
	ti.lpszText	= (LPWSTR)tip;

	return (BOOL)::SendMessageW(g_hWndTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

/* ==========================================================================
	Function Name	: (BOOL) LoadConfig()
	Outline			: レジストリよりTeraTerm Menuの表示設定等を取得する
	Arguments		: なし
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL LoadConfig(void)
{
	HKEY	hKey;
	wchar_t	uimsg[MAX_UIMSG];

	if ((hKey = RegCreate(HKEY_CURRENT_USER, TTERM_KEY)) == INVALID_HANDLE_VALUE)
		return FALSE;
	
	if (RegGetDword(hKey, KEY_ICONMODE, g_MenuData.dwIconMode) == TRUE) {
		if (g_MenuData.dwIconMode == MODE_LARGEICON) {
			UTIL_get_lang_msgW("MENU_ICON", uimsg, _countof(uimsg), STR_ICONMODE, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_ICON, MF_CHECKED | MF_BYCOMMAND, ID_ICON, uimsg);
		}
	} else
		g_MenuData.dwIconMode = MODE_SMALLICON;
	
	if (RegGetBOOL(hKey, KEY_LEFTBUTTONPOPUP, g_MenuData.bLeftButtonPopup) == FALSE)
		g_MenuData.bLeftButtonPopup = TRUE;
	if (g_MenuData.bLeftButtonPopup == TRUE) {
		UTIL_get_lang_msgW("MENU_LEFTPOPUP", uimsg, _countof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFileW);
		::ModifyMenuW(g_hConfigMenu, ID_LEFTPOPUP, MF_CHECKED | MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
	}

	DWORD dword_tmp;
	BOOL r = RegGetDword(hKey, KEY_MENUTEXTCOLOR, dword_tmp);
	g_MenuData.crMenuTxt = dword_tmp;
	if (r == FALSE) {
		g_MenuData.crMenuTxt = ::GetSysColor(COLOR_MENUTEXT);
	}

	if (RegGetBOOL(hKey, KEY_HOTKEY, g_MenuData.bHotkey) == FALSE)
		g_MenuData.bHotkey	= FALSE;
	if (g_MenuData.bHotkey == TRUE) {
		UTIL_get_lang_msgW("MENU_HOTKEY", uimsg, _countof(uimsg), STR_HOTKEY, UILanguageFileW);
		::ModifyMenuW(g_hConfigMenu, ID_HOTKEY, MF_CHECKED | MF_BYCOMMAND, ID_HOTKEY, uimsg);
		::RegisterHotKey(g_hWnd, WM_MENUOPEN, MOD_CONTROL | MOD_ALT, 'M');
	}

	if (RegGetLONG(hKey, KEY_LF_HEIGHT, g_MenuData.lfFont.lfHeight) == TRUE) {
		RegGetLONG(hKey, KEY_LF_WIDTH, g_MenuData.lfFont.lfWidth);
		RegGetLONG(hKey, KEY_LF_ESCAPEMENT, g_MenuData.lfFont.lfEscapement);
		RegGetLONG(hKey, KEY_LF_ORIENTATION, g_MenuData.lfFont.lfOrientation);
		RegGetLONG(hKey, KEY_LF_WEIGHT, g_MenuData.lfFont.lfWeight);
		RegGetBYTE(hKey, KEY_LF_ITALIC, g_MenuData.lfFont.lfItalic);
		RegGetBYTE(hKey, KEY_LF_UNDERLINE, g_MenuData.lfFont.lfUnderline);
		RegGetBYTE(hKey, KEY_LF_STRIKEOUT, g_MenuData.lfFont.lfStrikeOut);
		RegGetBYTE(hKey, KEY_LF_CHARSET, g_MenuData.lfFont.lfCharSet);
		RegGetBYTE(hKey, KEY_LF_OUTPRECISION, g_MenuData.lfFont.lfOutPrecision);
		RegGetBYTE(hKey, KEY_LF_CLIPPRECISION, g_MenuData.lfFont.lfClipPrecision);
		RegGetBYTE(hKey, KEY_LF_QUALITY, g_MenuData.lfFont.lfQuality);
		RegGetBYTE(hKey, KEY_LF_PITCHANDFAMILY, g_MenuData.lfFont.lfPitchAndFamily);
		RegGetStr(hKey, KEY_LF_FACENAME, g_MenuData.lfFont.lfFaceName, LF_FACESIZE);
	} else
		::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &(g_MenuData.lfFont));

	g_szLockBox[0] = 0;

	RegClose(hKey);

	g_MenuData.crMenuBg		= ::GetSysColor(COLOR_MENU);
	g_MenuData.crSelMenuBg	= ::GetSysColor(COLOR_HIGHLIGHT);
	g_MenuData.crSelMenuTxt	= ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	g_MenuData.hFont		= ::CreateFontIndirectW(&(g_MenuData.lfFont));

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SaveConfig()
	Outline			: レジストリにTeraTerm Menuの表示設定等を保存する
	Arguments		: なし
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SaveConfig(void)
{
	HKEY	hKey;

	if ((hKey = RegOpen(HKEY_CURRENT_USER, TTERM_KEY)) == INVALID_HANDLE_VALUE)
		return FALSE;

	RegSetDword(hKey, KEY_ICONMODE, g_MenuData.dwIconMode);
	RegSetDword(hKey, KEY_LEFTBUTTONPOPUP, g_MenuData.bLeftButtonPopup);
	RegSetDword(hKey, KEY_HOTKEY, g_MenuData.bHotkey);
	RegSetDword(hKey, KEY_MENUTEXTCOLOR, g_MenuData.crMenuTxt);
	RegSetDword(hKey, KEY_LF_HEIGHT, g_MenuData.lfFont.lfHeight);
	RegSetDword(hKey, KEY_LF_WIDTH, g_MenuData.lfFont.lfWidth);
	RegSetDword(hKey, KEY_LF_ESCAPEMENT, g_MenuData.lfFont.lfEscapement);
	RegSetDword(hKey, KEY_LF_ORIENTATION, g_MenuData.lfFont.lfOrientation);
	RegSetDword(hKey, KEY_LF_WEIGHT, g_MenuData.lfFont.lfWeight);
	RegSetDword(hKey, KEY_LF_ITALIC, g_MenuData.lfFont.lfItalic);
	RegSetDword(hKey, KEY_LF_UNDERLINE, g_MenuData.lfFont.lfUnderline);
	RegSetDword(hKey, KEY_LF_STRIKEOUT, g_MenuData.lfFont.lfStrikeOut);
	RegSetDword(hKey, KEY_LF_CHARSET, g_MenuData.lfFont.lfCharSet);
	RegSetDword(hKey, KEY_LF_OUTPRECISION, g_MenuData.lfFont.lfOutPrecision);
	RegSetDword(hKey, KEY_LF_CLIPPRECISION, g_MenuData.lfFont.lfClipPrecision);
	RegSetDword(hKey, KEY_LF_QUALITY, g_MenuData.lfFont.lfQuality);
	RegSetDword(hKey, KEY_LF_PITCHANDFAMILY, g_MenuData.lfFont.lfPitchAndFamily);
	RegSetStr(hKey, KEY_LF_FACENAME, g_MenuData.lfFont.lfFaceName);

	RegClose(hKey);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (LRESULT CALLBACK) GetMsgProc()
	Outline			: フック プロシージャ（GetMsgProcのヘルプ参照）
	Arguments		: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG	*lpMsg;

	lpMsg	= (MSG *) lParam;
	if (nCode < 0 || !(::IsChild(g_hWndMenu, lpMsg->hwnd)))
		return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);

	switch (lpMsg->message) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (g_hWndTip != NULL) {
			MSG	msg;
			msg.lParam	= lpMsg->lParam;
			msg.wParam	= lpMsg->wParam;
			msg.message	= lpMsg->message;
			msg.hwnd	= lpMsg->hwnd;
			::SendMessage(g_hWndTip, TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) &msg);
		}
		break;
	default:
		break;
    }

    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

/* ==========================================================================
	Function Name	: (BOOL) CreateTooltip()
	Outline			: ツールチップを作成する
	Arguments		: なし
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL CreateTooltip(HWND /* hWnd unusedParam */)
{
	wchar_t uimsg[MAX_UIMSG];

	::InitCommonControls(); 

	g_hWndTip = ::CreateWindowExA(0,
								TOOLTIPS_CLASSA,
								(LPSTR) NULL,
								TTS_ALWAYSTIP | TTS_BALLOON,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								g_hWndMenu,
								(HMENU) NULL,
								g_hI,
								NULL);

	if (g_hWndTip == NULL)
		return FALSE;

	AddTooltip(g_hWndMenu, BUTTON_SET, L"Register");
	AddTooltip(g_hWndMenu, BUTTON_DELETE, L"Unregister");
	AddTooltip(g_hWndMenu, BUTTON_ETC, L"Configure");
	AddTooltip(g_hWndMenu, CHECK_TTSSH, L"use SSH");

	UTIL_get_lang_msgW("MSG_TOOLTIP_CHECK_LOCKBOX", uimsg, _countof(uimsg),
					   L"If LockBox is enabled, login passwords are stored in LockBox encrypted.",
					   UILanguageFileW);
	AddTooltip(g_hWndMenu, CHECK_LOCKBOX, uimsg);
	UTIL_get_lang_msgW("MSG_TOOLTIP_BUTTON_LOCKBOX", uimsg, _countof(uimsg),
					   L"Set a password for locking and unlocking LockBox.",
					   UILanguageFileW);
	AddTooltip(g_hWndMenu, BUTTON_LOCKBOX, uimsg);

	g_hHook = ::SetWindowsHookEx(WH_GETMESSAGE,
								GetMsgProc,
								(HINSTANCE) NULL,
								::GetCurrentThreadId()); 

	if (g_hHook == (HHOOK) NULL)
		return FALSE; 

	return TRUE; 
}

/* ==========================================================================
	Function Name	: (void) PopupMenu()
	Outline			: メインのポップアップメニューを表示する。
	Arguments		: HWND		hWnd		(In) 親ウインドウのハンドル
	Return Value	: なし
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
void PopupMenu(HWND hWnd)
{
	POINT Point;

	GetCursorPos(&Point);
	::SetForceForegroundWindow(hWnd);

	::TrackPopupMenu(g_hSubMenu,
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						Point.x,
						Point.y,
						0,
						hWnd,
						NULL);
}

/* ==========================================================================
	Function Name	: (void) PopupListMenu()
	Outline			: 設定一覧のポップアップメニューを表示する。
	Arguments		: HWND		hWnd		(In) 親ウインドウのハンドル
	Return Value	: なし
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
void PopupListMenu(HWND hWnd)
{
	POINT Point;

	GetCursorPos(&Point);
	::SetForceForegroundWindow(hWnd);

	::TrackPopupMenu(g_hListMenu,
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						Point.x,
						Point.y,
						0,
						hWnd,
						NULL);
}

/* ==========================================================================
	Function Name	: (BOOL) InitListBox()
	Outline			: 設定ダイアログ内の設定一覧リストボックスを初期化する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitListBox(HWND hWnd)
{
	wchar_t	szPath[MAX_PATH];
	DWORD	dwCnt = 0;
	DWORD	dwIndex = 0;
	NameList *pNameList = g_MenuData.pNameList;

	::SendDlgItemMessage(hWnd, LIST_HOST, LB_RESETCONTENT, 0, 0);

	while(pNameList) {
		if (GetApplicationFilename(pNameList->szName, szPath) == TRUE) {
			::SendDlgItemMessageW(hWnd, LIST_HOST, LB_ADDSTRING, 0, (LPARAM)pNameList->szName);
			::SendDlgItemMessage(hWnd, LIST_HOST, LB_SETITEMDATA, (WPARAM) dwCnt, (LPARAM) dwIndex);
			dwCnt++;
		}
		dwIndex++;
		pNameList = pNameList->pNext;
	}
	return TRUE;
}


void init_password_control(HWND dlg, int item)
{
	HWND passwordControl = GetDlgItem(dlg, item);

	SetWindowLongPtr(passwordControl, GWLP_USERDATA,
					 SetWindowLongPtr(passwordControl, GWLP_WNDPROC,
									  (LONG_PTR) password_wnd_proc));
}

/* ==========================================================================
	Function Name	: (BOOL) InitConfigDlg()
	Outline			: 設定ダイアログを初期化する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitConfigDlg(HWND hWnd)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_CONFIG_TITLE" },
		{ LBL_LIST, "DLG_CONFIG_ITEM" },
		{ GRP_CONFIG, "DLG_CONFIG_CONFIG" },
		{ LBL_ENTRY, "DLG_CONFIG_NAME" },
		{ GRP_LAUNCH, "DLG_CONFIG_PATTERN" },
		{ RADIO_LOGIN, "DLG_CONFIG_AUTO" },
		{ LBL_HOST, "DLG_CONFIG_HOST" },
		{ CHECK_USER, "DLG_CONFIG_USER" },
		{ CHECK_PASSWORD, "DLG_CONFIG_PASS" },
		{ CHECK_LOCKBOX, "DLG_CONFIG_LOCKBOX_CHECK" },
		{ BUTTON_LOCKBOX, "DLG_CONFIG_LOCKBOX_BUTTON" },
		{ RADIO_MACRO, "DLG_CONFIG_MACRO" },
		{ RADIO_DIRECT, "DLG_CONFIG_LAUNCH" },
		{ CHECK_STARTUP, "DLG_CONFIG_STARTUP" },
		{ CHECK_TTSSH, "DLG_CONFIG_SSH" },
		{ IDC_KEYFILE_LABEL, "DLG_CONFIG_KEYFILE" },
		{ IDC_CHALLENGE_CHECK, "DLG_CONFIG_CHALLENGE" },
		{ IDC_PAGEANT_CHECK, "DLG_CONFIG_PAGEANT" },
		{ BUTTON_ETC, "DLG_CONFIG_DETAIL" },
	};
	SetI18nDlgStrsW(hWnd, "TTMenu", text_info, _countof(text_info), UILanguageFileW);

	init_password_control(hWnd, EDIT_PASSWORD);

	memset(&g_JobInfo, 0, sizeof(JobInfo));

	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_SIZE, MF_BYCOMMAND);

	g_hIconLeft		= ::LoadIconA(g_hI, (LPCSTR)ICON_LEFT);
	g_hIconRight	= ::LoadIconA(g_hI, (LPCSTR)ICON_RIGHT);
	::SendDlgItemMessage(hWnd, BUTTON_SET, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM)(HANDLE) g_hIconLeft);
	::SendDlgItemMessage(hWnd, BUTTON_DELETE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM)(HANDLE) g_hIconRight);

	::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_LOGIN);
	EnableItem(hWnd, EDIT_MACRO, FALSE);
	EnableItem(hWnd, BUTTON_MACRO, FALSE);
	::CheckDlgButton(hWnd, CHECK_USER, BST_CHECKED);
	::CheckDlgButton(hWnd, CHECK_PASSWORD, BST_CHECKED);
	::CheckDlgButton(hWnd, CHECK_LOCKBOX, BST_UNCHECKED);
	::CheckDlgButton(hWnd, CHECK_INI_FILE, BST_CHECKED);

	::SendDlgItemMessage(hWnd, EDIT_ENTRY, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_HOST, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_USER, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_PASSWORD, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, IDC_KEYFILE_PATH, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_MACRO, EM_LIMITTEXT, MAX_PATH - 1, 0);

	InitListBox(hWnd);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) InitEtcDlg()
	Outline			: 詳細設定ダイアログを初期化する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitEtcDlg(HWND hWnd)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_ETC_TITLE" },
		{ LBL_TTMPATH, "DLG_ETC_APP" },
		{ LBL_OPTION, "DLG_ETC_OPTION" },
		{ GRP_INITFILE, "DLG_ETC_CONFIG" },
		{ GRP_AUTOLOGIN, "DLG_ETC_AUTO" },
		{ LBL_LOG, "DLG_ETC_LOGFILE" },
		{ GRP_PROMPT, "DLG_ETC_PROMPT" },
		{ LBL_PROMPT_USER, "DLG_ETC_USER" },
		{ LBL_PROMPT_PASS, "DLG_ETC_PASS" },
		{ BUTTON_DEFAULT, "BTN_DEFAULT" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	SetI18nDlgStrsW(hWnd, "TTMenu", text_info, _countof(text_info), UILanguageFileW);

	if (wcslen(g_JobInfo.szTeraTerm) == 0) {
		GetTeraTermPath(g_JobInfo.szTeraTerm, MAX_PATH);
	}
	if (g_JobInfo.bTtssh == TRUE && lwcsstri(g_JobInfo.szTeraTerm, TERATERM) == NULL) {
		GetTeraTermPath(g_JobInfo.szTeraTerm, MAX_PATH);
	}
	if (wcslen(g_JobInfo.szLoginPrompt) == 0) {
		wcscpy_s(g_JobInfo.szLoginPrompt, _countof(g_JobInfo.szLoginPrompt), LOGIN_PROMPT);
	}
	if (wcslen(g_JobInfo.szPasswdPrompt) == 0) {
		wcscpy_s(g_JobInfo.szPasswdPrompt, _countof(g_JobInfo.szPasswdPrompt), PASSWORD_PROMPT);
	}

	::SetDlgItemTextW(hWnd, EDIT_TTMPATH, g_JobInfo.szTeraTerm);
	::SetDlgItemTextW(hWnd, EDIT_INITFILE, g_JobInfo.szInitFile);
	::SetDlgItemTextW(hWnd, EDIT_OPTION, g_JobInfo.szOption);
	::SetDlgItemTextW(hWnd, EDIT_PROMPT_USER, g_JobInfo.szLoginPrompt);
	::SetDlgItemTextW(hWnd, EDIT_PROMPT_PASS, g_JobInfo.szPasswdPrompt);
	::SetDlgItemTextW(hWnd, EDIT_LOG, g_JobInfo.szLog);

	::SendDlgItemMessage(hWnd, EDIT_TTMPATH, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_INITFILE, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_OPTION, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_PROMPT_USER, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_PROMPT_PASS, EM_LIMITTEXT, MAX_PATH - 1, 0);
	::SendDlgItemMessage(hWnd, EDIT_LOG, EM_LIMITTEXT, MAX_PATH - 1, 0);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) InitVersionDlg()
	Outline			: 「バージョン情報」ダイアログを初期化する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitVersionDlg(HWND hWnd)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_ABOUT_TITLE" },
		{ IDC_APP_NAME, "DLG_ABOUT_APPNAME" },
		{ IDOK, "BTN_OK" },
	};

	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_SIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MINIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_RESTORE, MF_BYCOMMAND);

	SetI18nDlgStrsW(hWnd, "TTMenu", text_info, _countof(text_info), UILanguageFileW);

	char buf[128];
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "version %d.%d ", TTPMENU_VERSION_MAJOR, TTPMENU_VERSION_MINOR);

	{
		char *substr = GetVersionSubstr();
		strncat_s(buf, sizeof(buf), substr, _TRUNCATE);
		free(substr);
	}
	SetDlgItemTextA(hWnd, IDC_VERSION, buf);

	BOOL use_ini;
	wchar_t *inifile;
	RegGetStatus(&use_ini, &inifile);
	wchar_t *setting;
	if (!use_ini) {
		aswprintf(&setting, L"use registry");
	}
	else {
		aswprintf(&setting, L"use ini file\r\n%s", inifile);
	}
	free(inifile);

	SetDlgItemTextW(hWnd, IDC_SETTINGS, setting);
	free(setting);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SetDefaultEtcDlg()
	Outline			: 詳細設定ダイアログの各項目にデフォルト値を設定する。
	Arguments		: HWND		hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SetDefaultEtcDlg(HWND hWnd)
{
	wchar_t	szTTermPath[MAX_PATH];

	GetTeraTermPath(szTTermPath, MAX_PATH);

	::SetDlgItemTextW(hWnd, EDIT_TTMPATH, szTTermPath);
	::SetDlgItemTextA(hWnd, EDIT_INITFILE, "");
	// デフォルトオプションに /KT , /KR を追加 (2005.1.25 yutaka)
	::SetDlgItemTextA(hWnd, EDIT_OPTION, "/KT=UTF8 /KR=UTF8");
//	::SetDlgItemText(hWnd, EDIT_OPTION, "");
	::SetDlgItemTextW(hWnd, EDIT_PROMPT_USER, LOGIN_PROMPT);
	::SetDlgItemTextW(hWnd, EDIT_PROMPT_PASS, PASSWORD_PROMPT);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SetTaskTray()
	Outline			: タスクトレイにアイコンを登録／削除する。
	Arguments		: HWND		hWnd		(In) ウインドウのハンドル
					: DWORD		dwMessage	(In) Shell_NotifyIconの第一引数
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SetTaskTray(HWND hWnd, DWORD dwMessage)
{
	NOTIFYICONDATAA	nid;
	int i;
	BOOL ret;
	DWORD ecode;

	memset(&nid, 0, sizeof(nid));
	nid.cbSize				= sizeof(nid);
	nid.hWnd				= hWnd;
	nid.uID					= TTERM_ICON;
	nid.uFlags				= NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage	= WM_TMENU_NOTIFY;
	nid.hIcon				= g_hIconSmall;
	lstrcpyA(nid.szTip, "TeraTerm Menu");

	/* Shell_NotifyIcon関数は、シェルへの登録が4秒以内に完了しないとエラーと見なすため、
	 * リトライ処理を追加する。
	 *
	 * Microsoft公式情報によると、WindowsXP〜7までが当該仕様の模様。Windows8/8.1では、
	 * リトライは不要と思われるが、共通処置とする。
	 * cf. http://support.microsoft.com/kb/418138/ja
	 * (2014.6.21 yutaka)
	 */
	if (dwMessage == NIM_ADD) {
		for (i = 0 ; i < 10 ; i++) {
			ret = ::Shell_NotifyIconA(dwMessage, &nid);
			ecode = GetLastError();
			if (ret == FALSE && ecode == ERROR_TIMEOUT) {
				Sleep(1000);
				ret = ::Shell_NotifyIconA(NIM_MODIFY, &nid);
				if (ret == TRUE) {
					break;
				}
			} else {
				break;
			}
		}
		return ret;
	} else {
		::Shell_NotifyIconA(dwMessage, &nid);
	}

	return TRUE;
}

/**
 *	パスワード取得
 *	@retval	パスワード(wchar_t文字列) 不要になったら SecureZeroMemory() してから free() すること
 */
static wchar_t *GetPassword(JobInfo *jobInfo, HWND hWnd)
{
	wchar_t *szPasswordW;
	char szRawPassword[MAX_PATH];
	if (jobInfo->bPassword == FALSE) {
		return NULL;
	}

	if (jobInfo->bLockBox == TRUE) {
		BOOL usePassword = DecryptPassword(jobInfo->szPassword, szRawPassword, hWnd);
		if (usePassword == FALSE) {
			szPasswordW = NULL;
		} else {
			szPasswordW = ToWcharA(szRawPassword);
		}
	} else {
		CryptUnprotectMemory(jobInfo->szPassword, sizeof(jobInfo->szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		EncodePassword((const char *)jobInfo->szPassword, szRawPassword);
		CryptProtectMemory(jobInfo->szPassword, sizeof(jobInfo->szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		szPasswordW = ToWcharA(szRawPassword);
	}
	SecureZeroMemory(szRawPassword, sizeof(szRawPassword));
	return szPasswordW;
}

/* ==========================================================================
	Function Name	: (NameList *) getNthNameList()
	Outline			: 表示設定構造体 NameList の N 番目 のリストのアドレスを返す
	Arguments		: DWORD		N		(In) N 番目
	Return Value	: 成功 NameListのアドレス / 失敗 NULL
	Reference		:
	Renewal			:
	Notes			:
	Attention		:
	Up Date			:
   ======1=========2=========3=========4=========5=========6=========7======= */
NameList *getNthNameList(DWORD Nth)
{
	NameList *pNameList = g_MenuData.pNameList;
	for (DWORD i = 0; i < MAXJOBNUM && pNameList; i++) {
		if (i == Nth) {
			return pNameList;
		}
		pNameList = pNameList->pNext;
	}
	return NULL;
}

/* ==========================================================================
	Function Name	: (BOOL) ConnectHost()
	Outline			: 自動ログインまたはアプリケーションの実行をする。
	Arguments		: HWND		hWnd		(In) ウインドウのハンドル
					: UINT		idItem		(In) 選択されたコントロールID
					: char		*szJobName	(In) 実行するジョブ
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ConnectHost(HWND hWnd, UINT idItem, const wchar_t *szJobName)
{
	wchar_t	szName[MAX_PATH];
	wchar_t	szDirectory[MAX_PATH];
	wchar_t	*szTemp;
	JobInfo	jobInfo;

	DWORD	dwErr = NO_ERROR;
	wchar_t uimsg[MAX_UIMSG];

	if (szJobName) {
		wcscpy_s(szName, _countof(szName), szJobName);
	} else {
		NameList *pNameList = getNthNameList(idItem - ID_MENU_MIN);
		if (pNameList) {
			wcscpy_s(szName, _countof(szName), pNameList->szName);
		} else {
			return FALSE;
		}
	}

	if (RegLoadLoginHostInformation(szName, &jobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msgW("MSG_ERROR_MAKETTL", uimsg, _countof(uimsg),
						   L"Couldn't read the registry data.\n", UILanguageFileW);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	if (szJobName != NULL && jobInfo.bStartup == FALSE)
		return TRUE;

	if (wcslen(jobInfo.szTeraTerm) == 0) {
		GetTeraTermPath(jobInfo.szTeraTerm, MAX_PATH);
	}

	wchar_t	*szArgment = _wcsdup(L"");

	if (jobInfo.dwMode != MODE_DIRECT)
		if (wcslen(jobInfo.szInitFile) != 0) {
			aswprintf(&szTemp, L"/F=\"%s\"", jobInfo.szInitFile);
			awcscat(&szArgment, szTemp);
			free(szTemp);
		}

	if (wcslen(jobInfo.szOption) != 0) {
		aswprintf(&szTemp, L" %s", jobInfo.szOption);
		awcscat(&szArgment, szTemp);
		free(szTemp);
	}

	if (jobInfo.dwMode == MODE_AUTOLOGIN ) {
		wchar_t *passwordW = GetPassword(&jobInfo, hWnd);

		TTDupInfo info = {};
		info.szHostName = jobInfo.szHostName;
		info.szUsername = jobInfo.szUsername;
		info.szOption = szArgment;

		if (jobInfo.bTtssh != TRUE) {
			// TELNET
			info.mode = TTDUP_TELNET;
			info.szPasswordW = passwordW;
			info.szLoginPrompt = jobInfo.szLoginPrompt;
			info.szPasswdPrompt = jobInfo.szPasswdPrompt;
		} else if (jobInfo.bChallenge) {
			// SSH keyboard-interactive
			info.mode = TTDUP_SSH_CHALLENGE;
			info.szPasswordW = passwordW;
		} else if (jobInfo.bPageant) {
			// SSH Pageant
			info.mode = TTDUP_SSH_PAGEANT;
		} else if (jobInfo.PrivateKeyFile[0] == L'\0') {
			// SSH password authentication
			info.mode = TTDUP_SSH_PASSWORD;
			info.szPasswordW = passwordW;
		} else {
			// SSH publickey
			info.mode = TTDUP_SSH_PUBLICKEY;
			info.szPasswordW = passwordW;
			info.PrivateKeyFile = jobInfo.PrivateKeyFile;
		}
		dwErr = ConnectHost(g_hI, hWnd, &info);
		if (passwordW != NULL) {
			SecureZeroMemory(passwordW, sizeof(wchar_t) * (wcslen(passwordW) + 1));
			free(passwordW);
		}
	}
	else {
		switch (jobInfo.dwMode) {
		case MODE_MACRO:
			aswprintf(&szTemp, L" /M=\"%s\"", jobInfo.szMacroFile);
			awcscat(&szArgment, szTemp);
			free(szTemp);
			break;
		case MODE_DIRECT:
			break;
		default:
			assert(FALSE);
			break;
		}

		// フルパス化する
		wchar_t *exe_fullpath = GetFullPath(jobInfo.szTeraTerm);
		wcscpy_s(jobInfo.szTeraTerm, _countof(jobInfo.szTeraTerm), exe_fullpath);
		free(exe_fullpath);

		// 実行するプログラムのカレントパス
		//   プログラムのあるフォルダ
		wcscpy_s(szDirectory, _countof(szDirectory), jobInfo.szTeraTerm);
		if ((::GetFileAttributesW(jobInfo.szTeraTerm) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			wchar_t	*pt = wcsrchr(szDirectory, '\\');
			if (pt != NULL)
				*pt	= '\0';
		}

		SHELLEXECUTEINFOW ExecInfo = {};
		ExecInfo.cbSize			= sizeof(ExecInfo);
		ExecInfo.fMask			= SEE_MASK_FLAG_NO_UI;
		ExecInfo.hwnd			= hWnd;
		ExecInfo.lpVerb			= NULL;
		ExecInfo.lpFile			= jobInfo.szTeraTerm;
		ExecInfo.lpParameters	= szArgment;
		ExecInfo.lpDirectory	= szDirectory;
		ExecInfo.nShow			= SW_SHOWNORMAL;
		ExecInfo.hInstApp		= g_hI;

		if (::ShellExecuteExW(&ExecInfo) == FALSE) {
			dwErr = ::GetLastError();
		}
	}

	if (dwErr != NO_ERROR) {
		UTIL_get_lang_msgW("MSG_ERROR_LAUNCH", uimsg, _countof(uimsg),
						   L"Launching the application was failure.\n", UILanguageFileW);
		ErrorMessage(hWnd, dwErr, uimsg);
	}

	free(szArgment);
	szArgment = NULL;

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) InitMenu()
	Outline			: 各ポップアップメニューを作成する。
	Arguments		: なし
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitMenu(void)
{
	wchar_t	uimsg[MAX_UIMSG];

	if (g_hListMenu == NULL) {
		g_hMenu			= ::LoadMenuA(g_hI, (LPCSTR) TTERM_MENU);
		g_hSubMenu		= ::GetSubMenu(g_hMenu, 0);
		g_hListMenu		= ::CreateMenu();
		g_hConfigMenu	= ::GetSubMenu(g_hSubMenu, 1);
		if (g_hMenu == NULL || g_hSubMenu == NULL || g_hListMenu == NULL)
			return FALSE;

		static const DlgTextInfo SubMenuTextInfo[] = {
			{ ID_TMENU_ADD, "MENU_LISTCONFIG" },
			{ 1, "MENU_SETTING" },
			{ ID_VERSION, "MENU_VERSION" },
			{ ID_EXEC, "MENU_EXEC" },
			{ ID_TMENU_CLOSE, "MENU_CLOSE" },
		};
		SetI18nMenuStrsW(g_hSubMenu, "TTMenu", SubMenuTextInfo, _countof(SubMenuTextInfo), UILanguageFileW);

		static const DlgTextInfo ConfigMenuTextInfo[] = {
			{ ID_ICON, "MENU_ICON" },
			{ ID_FONT, "MENU_FONT" },
			{ ID_LEFTPOPUP, "MENU_LEFTPOPUP" },
			{ ID_HOTKEY, "MENU_HOTKEY" },
		};
		SetI18nMenuStrsW(g_hConfigMenu, "TTMenu", ConfigMenuTextInfo, _countof(ConfigMenuTextInfo), UILanguageFileW);
		UTIL_get_lang_msgW("MENU_EXEC", uimsg, _countof(uimsg), L"Execute", UILanguageFileW);
		::ModifyMenuW(g_hSubMenu, ID_EXEC, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)g_hListMenu, uimsg);
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (VOID) DeleteListMenuIcons()
	Outline			: SHGetFileInfo で取り出したアイコンリソースを開放する。
	                  ポップアップメニューと一覧表示で使用するため、
	                  開放できるのは一覧更新直前とプログラム終了時。
	Arguments		: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
VOID DeleteListMenuIcons()
{
	NameList *pNameList = g_MenuData.pNameList;
	g_MenuData.pNameList = NULL;

	while (pNameList) {
		if (pNameList->hLargeIcon != NULL) {
			::DestroyIcon(pNameList->hLargeIcon);
		}
		if (pNameList->hSmallIcon != NULL) {
			::DestroyIcon(pNameList->hSmallIcon);
		}
		NameList *pNext = pNameList->pNext;
		free(pNameList);
		pNameList = pNext;
	}
}

/* ==========================================================================
	Function Name	: (BOOL) InitListMenu()
	Outline			: 設定一覧ポップアップメニューを初期化する。
	Arguments		: HWND		hWnd		(In) ウインドウのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL InitListMenu(HWND hWnd)
{
	wchar_t	szPath[MAX_PATH];
	wchar_t	szEntryName[MAX_PATH];
	HKEY	hKey;
	DWORD	dwIndex = 0;
	DWORD	dwSize = MAX_PATH;

	DeleteListMenuIcons();

	NameList **pBefore = &g_MenuData.pNameList;
	if ((hKey = RegOpen(HKEY_CURRENT_USER, TTERM_KEY)) != INVALID_HANDLE_VALUE) {
		while (dwIndex < MAXJOBNUM && RegEnumEx(hKey, dwIndex, szEntryName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			size_t len = wcslen(szEntryName);
			NameList *pNameList = (NameList *)malloc(sizeof(NameList) + (len + 1) * sizeof(wchar_t));
			if (pNameList == NULL) {
				RegClose(hKey);
				RedrawMenu(hWnd);
				return FALSE;
			}
			wcscpy_s(pNameList->szName, len + 1, szEntryName);
			if (GetApplicationFilename(szEntryName, szPath) == TRUE) {
				ExtractAssociatedIconEx(szPath, &(pNameList->hLargeIcon), &(pNameList->hSmallIcon));
			} else {
				pNameList->hLargeIcon = NULL;
				pNameList->hSmallIcon = NULL;
			}
			pNameList->pNext = NULL;
			*pBefore = pNameList;
			pBefore = &(pNameList->pNext);
			dwIndex++;
			dwSize = MAX_PATH;
		}
		RegClose(hKey);
	}

	RedrawMenu(hWnd);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RedrawMenu()
	Outline			: 設定一覧ポップアップメニューを描画する。
	Arguments		: HWND		hWnd		(In) ウインドウのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RedrawMenu(HWND hWnd)
{
	int			num;
	wchar_t		szPath[MAX_PATH];
	HDC			hDC;
	DWORD		itemNum;
	DWORD		desktopHeight;
	DWORD		dwCnt = 0;
	DWORD		dwValidCnt = 0;
	TEXTMETRIC	textMetric;
	wchar_t		uimsg[MAX_UIMSG];

	::DeleteMenu(g_hListMenu, ID_NOENTRY, MF_BYCOMMAND);
	num = ::GetMenuItemCount(g_hListMenu);
	for (dwCnt = 0; dwCnt < (DWORD) num; dwCnt++)
		if (::DeleteMenu(g_hListMenu, ID_MENU_MIN + dwCnt, MF_BYCOMMAND) == FALSE)
			num++;

	hDC = ::GetDC(hWnd);
	if (hDC == NULL) {
		g_MenuData.dwMenuHeight = ICONSPACE_SMALL;
	} else {
		if (g_MenuData.hFont != NULL) {
			::SelectObject(hDC, (HGDIOBJ) g_MenuData.hFont);
		}
		::GetTextMetrics(hDC, &textMetric);
		if (g_MenuData.dwIconMode == MODE_SMALLICON) {
			g_MenuData.dwMenuHeight	= (ICONSPACE_SMALL > textMetric.tmHeight) ? ICONSPACE_SMALL : textMetric.tmHeight;
		} else {
			g_MenuData.dwMenuHeight	= (ICONSPACE_LARGE > textMetric.tmHeight) ? ICONSPACE_LARGE : textMetric.tmHeight;
		}
		if (g_MenuData.dwMenuHeight == 0) {
			g_MenuData.dwMenuHeight = ICONSPACE_SMALL;
		}
        ReleaseDC(hWnd, hDC);
	}

	desktopHeight	= ::GetSystemMetrics(SM_CYSCREEN);
	itemNum			= desktopHeight / g_MenuData.dwMenuHeight;

	dwCnt = 0;
	NameList *pNameList = g_MenuData.pNameList;
	while(pNameList) {
		if (GetApplicationFilename(pNameList->szName, szPath) == TRUE) {
			if (dwCnt % itemNum == 0 && dwCnt != 0)
				::AppendMenuW(g_hListMenu, MF_OWNERDRAW | MF_MENUBARBREAK, ID_MENU_MIN + dwCnt,
							  (LPCWSTR)(UINT_PTR)dwCnt);
			else
				::AppendMenuW(g_hListMenu, MF_OWNERDRAW | MF_POPUP, ID_MENU_MIN + dwCnt,
							  (LPCWSTR)(UINT_PTR)dwCnt);
			dwValidCnt++;
		}
		dwCnt++;
		pNameList = pNameList->pNext;
	}
	if (dwValidCnt == 0) {
		UTIL_get_lang_msgW("MENU_NOTRAY", uimsg, _countof(uimsg), STR_NOENTRY, UILanguageFileW);
		::AppendMenuW(g_hListMenu, MF_STRING | MF_GRAYED, ID_NOENTRY, uimsg);
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegSaveLoginHostInformation()
	Outline			: レジストリに設定情報を保存する。
	Arguments		: JobInfo		*jobInfo	(In) 設定情報構造体
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegSaveLoginHostInformation(JobInfo *jobInfo)
{
	HKEY	hKey;
	wchar_t	szSubKey[MAX_PATH * 2];

	_snwprintf_s(szSubKey, _countof(szSubKey), _TRUNCATE, L"%s\\%s", TTERM_KEY, jobInfo->szName);
	if ((hKey = RegCreate(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	RegSetStr(hKey, KEY_HOSTNAME, jobInfo->szHostName);
	RegSetDword(hKey, KEY_MODE, jobInfo->dwMode);

	RegSetDword(hKey, KEY_USERFLAG, (DWORD) jobInfo->bUsername);
	RegSetStr(hKey, KEY_USERNAME, jobInfo->szUsername);
	RegSetDword(hKey, KEY_PASSWDFLAG, (DWORD) jobInfo->bPassword);
	RegSetDword(hKey, KEY_LOCKBOXFLAG, (DWORD) jobInfo->bLockBox);

	if (jobInfo->bLockBox == FALSE) {
		CryptUnprotectMemory(jobInfo->szPassword, sizeof(jobInfo->szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		RegSetBinary(hKey, KEY_PASSWORD, jobInfo->szPassword, (DWORD)(strlen(jobInfo->szPassword)));
		CryptProtectMemory(jobInfo->szPassword, sizeof(jobInfo->szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
	} else {
		RegSetBinary(hKey, KEY_PASSWORD, jobInfo->szPassword, ENCRYPT2_PROFILE_LEN);
	}

	RegSetStr(hKey, KEY_TERATERM, jobInfo->szTeraTerm);
	RegSetStr(hKey, KEY_INITFILE, jobInfo->szInitFile);
	RegSetStr(hKey, KEY_OPTION, jobInfo->szOption);
	RegSetStr(hKey, KEY_LOGIN_PROMPT, jobInfo->szLoginPrompt);
	RegSetStr(hKey, KEY_PASSWORD_PROMPT, jobInfo->szPasswdPrompt);

	RegSetStr(hKey, KEY_MACROFILE, jobInfo->szMacroFile);

	RegSetDword(hKey, KEY_TTSSH, (DWORD) jobInfo->bTtssh);
	RegSetDword(hKey, KEY_STARTUP, (DWORD) jobInfo->bStartup);

	RegSetStr(hKey, KEY_LOG, jobInfo->szLog);

	// SSH2
	RegSetStr(hKey, KEY_KEYFILE, jobInfo->PrivateKeyFile);
	RegSetDword(hKey, KEY_CHALLENGE, (DWORD) jobInfo->bChallenge);
	RegSetDword(hKey, KEY_PAGEANT, (DWORD) jobInfo->bPageant);

	RegClose(hKey);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) RegLoadLoginHostInformation()
	Outline			: レジストリから設定情報を取得する。
	Arguments		: const wchar_t *szName		(In) 設定情報名
					: JobInfo		*jobInfo	(In) 設定情報構造体
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegLoadLoginHostInformation(const wchar_t *szName, JobInfo *job_Info)
{
	HKEY	hKey;
	wchar_t	szSubKey[MAX_PATH * 2];
	DWORD	dwSize;
	JobInfo jobInfo;
	DWORD dword_tmp;

	memset(&jobInfo, 0, sizeof(JobInfo));

	_snwprintf_s(szSubKey, _countof(szSubKey), _TRUNCATE, L"%s\\%s", TTERM_KEY, szName);
	if ((hKey = RegOpen(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	wcscpy_s(jobInfo.szName, _countof(jobInfo.szName), szName);

	RegGetStr(hKey, KEY_HOSTNAME, jobInfo.szHostName, MAX_PATH);
	RegGetDword(hKey, KEY_MODE, dword_tmp);
	jobInfo.dwMode = (JobMode)dword_tmp;

	RegGetBOOL(hKey, KEY_USERFLAG, jobInfo.bUsername);
	RegGetStr(hKey, KEY_USERNAME, jobInfo.szUsername, MAX_PATH);
	RegGetBOOL(hKey, KEY_PASSWDFLAG, jobInfo.bPassword);
	RegGetBOOL(hKey, KEY_LOCKBOXFLAG, jobInfo.bLockBox);

	SecureZeroMemory(jobInfo.szPassword, sizeof(jobInfo.szPassword));
	dwSize = MAX_PATH; // jobInfo.szPassword は 272バイトだが、使用可能なパスワード長は MAX_PATH(260バイト)
	if (! RegGetBinary(hKey, KEY_PASSWORD, jobInfo.szPassword, &dwSize)) {
		SecureZeroMemory(jobInfo.szPassword, sizeof(jobInfo.szPassword));
	}
	if (jobInfo.bLockBox == FALSE) {
		CryptProtectMemory(jobInfo.szPassword, sizeof(jobInfo.szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
	}

	RegGetStr(hKey, KEY_TERATERM, jobInfo.szTeraTerm, MAX_PATH);
	RegGetStr(hKey, KEY_INITFILE, jobInfo.szInitFile, MAX_PATH);
	RegGetStr(hKey, KEY_OPTION, jobInfo.szOption, MAX_PATH);
	RegGetStr(hKey, KEY_LOGIN_PROMPT, jobInfo.szLoginPrompt, MAX_PATH);
	RegGetStr(hKey, KEY_PASSWORD_PROMPT, jobInfo.szPasswdPrompt, MAX_PATH);

	RegGetStr(hKey, KEY_MACROFILE, jobInfo.szMacroFile, MAX_PATH);

	RegGetBOOL(hKey, KEY_TTSSH, jobInfo.bTtssh);
	RegGetBOOL(hKey, KEY_STARTUP, jobInfo.bStartup);

	RegGetStr(hKey, KEY_LOG, jobInfo.szLog, MAX_PATH);

	// SSH2
	ZeroMemory(jobInfo.PrivateKeyFile, sizeof(jobInfo.PrivateKeyFile));
	RegGetStr(hKey, KEY_KEYFILE, jobInfo.PrivateKeyFile, MAX_PATH);
	RegGetBOOL(hKey, KEY_CHALLENGE, jobInfo.bChallenge);
	RegGetBOOL(hKey, KEY_PAGEANT, jobInfo.bPageant);

	RegClose(hKey);

	*job_Info = jobInfo;

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SaveEtcInformation()
	Outline			: 詳細設定情報をグローバル変数に保存する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SaveEtcInformation(HWND hWnd)
{
	::GetDlgItemTextW(hWnd, EDIT_TTMPATH, g_JobInfo.szTeraTerm, MAX_PATH);
	::GetDlgItemTextW(hWnd, EDIT_INITFILE, g_JobInfo.szInitFile, MAX_PATH);
	::GetDlgItemTextW(hWnd, EDIT_OPTION, g_JobInfo.szOption, MAX_PATH);
	::GetDlgItemTextW(hWnd, EDIT_PROMPT_USER, g_JobInfo.szLoginPrompt, MAX_PATH);
	::GetDlgItemTextW(hWnd, EDIT_PROMPT_PASS, g_JobInfo.szPasswdPrompt, MAX_PATH);

	::GetDlgItemTextW(hWnd, EDIT_LOG, g_JobInfo.szLog, MAX_PATH);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SaveLoginHostInformation()
	Outline			: 設定情報を保存する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SaveLoginHostInformation(HWND hWnd)
{
	wchar_t	szName[MAX_PATH];
	DWORD	dwErr;
	wchar_t	uimsg[MAX_UIMSG];
	unsigned char	cEncodePassword[MAX_PATH];
	unsigned char	cEncodeLockBox[MAX_PATH];
	Encrypt2Profile	profile;

	if (::GetDlgItemTextW(hWnd, EDIT_ENTRY, g_JobInfo.szName, MAX_PATH) == 0) {
		UTIL_get_lang_msgW("MSG_ERROR_NOREGNAME", uimsg, _countof(uimsg),
						   L"error: no registry name", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}
	if (wcschr(g_JobInfo.szName, L'\\') != NULL) {
		UTIL_get_lang_msgW("MSG_ERROR_INVALIDREGNAME", uimsg, _countof(uimsg),
						   L"can't use \"\\\" in registry name", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	if (::IsDlgButtonChecked(hWnd, RADIO_LOGIN) == BST_CHECKED)
		g_JobInfo.dwMode = MODE_AUTOLOGIN;
	if (::IsDlgButtonChecked(hWnd, RADIO_MACRO) == BST_CHECKED)
		g_JobInfo.dwMode = MODE_MACRO;
	if (::IsDlgButtonChecked(hWnd, RADIO_DIRECT) == BST_CHECKED)
		g_JobInfo.dwMode = MODE_DIRECT;

	if (::GetDlgItemTextW(hWnd, EDIT_HOST, g_JobInfo.szHostName, MAX_PATH) == 0 && g_JobInfo.dwMode == MODE_AUTOLOGIN) {
		UTIL_get_lang_msgW("MSG_ERROR_NOHOST", uimsg, _countof(uimsg),
						   L"error: no host name", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	g_JobInfo.bUsername	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_USER);
	::GetDlgItemTextW(hWnd, EDIT_USER, g_JobInfo.szUsername, MAX_PATH);

	g_JobInfo.bPassword	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_PASSWORD);
	g_JobInfo.bLockBox = (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_LOCKBOX);
	::GetDlgItemTextA(hWnd, EDIT_PASSWORD, (LPSTR)cEncodePassword, MAX_PATH);
	if (g_JobInfo.bLockBox == FALSE) {
		EncodePassword((char *)cEncodePassword, g_JobInfo.szPassword);
		CryptProtectMemory(g_JobInfo.szPassword, sizeof(g_JobInfo.szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		SecureZeroMemory(cEncodePassword, sizeof(cEncodePassword));
	} else if (g_JobInfo.dwMode == MODE_AUTOLOGIN) {
		if (g_szLockBox[0] == 0) {
			SecureZeroMemory(cEncodePassword, sizeof(cEncodePassword));
			UTIL_get_lang_msgW("MSG_ERROR_NOLOCKBOX", uimsg, _countof(uimsg),
							   L"error: LockBox is not setup.", UILanguageFileW);
			::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
			return FALSE;
		}
		memcpy(cEncodeLockBox, g_szLockBox, sizeof(g_szLockBox));
		CryptUnprotectMemory(cEncodeLockBox, sizeof(g_szLockBox), CRYPTPROTECTMEMORY_SAME_PROCESS);
		Encrypt2EncDec((char *)cEncodePassword, cEncodeLockBox, &profile, 1);
		SecureZeroMemory(cEncodePassword, sizeof(cEncodePassword));
		SecureZeroMemory(cEncodeLockBox, sizeof(cEncodeLockBox));
		memcpy(g_JobInfo.szPassword, &profile, ENCRYPT2_PROFILE_LEN);
		SecureZeroMemory(&profile, sizeof(profile));
	} else {
		SecureZeroMemory(cEncodePassword, sizeof(cEncodePassword));
	}

	if (::GetDlgItemTextW(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile, MAX_PATH) == 0 && g_JobInfo.dwMode == MODE_MACRO) {
		UTIL_get_lang_msgW("MSG_ERROR_NOMACRO", uimsg, _countof(uimsg),
						   L"error: no macro filename", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	g_JobInfo.bStartup	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_STARTUP);

	g_JobInfo.bTtssh	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_TTSSH);
	if (g_JobInfo.bTtssh == TRUE && lwcsstri(g_JobInfo.szTeraTerm, TERATERM) == NULL) {
		GetTeraTermPath(g_JobInfo.szTeraTerm, MAX_PATH);
	} else if (wcslen(g_JobInfo.szTeraTerm) == 0) {
		GetTeraTermPath(g_JobInfo.szTeraTerm, MAX_PATH);
	}

	wchar_t *exe_fullpath = GetFullPath(g_JobInfo.szTeraTerm);
	DWORD e = GetFileAttributesW(exe_fullpath);
	free(exe_fullpath);
	if (e == INVALID_FILE_ATTRIBUTES) {
		dwErr = ::GetLastError();
		if (dwErr == ERROR_FILE_NOT_FOUND || dwErr == ERROR_PATH_NOT_FOUND) {
			UTIL_get_lang_msgW("MSG_ERROR_CHECKFILE", uimsg, _countof(uimsg),
							   L"checking [%s] file was failure.\n", UILanguageFileW);
			ErrorMessage(hWnd, dwErr, uimsg, g_JobInfo.szTeraTerm);
			return FALSE;
		}
	}

	if (::GetDlgItemTextW(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile, MAX_PATH) == 0) {
		ZeroMemory(g_JobInfo.PrivateKeyFile, sizeof(g_JobInfo.PrivateKeyFile));
	}
	if (g_JobInfo.bTtssh) {
		g_JobInfo.bChallenge = (BOOL) ::IsDlgButtonChecked(hWnd, IDC_CHALLENGE_CHECK);
		g_JobInfo.bPageant = (BOOL) ::IsDlgButtonChecked(hWnd, IDC_PAGEANT_CHECK);
	}

	if (RegSaveLoginHostInformation(&g_JobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msgW("MSG_ERROR_SAVEREG", uimsg, _countof(uimsg),
						   L"error: couldn't save to registry.\n", UILanguageFileW);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	InitListMenu(hWnd);
	InitListBox(hWnd);

	LRESULT index = 0;
	while ((index = ::SendDlgItemMessageW(hWnd, LIST_HOST, LB_SELECTSTRING, index, (LPARAM)g_JobInfo.szName)) != LB_ERR) {
		::SendDlgItemMessageW(hWnd, LIST_HOST, LB_GETTEXT, index, (LPARAM)szName);
		if (_wcsicmp(g_JobInfo.szName, szName) == 0)
			break;
	}

	return TRUE;
}

static void EnableWindows(HWND hWnd, const int *list, size_t count, BOOL enable)
{
	for (size_t i = 0; i < count; i++) {
		HWND w = GetDlgItem(hWnd, list[i]);
		EnableWindow(w, enable);
	}
}

static const int autologin_items[] = {
	EDIT_HOST,
	CHECK_USER,
	EDIT_USER,
	CHECK_PASSWORD,
	EDIT_PASSWORD,
	CHECK_LOCKBOX,
	BUTTON_LOCKBOX,
	CHECK_TTSSH,
	IDC_KEYFILE_PATH,
	IDC_KEYFILE_BUTTON,
	IDC_CHALLENGE_CHECK,
	IDC_PAGEANT_CHECK,
};

static const int macro_items[] = {
	EDIT_MACRO,
	BUTTON_MACRO,
};

/* ==========================================================================
	Function Name	: (BOOL) LoadLoginHostInformation()
	Outline			: 設定情報を取得する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL LoadLoginHostInformation(HWND hWnd)
{
	LRESULT	index;
	wchar_t	szName[MAX_PATH];
	wchar_t	uimsg[MAX_UIMSG];
	DWORD	dwErr;
	char 	szEncodePassword[MAX_PATH];

	index = ::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		return FALSE;
	}

	::SendDlgItemMessageW(hWnd, LIST_HOST, LB_GETTEXT, (WPARAM)index, (LPARAM)szName);

	if (RegLoadLoginHostInformation(szName, &g_JobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msgW("MSG_ERROR_OPENREG", uimsg, _countof(uimsg),
						   L"Couldn't open the registry.\n", UILanguageFileW);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	switch (g_JobInfo.dwMode) {
	case MODE_AUTOLOGIN:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_LOGIN);
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), TRUE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), FALSE);
		EnableItem(hWnd, EDIT_USER, g_JobInfo.bUsername);
		EnableItem(hWnd, EDIT_PASSWORD, g_JobInfo.bPassword);
		break;
	case MODE_MACRO:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_MACRO);
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), FALSE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), TRUE);
		break;
	case MODE_DIRECT:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_DIRECT);
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), FALSE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), FALSE);
		break;
	}

	if (wcslen(g_JobInfo.szName) == 0)
		wcscpy_s(g_JobInfo.szName, _countof(g_JobInfo.szName), g_JobInfo.szHostName);

	::SetDlgItemTextW(hWnd, EDIT_ENTRY, g_JobInfo.szName);
	::SetDlgItemTextW(hWnd, EDIT_HOST, g_JobInfo.szHostName);
	::SetDlgItemTextW(hWnd, EDIT_USER, g_JobInfo.szUsername);

	if (g_JobInfo.bLockBox == TRUE) {
		if (g_JobInfo.dwMode == MODE_AUTOLOGIN) {
			DecryptPassword(g_JobInfo.szPassword, szEncodePassword, hWnd);
			::SetDlgItemTextA(hWnd, EDIT_PASSWORD, szEncodePassword);
			SecureZeroMemory(szEncodePassword, sizeof(szEncodePassword));
		} else {
			::SetDlgItemTextA(hWnd, EDIT_PASSWORD, "");
		}
	} else {
		CryptUnprotectMemory(g_JobInfo.szPassword, sizeof(g_JobInfo.szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		EncodePassword(g_JobInfo.szPassword, szEncodePassword);
		CryptProtectMemory(g_JobInfo.szPassword, sizeof(g_JobInfo.szPassword), CRYPTPROTECTMEMORY_SAME_PROCESS);
		::SetDlgItemTextA(hWnd, EDIT_PASSWORD, szEncodePassword);
		SecureZeroMemory(szEncodePassword, sizeof(szEncodePassword));
	}

	::SetDlgItemTextW(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile);

	::CheckDlgButton(hWnd, CHECK_USER, g_JobInfo.bUsername);

	::CheckDlgButton(hWnd, CHECK_PASSWORD, g_JobInfo.bPassword);

	::CheckDlgButton(hWnd, CHECK_LOCKBOX, g_JobInfo.bLockBox);

	::CheckDlgButton(hWnd, CHECK_TTSSH, g_JobInfo.bTtssh);

	// 秘密鍵ファイルの追加 (2005.1.28 yutaka)
	::SetDlgItemTextW(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile);
	if (g_JobInfo.bTtssh == TRUE) {
		EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), TRUE);
		if (g_JobInfo.bChallenge) {
			SendMessage(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), FALSE);
		} else if (g_JobInfo.bPageant) {
			SendMessage(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), FALSE);
		} else {
			SendMessage(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), TRUE);
		}

	} else {
		SendMessage(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), FALSE);

	}

	::CheckDlgButton(hWnd, CHECK_STARTUP, g_JobInfo.bStartup);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) DeleteLoginHostInformation()
	Outline			: 設定情報を削除する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL DeleteLoginHostInformation(HWND hWnd)
{
	LRESULT	index;
	wchar_t	szEntryName[MAX_PATH];
	wchar_t	szSubKey[MAX_PATH * 2];
	wchar_t	uimsg[MAX_UIMSG];
	DWORD	dwErr;

	if ((index = ::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETCURSEL, 0, 0)) == LB_ERR) {
		UTIL_get_lang_msgW("MSG_ERROR_SELECTREG", uimsg, _countof(uimsg),
						   L"Select deleted registry name", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	if (::SendDlgItemMessageW(hWnd, LIST_HOST, LB_GETTEXT, (WPARAM) index, (LPARAM)szEntryName) == LB_ERR) {
		UTIL_get_lang_msgW("MSG_ERROR_GETDELETEREG", uimsg, _countof(uimsg),
						   L"Couldn't get the deleting entry", UILanguageFileW);
		::MessageBoxW(hWnd, uimsg, L"TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	_snwprintf_s(szSubKey, _countof(szSubKey), _TRUNCATE, L"%s\\%s", TTERM_KEY, szEntryName);
	if (RegDelete(HKEY_CURRENT_USER, szSubKey) != ERROR_SUCCESS) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msgW("MSG_ERROR_DELETEREG", uimsg, _countof(uimsg),
						   L"Couldn't delete the registry.\n", UILanguageFileW);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	InitListMenu(hWnd);
	InitListBox(hWnd);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) ManageWMCommand_Config()
	Outline			: 設定情報ダイアログのWM_COMMANDを処理する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
					: WPARAM		wParam		(In) 
	Return Value	: 処理 TRUE / 未処理 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ManageWMCommand_Config(HWND hWnd, WPARAM wParam)
{
	LRESULT ret = 0;
	wchar_t title[MAX_UIMSG];
	wchar_t *filter;
	LockBoxDlgPrivateData pData;
	char szPassword[MAX_PATH];

	// 秘密鍵ファイルのコントロール (2005.1.28 yutaka)
	switch(wParam) {
	case CHECK_TTSSH | (BN_CLICKED << 16) :
		ret = SendMessage(GetDlgItem(hWnd, CHECK_TTSSH), BM_GETCHECK, 0, 0);
		if (ret & BST_CHECKED) {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), TRUE);

		} else {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), FALSE);

		}
		return TRUE;

	case IDC_CHALLENGE_CHECK | (BN_CLICKED << 16) :
		// "use Challenge"をチェックした場合は鍵ファイルをdisabledにする。(2007.11.14 yutaka)
		ret = SendMessage(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), BM_GETCHECK, 0, 0);
		if (ret & BST_CHECKED) {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), FALSE);

		} else {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), TRUE);

		}
		return TRUE;

	case IDC_PAGEANT_CHECK | (BN_CLICKED << 16) :
		// "use Pageant"のクリックに対応。(2008.5.26 maya)
		ret = SendMessage(GetDlgItem(hWnd, IDC_PAGEANT_CHECK), BM_GETCHECK, 0, 0);
		if (ret & BST_CHECKED) {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), FALSE);

		} else {
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_PATH), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_KEYFILE_BUTTON), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHALLENGE_CHECK), TRUE);

		}
		return TRUE;

	default:
		break;
	}

	switch(LOWORD(wParam)) {
	case IDOK:
	case IDCANCEL:
		::SetDlgItemTextA(hWnd, EDIT_PASSWORD, ""); // エディットコントロールのクリア
		g_hWndMenu = NULL;
		::EndDialog(hWnd, TRUE);
		return TRUE;
	case BUTTON_SET:
		SaveLoginHostInformation(hWnd);
		return TRUE;
	case BUTTON_DELETE:
		DeleteLoginHostInformation(hWnd);
		return TRUE;
	case BUTTON_ETC:
		::GetDlgItemTextW(hWnd, EDIT_ENTRY, g_JobInfo.szName, MAX_PATH);
		TTDialogBox(g_hI, MAKEINTRESOURCEW(DIALOG_ETC), hWnd, DlgCallBack_Etc);
		return TRUE;
	case LIST_HOST:
		if (HIWORD(wParam) == LBN_SELCHANGE)
			LoadLoginHostInformation(hWnd);
		return TRUE;
	case CHECK_USER:
		if (IsDlgButtonChecked(hWnd, CHECK_USER) == BST_CHECKED)
			EnableItem(hWnd, EDIT_USER, TRUE);
		else {
			EnableItem(hWnd, EDIT_USER, FALSE);
			::CheckDlgButton(hWnd, CHECK_PASSWORD, BST_UNCHECKED);
			::SendMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_PASSWORD, (LPARAM) BST_UNCHECKED);
			::CheckDlgButton(hWnd, CHECK_LOCKBOX, BST_UNCHECKED);
			::SendMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_LOCKBOX, (LPARAM) BST_UNCHECKED);
		}
		return TRUE;
	case CHECK_PASSWORD:
		if (IsDlgButtonChecked(hWnd, CHECK_PASSWORD) == BST_CHECKED) {
			EnableItem(hWnd, EDIT_PASSWORD, TRUE);
			::CheckDlgButton(hWnd, CHECK_USER, BST_CHECKED);
			::PostMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_USER, (LPARAM) BST_CHECKED);
		} else {
			EnableItem(hWnd, EDIT_PASSWORD, FALSE);
			::CheckDlgButton(hWnd, CHECK_LOCKBOX, BST_UNCHECKED);
			::PostMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_LOCKBOX, (LPARAM) BST_UNCHECKED);
		}
		return TRUE;
	case CHECK_LOCKBOX:
		if (IsDlgButtonChecked(hWnd, CHECK_LOCKBOX) == BST_CHECKED) {
			if (g_szLockBox[0] == 0) {
				pData.bLockBox = g_JobInfo.bLockBox;
				pData.pEncryptPassword = g_JobInfo.szPassword;
				pData.pDecryptPassword = szPassword;
				pData.nMessageFlag = 0;
				if (TTDialogBoxParam(g_hI, MAKEINTRESOURCEW(DIALOG_LOCKBOX), hWnd, DlgCallBack_LockBox, (LPARAM)&pData) == FALSE) {
					// キャンセルされたのでチェックをはずす
					::CheckDlgButton(hWnd, CHECK_LOCKBOX, BST_UNCHECKED);
				} else {
					// ユーザー、パスワードをEnableにする
					::CheckDlgButton(hWnd, CHECK_USER, BST_CHECKED);
					::PostMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_USER, (LPARAM) BST_CHECKED);
					::CheckDlgButton(hWnd, CHECK_PASSWORD, BST_CHECKED);
					::PostMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_PASSWORD, (LPARAM) BST_CHECKED);
				}
				SecureZeroMemory(szPassword, sizeof(szPassword));
			}
		}
		return TRUE;
	case BUTTON_LOCKBOX:
		pData.bLockBox = g_JobInfo.bLockBox;
		pData.pEncryptPassword = g_JobInfo.szPassword;
		pData.pDecryptPassword = szPassword;
		pData.nMessageFlag = 0;
		if (TTDialogBoxParam(g_hI, MAKEINTRESOURCEW(DIALOG_LOCKBOX), hWnd, DlgCallBack_LockBox, (LPARAM)&pData) == TRUE) {
			if (g_JobInfo.bLockBox == TRUE) {
				::SetDlgItemTextA(hWnd, EDIT_PASSWORD, pData.pDecryptPassword);
			}
		}
		SecureZeroMemory(szPassword, sizeof(szPassword));
		return TRUE;
	case CHECK_INI_FILE:
		if (IsDlgButtonChecked(hWnd, CHECK_INI_FILE) == BST_CHECKED)
			EnableItem(hWnd, COMBO_INI_FILE, TRUE);
		else
			EnableItem(hWnd, COMBO_INI_FILE, FALSE);
		return TRUE;
	case BUTTON_MACRO:
		::GetDlgItemTextW(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile, MAX_PATH);
		UTIL_get_lang_msgW("FILEDLG_MACRO_TITLE", title, _countof(title),
						   L"specifying macro file", UILanguageFileW);
		GetI18nStrWW("TTMenu", "FILEDLG_MACRO_FILTER",
					 L"macro file(*.ttl)\\0*.ttl\\0all files(*.*)\\0*.*\\0\\0", UILanguageFileW,
					 &filter);
		OpenFileDlg(hWnd, EDIT_MACRO, title, filter, g_JobInfo.szMacroFile, _countof(g_JobInfo.szMacroFile));
		free(filter);
		return TRUE;

	case IDC_KEYFILE_BUTTON:
		::GetDlgItemTextW(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile, MAX_PATH);
		UTIL_get_lang_msgW("FILEDLG_KEY_TITLE", title, _countof(title),
						   L"specifying private key file", UILanguageFileW);
		GetI18nStrWW("TTMenu", "FILEDLG_KEY_FILTER",
					 L"identity files\\0identity;id_rsa;id_dsa\\0identity(RSA1)\\0identity\\0id_rsa(SSH2)\\0id_rsa\\0id_dsa(SSH2)\\0id_dsa\\0all(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filter);
		OpenFileDlg(hWnd, IDC_KEYFILE_PATH, title, filter, g_JobInfo.PrivateKeyFile, _countof(g_JobInfo.PrivateKeyFile));
		free(filter);
		return TRUE;

	case RADIO_LOGIN:
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), TRUE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), FALSE);
		if (IsDlgButtonChecked(hWnd, CHECK_USER) == BST_CHECKED)
			EnableItem(hWnd, EDIT_USER, TRUE);
		else {
			EnableItem(hWnd, EDIT_USER, FALSE);
		}
		if (IsDlgButtonChecked(hWnd, CHECK_PASSWORD) == BST_CHECKED)
			EnableItem(hWnd, EDIT_PASSWORD, TRUE);
		else
			EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		return TRUE;
	case RADIO_MACRO:
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), FALSE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), TRUE);
		return TRUE;
	case RADIO_DIRECT:
		EnableWindows(hWnd, autologin_items, _countof(autologin_items), FALSE);
		EnableWindows(hWnd, macro_items, _countof(macro_items), FALSE);
		return TRUE;
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL) ManageWMCommand_Etc()
	Outline			: 詳細設定情報ダイアログのWM_COMMANDを処理する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
					: WPARAM		wParam		(In) 
	Return Value	: 処理 TRUE / 未処理 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ManageWMCommand_Etc(HWND hWnd, WPARAM wParam)
{
	wchar_t	szPath[MAX_PATH];
	wchar_t	title[MAX_UIMSG];
	wchar_t *filter;

	switch(LOWORD(wParam)) {
	case IDOK:
		SaveEtcInformation(hWnd);
		::EndDialog(hWnd, TRUE);
		return TRUE;
	case IDCANCEL:
		::EndDialog(hWnd, FALSE);
		return TRUE;
	case BUTTON_DEFAULT:
		SetDefaultEtcDlg(hWnd);
		return TRUE;
	case BUTTON_TTMPATH:
		::GetDlgItemTextW(hWnd, EDIT_TTMPATH, szPath, MAX_PATH);
		UTIL_get_lang_msgW("FILEDLG_TERATERM_TITLE", title, _countof(title),
						   L"specifying TeraTerm", UILanguageFileW);
		GetI18nStrWW("TTMenu", "FILEDLG_TERATERM_FILTER",
					 L"execute file(*.exe)\\0*.exe\\0all files(*.*)\\0*.*\\0\\0", UILanguageFileW, &filter);
		OpenFileDlg(hWnd, EDIT_TTMPATH, title, filter, szPath, _countof(szPath));
		free(filter);
		return TRUE;
	case BUTTON_INITFILE:
		::GetDlgItemTextW(hWnd, EDIT_INITFILE, szPath, MAX_PATH);
		UTIL_get_lang_msgW("FILEDLG_INI_TITLE", title, _countof(title),
						   L"specifying config file", UILanguageFileW);
		GetI18nStrWW("TTMenu", "FILEDLG_INI_FILTER", L"config file(*.ini)\\0*.ini\\0all files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filter);
		OpenFileDlg(hWnd, EDIT_INITFILE, title, filter, szPath, _countof(szPath));
		free(filter);
		return TRUE;
	case BUTTON_LOG:
		::GetDlgItemTextW(hWnd, EDIT_LOG, szPath, MAX_PATH);
		UTIL_get_lang_msgW("FILEDLG_LOG_TITLE", title, _countof(title),
						   L"specifying log file", UILanguageFileW);
		GetI18nStrWW("TTMenu", "FILEDLG_LOG_FILTER",
					 L"log file(*.log)\\0*.log\\0all files(*.*)\\0*.*\\0\\0",
					 UILanguageFileW, &filter);
		OpenFileDlg(hWnd, EDIT_LOG, title, filter, szPath, _countof(szPath));
		free(filter);
		return TRUE;
	}

	return FALSE;
}


/* ==========================================================================
	Function Name	: (BOOL) ManageWMCommand_Version()
	Outline			: 「バージョン情報」ダイアログのWM_COMMANDを処理する。
	Arguments		: HWND			hWnd		(In) ダイアログのハンドル
					: WPARAM		wParam		(In) 
	Return Value	: 処理 TRUE / 未処理 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ManageWMCommand_Version(HWND hWnd, WPARAM wParam)
{
	switch(LOWORD(wParam)) {
	case IDOK:
		::EndDialog(hWnd, TRUE);
		return TRUE;
	case IDCANCEL:
		::EndDialog(hWnd, TRUE);
		return TRUE;
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL) ManageWMCommand_Menu()
	Outline			: メインウインドウのWM_COMMANDを処理する。
	Arguments		: HWND			hWnd		(In) ウインドウのハンドル
					: WPARAM		wParam		(In) 
	Return Value	: 処理 TRUE / 未処理 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ManageWMCommand_Menu(HWND hWnd, WPARAM wParam)
{
	wchar_t	uimsg[MAX_UIMSG];

	switch(LOWORD(wParam)) {
	case ID_TMENU_ADD:
		TTDialogBox(g_hI, MAKEINTRESOURCEW(DIALOG_CONFIG), 0, DlgCallBack_Config);
		return TRUE;
	case ID_TMENU_CLOSE:
		::DestroyWindow(hWnd);
		return	TRUE;
	case ID_VERSION:
		TTDialogBox(g_hI, MAKEINTRESOURCEW(DIALOG_VERSION), hWnd, DlgCallBack_Version);
		return TRUE;
	case ID_ICON:
		if (GetMenuState(g_hConfigMenu, ID_ICON, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msgW("MENU_ICON", uimsg, _countof(uimsg), STR_ICONMODE, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_ICON, MF_BYCOMMAND, ID_ICON, uimsg);
			g_MenuData.dwIconMode = MODE_SMALLICON;
		} else {
			UTIL_get_lang_msgW("MENU_ICON", uimsg, _countof(uimsg), STR_ICONMODE, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_ICON, MF_CHECKED | MF_BYCOMMAND, ID_ICON, uimsg);
			g_MenuData.dwIconMode = MODE_LARGEICON;
		}
		RedrawMenu(hWnd);
		return	TRUE;
	case ID_LEFTPOPUP:
		if (GetMenuState(g_hConfigMenu, ID_LEFTPOPUP, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msgW("MENU_LEFTPOPUP", uimsg, _countof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_LEFTPOPUP, MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
			g_MenuData.bLeftButtonPopup = FALSE;
		} else {
			UTIL_get_lang_msgW("MENU_LEFTPOPUP", uimsg, _countof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_LEFTPOPUP, MF_CHECKED | MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
			g_MenuData.bLeftButtonPopup = TRUE;
		}
		return	TRUE;
	case ID_HOTKEY:
		if (GetMenuState(g_hConfigMenu, ID_HOTKEY, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msgW("MENU_HOTKEY", uimsg, _countof(uimsg), STR_HOTKEY, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_HOTKEY, MF_BYCOMMAND, ID_HOTKEY, uimsg);
			::UnregisterHotKey(g_hWnd, WM_MENUOPEN);
			g_MenuData.bHotkey = FALSE;
		} else {
			UTIL_get_lang_msgW("MENU_HOTKEY", uimsg, _countof(uimsg), STR_HOTKEY, UILanguageFileW);
			::ModifyMenuW(g_hConfigMenu, ID_HOTKEY, MF_CHECKED | MF_BYCOMMAND, ID_HOTKEY, uimsg);
			::RegisterHotKey(g_hWnd, WM_MENUOPEN, MOD_CONTROL | MOD_ALT, 'M');
			g_MenuData.bHotkey = TRUE;
		}
		return	TRUE;
	case ID_FONT:
		SetMenuFont(hWnd);
		break;
	case ID_NOENTRY:
		return	TRUE;
	default:
		ConnectHost(hWnd, LOWORD(wParam));
		return TRUE;
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL CALLBACK) DlgCallBack_Config()
	Outline			: 設定ダイアログのコールバック関数
					: （DialogProcのヘルプ参照）
	Arguments		: 
					: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
INT_PTR CALLBACK DlgCallBack_Config(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TEXTMETRIC			textMetric;
	PDRAWITEMSTRUCT		lpdis;
	LPMEASUREITEMSTRUCT	lpmis;
	static COLORREF		crSelText;
	static COLORREF		crSelBkgnd;
	static COLORREF		crText;
	static COLORREF		crBkgnd;

	switch(uMsg) {
	case WM_INITDIALOG:
		if (g_hWndMenu == NULL)
			g_hWndMenu = hWnd;
		else {
			::SetForceForegroundWindow(g_hWndMenu);
			::EndDialog(hWnd, FALSE);
		}
		SetDlgPos(hWnd, POSITION_CENTER);
		PostMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
		PostMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
		CreateTooltip(hWnd);
		crText		= ::GetSysColor(COLOR_WINDOWTEXT);
		crBkgnd		= ::GetSysColor(COLOR_WINDOW);
		crSelText	= ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		crSelBkgnd	= ::GetSysColor(COLOR_HIGHLIGHT);
		InitConfigDlg(hWnd);
		return TRUE;
	case WM_COMMAND:
		return ManageWMCommand_Config(hWnd, wParam);
	case WM_DESTROY:
		if (g_hWndTip != NULL) {
			::DestroyWindow(g_hWndTip);
			g_hWndTip = NULL;
		}
		if (g_hHook != NULL) {
			::UnhookWindowsHookEx(g_hHook);
			g_hHook = NULL;
		}
		if (g_hIconLeft != NULL) {
			::DestroyIcon(g_hIconLeft);
			g_hIconLeft = NULL;
		}
		if (g_hIconRight != NULL) {
			::DestroyIcon(g_hIconRight);
			g_hIconRight = NULL;
		}
		return TRUE;
	case WM_MEASUREITEM:
		lpmis = (LPMEASUREITEMSTRUCT) lParam;
		lpmis->itemHeight = LISTBOX_HEIGHT;
		return TRUE;
	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT) lParam;
		if (lpdis->itemID == -1 || lpdis->itemData >= MAXJOBNUM)
			return TRUE;
		if (lpdis->itemState & ODS_SELECTED) {
			::SetTextColor(lpdis->hDC, crSelText);
			::SetBkColor(lpdis->hDC, crSelBkgnd);
		} else {
			::SetTextColor(lpdis->hDC, crText);
			::SetBkColor(lpdis->hDC, crBkgnd);
		}
		::GetTextMetrics(lpdis->hDC, &textMetric);
		NameList *pNameList = getNthNameList(lpdis->itemData);
		if (pNameList) {
			::ExtTextOutW(lpdis->hDC,
						lpdis->rcItem.left + LISTBOX_WIDTH,
						lpdis->rcItem.top + (ICONSIZE_SMALL - textMetric.tmHeight) / 2,
						ETO_OPAQUE,
						&lpdis->rcItem,
						pNameList->szName,
						(UINT)wcslen(pNameList->szName),
						NULL);
			::DrawIconEx(lpdis->hDC,
						lpdis->rcItem.left + (LISTBOX_WIDTH - ICONSIZE_SMALL) / 2,
						lpdis->rcItem.top + (LISTBOX_HEIGHT - ICONSIZE_SMALL) / 2,
						pNameList->hSmallIcon,
						ICONSIZE_SMALL,
						ICONSIZE_SMALL,
						0,
						NULL,
						DI_NORMAL);
		}
		return TRUE;
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL CALLBACK) DlgCallBack_Etc()
	Outline			: 詳細設定ダイアログのコールバック関数
					: （DialogProcのヘルプ参照）
	Arguments		: 
					: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
INT_PTR CALLBACK DlgCallBack_Etc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /* lParam unusedParam */)
{
	switch(uMsg) {
	case WM_INITDIALOG:
		SetDlgPos(hWnd, POSITION_CENTER);
		InitEtcDlg(hWnd);
		return TRUE;
	case WM_COMMAND:
		return ManageWMCommand_Etc(hWnd, wParam);
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL CALLBACK) DlgCallBack_Version()
	Outline			: 「バージョン情報」ダイアログのコールバック関数
					: （DialogProcのヘルプ参照）
	Arguments		: 
					: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
INT_PTR CALLBACK DlgCallBack_Version(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /* lParam unusedParam */)
{
	switch(uMsg) {
	case WM_INITDIALOG:
		SetDlgPos(hWnd, POSITION_CENTER);
		PostMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
		PostMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
		SendDlgItemMessage(hWnd, IDC_TTPMENU_ICON, STM_SETICON, (WPARAM)g_hIcon, 0);
		InitVersionDlg(hWnd);
		return TRUE;
	case WM_COMMAND:
		return ManageWMCommand_Version(hWnd, wParam);
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL) DecryptPassword()
	Outline			: 暗号化されたパスワードの復号処理
	Arguments		: char *szEncryptPassword	(in)	暗号化されたパスワード
					: char *szDecryptPassword	(out)	復号されたパスワード
					: HWND hWnd					(in)	ダイアログのハンドル
	Return Value	: 成功 TRUE、失敗 FALSE
	Reference		:
	Renewal			:
	Notes			: グローバル変数 g_szLockBox (in)、g_hI (in) を参照している
	Attention		:
	Up Date			:
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL DecryptPassword(const char *szEncryptPassword, char *szDecryptPassword, HWND hWnd)
{
	LockBoxDlgPrivateData pData;
	char szEncryptKey[CRYPTPROTECTMEMORYLEN];
	BOOL ret;

	ret = FALSE;
	if (g_szLockBox[0] != 0) {
		memcpy(szEncryptKey, g_szLockBox, CRYPTPROTECTMEMORYLEN);
		CryptUnprotectMemory(szEncryptKey, CRYPTPROTECTMEMORYLEN, CRYPTPROTECTMEMORY_SAME_PROCESS);
		if (Encrypt2EncDec(szDecryptPassword, (const unsigned char *)szEncryptKey, (Encrypt2ProfileP)szEncryptPassword, 0) == 1) {
			ret = TRUE;
		} else {
			pData.nMessageFlag = 1;
		}
		SecureZeroMemory(szEncryptKey, sizeof(szEncryptKey));
	} else {
		pData.nMessageFlag = 0;
	}
	if (ret == FALSE) {
		pData.bLockBox = TRUE;
		pData.pEncryptPassword = szEncryptPassword;
		pData.pDecryptPassword = szDecryptPassword;
		if ((ret = (BOOL)TTDialogBoxParam(g_hI, MAKEINTRESOURCEW(DIALOG_LOCKBOX), hWnd, DlgCallBack_LockBox, (LPARAM)&pData)) == FALSE) {
			szDecryptPassword[0] = 0;
		}
	}

	return ret;
}

/* ==========================================================================
	Function Name	: (BOOL CALLBACK) DlgCallBack_LockBox()
	Outline			: 「LockBox」ダイアログのコールバック関数
	Arguments		: HWND		hWnd	(In)	ダイアログボックスを識別するハンドル
					: UINT		uMsg	(In)	メッセージ
					: WPARAM	wParam	(In)	追加のメッセージ固有情報
					: LPARAM	lParam	(In)	復号したパスワードの保存先、
					:							lParam[0]が0ではない場合はエラーメッセージを表示する
					: 					(Out)	復号したパスワード(成功時)
	Return Value	: 成功 TRUE
					: 失敗 FALSE
	Reference		: DialogProcのヘルプ参照
	Renewal			:
	Notes			: 成功した場合、g_szLockBoxに入力された文字列が設定される
	Attention		:
	Up Date			:
   ======1=========2=========3=========4=========5=========6=========7======= */
INT_PTR CALLBACK DlgCallBack_LockBox(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t uimsg[MAX_UIMSG];
	WORD cchEncryptKey;
	char szEncodeEncryptKey[CRYPTPROTECTMEMORYLEN] = {};
	LockBoxDlgPrivateData *pData;
	static int error;
	static const DlgTextInfo text_info[] = {
		{ 0,						"DLG_LOCKBOX_TITLE" },
		{ IDC_LOCKBOX_LABEL,		"DLG_LOCKBOX_LABEL" },
		{ IDOK,						"BTN_OK" },
		{ IDCANCEL,					"BTN_CANCEL" }
	};

	switch(uMsg) {
	case WM_INITDIALOG:
		SetDlgPos(hWnd, POSITION_CENTER);
		PostMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
		PostMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
		SetI18nDlgStrsW(hWnd, "TTMenu", text_info, _countof(text_info), UILanguageFileW);
		SendDlgItemMessage(hWnd, IDC_TTPMENU_ICON, STM_SETICON, (WPARAM)g_hIcon, 0);
		pData = (LockBoxDlgPrivateData *)lParam;
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)pData);
		SendDlgItemMessage(hWnd, IDC_LOCKBOX_EDIT, EM_LIMITTEXT, ENCRYPT2_PWD_MAX_LEN, 0);

		if (g_szLockBox[0] != 0) {
			memcpy(szEncodeEncryptKey, g_szLockBox, CRYPTPROTECTMEMORYLEN);
			CryptUnprotectMemory(szEncodeEncryptKey, CRYPTPROTECTMEMORYLEN, CRYPTPROTECTMEMORY_SAME_PROCESS);
			::SetDlgItemTextA(hWnd, IDC_LOCKBOX_EDIT, (LPCSTR)szEncodeEncryptKey);
			SecureZeroMemory(szEncodeEncryptKey, sizeof(szEncodeEncryptKey));
		} else {
			::SetDlgItemTextA(hWnd, IDC_LOCKBOX_EDIT, "");
		}
		if (pData->nMessageFlag == 0) {
			error = 0;
			UTIL_get_lang_msgW("DLG_LOCKBOX_SHOW", uimsg, _countof(uimsg),
							   L"This password is used to lock and unlock the LockBox.", UILanguageFileW);
		} else {
			error = 1;
			UTIL_get_lang_msgW("DLG_LOCKBOX_WRONG", uimsg, _countof(uimsg),
							   L"Incorrect password.", UILanguageFileW);
		}
		::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, uimsg);
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (error != 0) {
			int nID = GetWindowLong((HWND)lParam, GWL_ID);
			if (nID && nID == IDC_LOCKBOX_MESSAGE) {
				::SetBkMode((HDC)wParam, TRANSPARENT);
				::SetTextColor((HDC)wParam, RGB(255, 0, 0));
				::SetBkColor((HDC)wParam, ::GetSysColor(COLOR_WINDOW));
				return (LRESULT)GetStockObject(NULL_BRUSH);
			}
		}
		return FALSE;

	case WM_COMMAND:

		if(HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_LOCKBOX_EDIT) {
			SendMessage(hWnd, DM_SETDEFID, (WPARAM)IDOK, (LPARAM)0);
		}

		switch (wParam) {
		case IDOK:
			cchEncryptKey = (WORD)::SendDlgItemMessage(hWnd, IDC_LOCKBOX_EDIT, EM_LINELENGTH, (WPARAM)0, (LPARAM)0);
			if (cchEncryptKey > ENCRYPT2_PWD_MAX_LEN) {
				error = 1;
				UTIL_get_lang_msgW("DLG_LOCKBOX_STRTOOLONG", uimsg, _countof(uimsg),
								   L"The password is too long.", UILanguageFileW);
				::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, uimsg);
			} else if (cchEncryptKey == 0) {
				error = 1;
				UTIL_get_lang_msgW("DLG_LOCKBOX_NOSTR", uimsg, _countof(uimsg),
								   L"The password is not entered.", UILanguageFileW);
				::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, uimsg);
			} else {
				*(WORD *)szEncodeEncryptKey = ENCRYPT2_PWD_MAX_LEN;
				LRESULT read = SendDlgItemMessage(hWnd, IDC_LOCKBOX_EDIT, EM_GETLINE, (WPARAM)0, (LPARAM)szEncodeEncryptKey);
				if (read > 0 && read <= ENCRYPT2_PWD_MAX_LEN) {
					szEncodeEncryptKey[read] = 0;
				} else {
					SecureZeroMemory(szEncodeEncryptKey, sizeof(szEncodeEncryptKey));
					assert(FALSE);
				}
				// 復号可否に関わらず、g_szLockBox を上書きする
				SecureZeroMemory(g_szLockBox, sizeof(g_szLockBox));
				memcpy(g_szLockBox, szEncodeEncryptKey, cchEncryptKey + 1);
				CryptProtectMemory(g_szLockBox, CRYPTPROTECTMEMORYLEN, CRYPTPROTECTMEMORY_SAME_PROCESS);
				pData = (LockBoxDlgPrivateData *)GetWindowLongPtr(hWnd, DWLP_USER);
				if (pData->bLockBox == TRUE) {
					::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, L"- - -");
					error = 0;
					InvalidateRect(hWnd, NULL, TRUE);
					UpdateWindow(hWnd);
					if (Encrypt2EncDec(pData->pDecryptPassword, (const unsigned char *)szEncodeEncryptKey,
									   (Encrypt2ProfileP)pData->pEncryptPassword, 0) == 0) {
						SecureZeroMemory(szEncodeEncryptKey, sizeof(szEncodeEncryptKey));
						error = 1;
						UTIL_get_lang_msgW("DLG_LOCKBOX_WRONG", uimsg, _countof(uimsg),
										   L"Incorrect password.", UILanguageFileW);
						::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, uimsg);
						SecureZeroMemory(g_szLockBox, sizeof(g_szLockBox));
					} else{
						SecureZeroMemory(szEncodeEncryptKey, sizeof(szEncodeEncryptKey));
						UTIL_get_lang_msgW("DLG_LOCKBOX_VALID", uimsg, _countof(uimsg),
										   L"Correct password.", UILanguageFileW);
						::SetDlgItemTextW(hWnd, IDC_LOCKBOX_MESSAGE, uimsg);
						error = 0;
						InvalidateRect(hWnd, NULL, TRUE);
						UpdateWindow(hWnd);
						Sleep(900);
						::SetDlgItemTextA(hWnd, IDC_LOCKBOX_EDIT, ""); // エディットコントロールのクリア
						::EndDialog(hWnd, TRUE);
					}
				} else {
					SecureZeroMemory(szEncodeEncryptKey, sizeof(szEncodeEncryptKey));
					::SetDlgItemTextA(hWnd, IDC_LOCKBOX_EDIT, ""); // エディットコントロールのクリア
					::EndDialog(hWnd, TRUE);
				}
			}
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case IDCANCEL:
			::SetDlgItemTextA(hWnd, IDC_LOCKBOX_EDIT, ""); // エディットコントロールのクリア
			::EndDialog(hWnd, FALSE);
			return TRUE;
		}
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (LRESULT CALLBACK) WinProc()
	Outline			: メインウインドウのコールバック関数
					: （WindowProcのヘルプ参照）
	Arguments		: 
					: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC					hDC;
	HWND				hWndItem;
	BOOL				bRet;
	SIZE				size;
	DWORD				dwIconSize;
	DWORD				dwIconSpace;
	TEXTMETRIC			textMetric;
	LPDRAWITEMSTRUCT	lpdis;
	LPMEASUREITEMSTRUCT	lpmis;
	static UINT			WM_TASKBAR_RESTART;
	NameList 			*pNameList;

	g_hWnd	= hWnd;

	switch(uMsg) {
	case WM_CREATE:
		VirtualLock(g_szLockBox, sizeof(g_szLockBox));
		VirtualLock(&g_JobInfo, sizeof(g_JobInfo));
		PostMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
		PostMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
		SetDlgPos(hWnd, POSITION_CENTER);
		::ShowWindow(hWnd, SW_HIDE);
		SetTaskTray(hWnd, NIM_ADD);
		WM_TASKBAR_RESTART = ::RegisterWindowMessageA("TaskbarCreated");
		InitMenu();
		LoadConfig();
		InitListMenu(hWnd);
		ExecStartup(hWnd);
		return TRUE;
	case WM_COMMAND:
		bRet = ManageWMCommand_Menu(hWnd, wParam);
		return bRet;
	case WM_TMENU_NOTIFY:
		::PostMessage(hWnd, (UINT) lParam, 0, 0);
		return TRUE;
	case WM_DISPLAYCHANGE:
		InitListMenu(hWnd);
		return TRUE;
	case WM_ENDSESSION:
		if (wParam == FALSE) {
			return TRUE;  // キャンセルされた場合は何もしない
		}
		SecureZeroMemory(&g_JobInfo, sizeof(g_JobInfo));
		SecureZeroMemory(g_szLockBox, sizeof(g_szLockBox));
		SaveConfig();
		return TRUE;
	case WM_DESTROY:
		SecureZeroMemory(&g_JobInfo, sizeof(g_JobInfo));
		SecureZeroMemory(g_szLockBox, sizeof(g_szLockBox));
		SaveConfig();
		SetTaskTray(hWnd, NIM_DELETE);
		::UnregisterHotKey(hWnd, WM_MENUOPEN);
		DeleteListMenuIcons();
		if (g_MenuData.hFont != NULL) {
			::DeleteObject(g_MenuData.hFont);
			g_MenuData.hFont = NULL;
		}
		::DestroyMenu(g_hListMenu);
		::DestroyMenu(g_hMenu);
		::PostQuitMessage(0);
		return TRUE;
	case WM_HOTKEY:
		if (g_MenuData.bHotkey == TRUE)
			PopupListMenu(hWnd);
		return TRUE;
	case WM_LBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
		if (g_MenuData.bLeftButtonPopup == TRUE)
			PopupListMenu(hWnd);
		else
			PopupMenu(hWnd);
		return TRUE;
	case WM_RBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		PopupMenu(hWnd);
		return TRUE;
	case WM_MEASUREITEM:
		lpmis		= (LPMEASUREITEMSTRUCT) lParam;
		hWndItem	= ::GetDlgItem((HWND) g_hListMenu, (UINT) wParam);
		hDC			= ::GetWindowDC(hWndItem);
		if (g_MenuData.hFont != NULL)
			::SelectObject(hDC, (HGDIOBJ) g_MenuData.hFont);
		pNameList = getNthNameList(lpmis->itemData);
		::GetTextExtentPoint32W(hDC, pNameList->szName, (int)wcslen(pNameList->szName), &size);
		if (g_MenuData.dwIconMode == MODE_SMALLICON) {
			lpmis->itemWidth	= ICONSPACE_SMALL + size.cx;
			lpmis->itemHeight	= g_MenuData.dwMenuHeight;
		} else {
			lpmis->itemWidth	= ICONSPACE_LARGE + size.cx;
			lpmis->itemHeight	= g_MenuData.dwMenuHeight;
		}
		::ReleaseDC(hWndItem, hDC);
		return TRUE;
	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT) lParam;
		if (lpdis->itemID == -1 || lpdis->itemData >= MAXJOBNUM)
			return TRUE;
		if (g_MenuData.hFont != NULL)
			::SelectObject(lpdis->hDC, (HGDIOBJ) g_MenuData.hFont);
		if (lpdis->itemState & ODS_SELECTED) {
			::SetTextColor(lpdis->hDC, g_MenuData.crSelMenuTxt);
			::SetBkColor(lpdis->hDC, g_MenuData.crSelMenuBg);
		} else {
			::SetTextColor(lpdis->hDC, g_MenuData.crMenuTxt);
			::SetBkColor(lpdis->hDC, g_MenuData.crMenuBg);
		}
		if (g_MenuData.dwIconMode == MODE_LARGEICON) {
			dwIconSize	= ICONSIZE_LARGE;
			dwIconSpace	= ICONSPACE_LARGE;
		} else {
			dwIconSize	= ICONSIZE_SMALL;
			dwIconSpace	= ICONSPACE_SMALL;
		}
		::GetTextMetrics(lpdis->hDC, &textMetric);
		pNameList = getNthNameList(lpdis->itemData);
		if (pNameList) {
			::ExtTextOutW(lpdis->hDC,
						lpdis->rcItem.left + dwIconSpace,
						lpdis->rcItem.top + (g_MenuData.dwMenuHeight - textMetric.tmHeight) / 2,
						ETO_OPAQUE,
						&lpdis->rcItem,
						pNameList->szName,
						(UINT)wcslen(pNameList->szName),
						NULL);
			::DrawIconEx(lpdis->hDC,
						lpdis->rcItem.left + (dwIconSpace - dwIconSize) / 2,
						lpdis->rcItem.top + (g_MenuData.dwMenuHeight - dwIconSize) / 2,
						(g_MenuData.dwIconMode == MODE_LARGEICON) ? pNameList->hLargeIcon : pNameList->hSmallIcon,
						dwIconSize,
						dwIconSize,
						0,
						NULL,
						DI_NORMAL);
		}
		return TRUE;
	}

	if (WM_TASKBAR_RESTART != 0 && uMsg == WM_TASKBAR_RESTART)
		SetTaskTray(hWnd, NIM_ADD);

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/* ==========================================================================
	Function Name	: (int WINAPI) WinMain()
	Outline			: メイン関数
	Arguments		: 
					: 
	Return Value	: 
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR /* nCmdLine unusedParam */, int /* nCmdShow unusedParam */)
{
	typedef BOOL (WINAPI *pSetDllDir)(LPCSTR);
	typedef BOOL (WINAPI *pSetDefDllDir)(DWORD);

	MSG			msg;
	HWND		hWnd;
	WNDCLASSW	winClass;
	DWORD		dwErr;
	int			fuLoad = LR_DEFAULTCOLOR;
	HMODULE		module;
	pSetDllDir	setDllDir;
	pSetDefDllDir	setDefDllDir;

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	if ((module = GetModuleHandleA("kernel32.dll")) != NULL) {
		if ((setDefDllDir = (pSetDefDllDir)GetProcAddress(module, "SetDefaultDllDirectories")) != NULL) {
			// SetDefaultDllDirectories() が使える場合は、検索パスを %WINDOWS%\system32 のみに設定する
			(*setDefDllDir)((DWORD)0x00000800); // LOAD_LIBRARY_SEARCH_SYSTEM32
		}
		else if ((setDllDir = (pSetDllDir)GetProcAddress(module, "SetDllDirectoryA")) != NULL) {
			// SetDefaultDllDirectories() が使えなくても、SetDllDirectory() が使える場合は
			// カレントディレクトリだけでも検索パスからはずしておく。
			(*setDllDir)("");
		}
	}

	// ttpmenu.exe のあるフォルダをカレントディレクトに変更する
	wchar_t *exe_dir = GetExeDirW(NULL);
	SetCurrentDirectoryW(exe_dir);
	free(exe_dir);

	SetupFNameW = GetDefaultSetupFNameW(NULL);
	SetDPIAwareness(SetupFNameW);
	UILanguageFileW = GetUILanguageFileFullW(SetupFNameW);

	SetDialogFont(//L"Tahoma", 8, 0,
				  NULL, 0, 0,
				  UILanguageFileW, "TTMenu", "DLG_TAHOMA_FONT");

	// 設定ファイル初期化 (INIファイル/レジストリ切替)
	RegInit();

	g_hI			= hI;

	if (IsWindowsNT4()){
		fuLoad = LR_VGACOLOR;
	}
	g_hIcon			= (HICON)::LoadImage(g_hI, MAKEINTRESOURCE(TTERM_ICON), IMAGE_ICON, 32, 32, fuLoad);
	g_hIconSmall	= (HICON)::LoadImage(g_hI, MAKEINTRESOURCE(TTERM_ICON), IMAGE_ICON, 16, 16, fuLoad);

	memset(&winClass, 0, sizeof(winClass));
	winClass.style			= (CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS);
	winClass.lpfnWndProc	= WinProc;
	winClass.cbClsExtra		= 0;
	winClass.cbWndExtra		= 0;
	winClass.hInstance		= g_hI;
	winClass.hIcon			= NULL;
	winClass.hCursor		= NULL;
	winClass.hbrBackground	= NULL;
	winClass.lpszMenuName	= NULL;
	winClass.lpszClassName	= TTPMENU_CLASS;

	if (::FindWindowW(TTPMENU_CLASS, NULL) == NULL) {
		if (::RegisterClassW(&winClass) == 0) {
			wchar_t		uimsg[MAX_UIMSG];
			dwErr = ::GetLastError();
			UTIL_get_lang_msgW("MSG_ERROR_WINCLASS", uimsg, _countof(uimsg),
							   L"Couldn't register the window class.\n", UILanguageFileW);
			ErrorMessage(NULL, dwErr, uimsg);
			return FALSE;
		}
	}
	
	hWnd	= ::CreateWindowExW(0,
							TTPMENU_CLASS,
							L"Main Window",
							WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							(HWND) NULL,
							(HMENU) NULL,
							g_hI,
							NULL);
	if (hWnd == NULL)
		return FALSE;

	while (::GetMessageW(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}

	if (g_hIcon != NULL) {
		::DestroyIcon(g_hIcon);
		g_hIcon = NULL;
	}
	if (g_hIconSmall != NULL) {
		::DestroyIcon(g_hIconSmall);
		g_hIconSmall = NULL;
	}
	free(UILanguageFileW);
	free(SetupFNameW);
	RegExit();
	return TRUE;
}
