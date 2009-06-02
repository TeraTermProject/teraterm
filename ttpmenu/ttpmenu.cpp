/* ==========================================================================
	Project Name		: TeraTerm Menu
	Outline				: TeraTerm Menu Function
	Version				: 0.94
	Create				: 1998-11-22(Sun)
	Update				: 2002-10-02(Wed)
	Reference			: Copyright (C) S.Hayakawa 1997-2002
   ======1=========2=========3=========4=========5=========6=========7======= */
#define		STRICT

#include	<windows.h>
#include	<commctrl.h>
#include	<windowsx.h>  // for GET_X_LPARAM(), GET_Y_LPARAM()

#include	"ttpmenu.h"
#include	"registry.h"
#include	"winmisc.h"
#include	"resource.h"

#include "compat_w95.h"

// UTF-8 TeraTermでは、デフォルトインストール先を下記に変更した。(2004.12.2 yutaka)
// さらに、デフォルトインストール先はカレントディレクトリに変更。(2004.12.14 yutaka)
#define DEFAULT_PATH "."

// グローバル変数
HWND		g_hWnd;				// メインのハンドル
HWND		g_hWndMenu = NULL;	// 設定ダイアログのハンドル
HWND		g_hWndTip;			// 設定ダイアログ内ツールチップのハンドル
HICON		g_hIcon;			// アプリケーションアイコンのハンドル
HMENU		g_hMenu;			// メニュー（非表示）のハンドル
HMENU		g_hSubMenu;			// ポップアップメニューのハンドル
HMENU		g_hListMenu;		// 設定一覧ポップアップメニューのハンドル
HMENU		g_hConfigMenu;		// 表示設定ポップアップメニューのハンドル
HHOOK		g_hHook = NULL;		// ツールチップ関連フックのハンドル
HINSTANCE	g_hI;				// アプリケーションインスタンス

JobInfo		g_JobInfo;			// カレントの設定情報構造体（設定ダイアログ）
MenuData	g_MenuData;			// TeraTerm Menuの表示設定等の構造体

char		UILanguageFile[MAX_PATH];
HFONT		g_AboutFont;
HFONT		g_ConfigFont;
HFONT		g_DetailFont;

// ttdlg.c と同じ内容
// 実行ファイルからバージョン情報を得る (2005.2.28 yutaka)
void get_file_version(char *exefile, int *major, int *minor, int *release, int *build)
{
	typedef struct {
		WORD wLanguage;
		WORD wCodePage;
	} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;
	LPLANGANDCODEPAGE lplgcode;
	UINT unLen;
	DWORD size;
	char *buf = NULL;
	BOOL ret;
	int i;
	char fmt[80];
	char *pbuf;

	size = GetFileVersionInfoSize(exefile, NULL);
	if (size == 0) {
		goto error;
	}
	buf = (char *)malloc(size);
	ZeroMemory(buf, size);

	if (GetFileVersionInfo(exefile, 0, size, buf) == FALSE) {
		goto error;
	}

	ret = VerQueryValue(buf,
			"\\VarFileInfo\\Translation", 
			(LPVOID *)&lplgcode, &unLen);
	if (ret == FALSE)
		goto error;

	for (i = 0 ; i < (int)(unLen / sizeof(LANGANDCODEPAGE)) ; i++) {
		_snprintf(fmt, sizeof(fmt), "\\StringFileInfo\\%04x%04x\\FileVersion", 
			lplgcode[i].wLanguage, lplgcode[i].wCodePage);
		VerQueryValue(buf, fmt, (LPVOID *)&pbuf, &unLen);
		if (unLen > 0) { // get success
			int n, a, b, c, d;

			n = sscanf(pbuf, "%d, %d, %d, %d", &a, &b, &c, &d);
			if (n == 4) { // convert success
				*major = a;
				*minor = b;
				*release = c;
				*build = d;
				break;
			}
		}
	}

	free(buf);
	return;

error:
	free(buf);
	*major = *minor = *release = *build = 0;
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
	char	szEntryName[MAX_PATH];
	char	szJobName[MAXJOBNUM][MAX_PATH];
	HKEY	hKey;
	DWORD	dwCnt;
	DWORD	dwIndex = 0;
	DWORD	dwSize = MAX_PATH;

	if ((hKey = RegOpen(HKEY_CURRENT_USER, TTERM_KEY)) != INVALID_HANDLE_VALUE) {
		while (RegEnumEx(hKey, dwIndex, szEntryName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			::lstrcpy(szJobName[dwIndex++], szEntryName);
			dwSize = MAX_PATH;
		}
		::lstrcpy(szJobName[dwIndex], "");
		RegClose(hKey);

		for (dwCnt = 0; dwCnt < dwIndex; dwCnt++)
			ConnectHost(hWnd, 0, szJobName[dwCnt]);
	}

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) ErrorMessage()
	Outline			: 指定メッセージ＋システムのエラーメッセージを表示する。
	Arguments		: HWND			hWnd		(In) 親ウインドウのハンドル
					: LPTSTR		msg,...		(In) 任意メッセージ文字列
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ErrorMessage(HWND hWnd, DWORD dwErr, LPTSTR msg,...)
{
	char	szBuffer[MAX_PATH] = "";

	va_list ap;
	va_start(ap, msg);
	vsprintf(szBuffer + ::lstrlen(szBuffer), msg, ap);
	va_end(ap);

	::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dwErr,
					LANG_NEUTRAL,
					szBuffer + ::lstrlen(szBuffer),
					MAX_PATH,
					NULL);

	MessageBox(hWnd, szBuffer, "TeraTerm Menu", MB_ICONSTOP | MB_OK);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) SetMenuFont()
	Outline			: フォント指定ダイアログを表示し、指定されたフォントを
					: 設定する。
	Arguments		: HWND			hWnd		(In) 親ウインドウのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL SetMenuFont(HWND hWnd)
{
	HWND		hFontWnd;
	DWORD		rgbColors;
	LOGFONT		lfFont;
	CHOOSEFONT	chooseFont;
	static int	open = 0;

	if (open == 1) {
		while ((hFontWnd = ::FindWindow(NULL, "Font")) != NULL) {
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

	if (::ChooseFont(&chooseFont) == TRUE) {
		if (g_MenuData.hFont != NULL)
			::DeleteObject((HGDIOBJ) g_MenuData.hFont);
		g_MenuData.crMenuTxt	= chooseFont.rgbColors;
		g_MenuData.lfFont		= lfFont;
		g_MenuData.hFont		= ::CreateFontIndirect(&lfFont);
		RedrawMenu(hWnd);
	}

	open = 0;

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) ExtractAssociatedIconEx()
	Outline			: アプリケーションに関連付けられたアイコンを取得する。
					: 設定する。
	Arguments		: char			*szPath		(In) アプリケーション名
					: HICON			*hLargeIcon	(Out) 大きいアイコンのハンドル
					: HICON			*hSmallIcon	(Out) 小さいアイコンのハンドル
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ExtractAssociatedIconEx(char *szPath, HICON *hLargeIcon, HICON *hSmallIcon)
{
	SHFILEINFO	sfi;

	::SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), SHGFI_LARGEICON | SHGFI_ICON);
	*hLargeIcon = ::CopyIcon(sfi.hIcon);

	::SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_ICON);
	*hSmallIcon = ::CopyIcon(sfi.hIcon);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) GetApplicationFilename()
	Outline			: レジストリより指定された設定のアプロケーション名を取得
					: する。
	Arguments		: char			*szName		(In) 設定名
					: char			*szPath		(Out) アプリケーション名
	Return Value	: 成功 TRUE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL GetApplicationFilename(char *szName, char *szPath)
{
	char	szSubKey[MAX_PATH];
	char	szDefault[MAX_PATH] = DEFAULT_PATH;

	char	szTTermPath[MAX_PATH];
	BOOL	bRet;
	BOOL	bTtssh = FALSE;
	HKEY	hKey;

	::wsprintf(szSubKey, "%s\\%s", TTERM_KEY, szName);
	if ((hKey = RegOpen(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	bRet = RegGetStr(hKey, KEY_TERATERM, szPath, MAX_PATH);
	if (bRet == FALSE || ::lstrlen(szPath) == 0) {
		RegGetDword(hKey, KEY_TTSSH, (LPDWORD) &bTtssh);
		::GetProfileString("Tera Term Pro", "Path", szDefault, szTTermPath, MAX_PATH);
		::wsprintf(szPath, "%s\\%s", szTTermPath, bTtssh ? TTSSH : TERATERM);
	}

	RegClose(hKey);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) AddTooltip()
	Outline			: 指定されたコントロールにツールチップを関連付ける
	Arguments		: int			idControl	(In) コントロールID
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL AddTooltip(int idControl)
{
	TOOLINFO	ti;

	ti.cbSize	= sizeof(TOOLINFO);
	ti.uFlags	= TTF_IDISHWND; 
	ti.hwnd		= g_hWndMenu; 
	ti.uId		= (UINT) ::GetDlgItem(g_hWndMenu, idControl); 
	ti.hinst	= 0; 
	ti.lpszText	= LPSTR_TEXTCALLBACK;

	return ::SendMessage(g_hWndTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
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
	char	uimsg[MAX_UIMSG];

	if ((hKey = RegCreate(HKEY_CURRENT_USER, TTERM_KEY)) == INVALID_HANDLE_VALUE)
		return FALSE;
	
	if (RegGetDword(hKey, KEY_ICONMODE, &(g_MenuData.dwIconMode)) == TRUE) {
		if (g_MenuData.dwIconMode == MODE_LARGEICON) {
			UTIL_get_lang_msg("MENU_ICON", uimsg, sizeof(uimsg), STR_ICONMODE, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_ICON, MF_CHECKED | MF_BYCOMMAND, ID_ICON, uimsg);
		}
	} else
		g_MenuData.dwIconMode = MODE_SMALLICON;
	
	if (RegGetDword(hKey, KEY_LEFTBUTTONPOPUP, (LPDWORD) &(g_MenuData.bLeftButtonPopup)) == FALSE)
		g_MenuData.bLeftButtonPopup = TRUE;
	if (g_MenuData.bLeftButtonPopup == TRUE) {
		UTIL_get_lang_msg("MENU_LEFTPOPUP", uimsg, sizeof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFile);
		::ModifyMenu(g_hConfigMenu, ID_LEFTPOPUP, MF_CHECKED | MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
	}

	if (RegGetDword(hKey, KEY_MENUTEXTCOLOR, &(g_MenuData.crMenuTxt)) == FALSE)
		g_MenuData.crMenuTxt = ::GetSysColor(COLOR_MENUTEXT);

	if (RegGetDword(hKey, KEY_HOTKEY, (LPDWORD) &(g_MenuData.bHotkey)) == FALSE)
		g_MenuData.bHotkey	= FALSE;
	if (g_MenuData.bHotkey == TRUE) {
		UTIL_get_lang_msg("MENU_HOTKEY", uimsg, sizeof(uimsg), STR_HOTKEY, UILanguageFile);
		::ModifyMenu(g_hConfigMenu, ID_HOTKEY, MF_CHECKED | MF_BYCOMMAND, ID_HOTKEY, uimsg);
		::RegisterHotKey(g_hWnd, WM_MENUOPEN, MOD_CONTROL | MOD_ALT, 'M');
	}

	if (RegGetDword(hKey, KEY_LF_HEIGHT, (DWORD *) &(g_MenuData.lfFont.lfHeight)) == TRUE) {
		RegGetDword(hKey, KEY_LF_WIDTH, (DWORD *) &(g_MenuData.lfFont.lfWidth));
		RegGetDword(hKey, KEY_LF_ESCAPEMENT, (DWORD *) &(g_MenuData.lfFont.lfEscapement));
		RegGetDword(hKey, KEY_LF_ORIENTATION, (DWORD *) &(g_MenuData.lfFont.lfOrientation));
		RegGetDword(hKey, KEY_LF_WEIGHT, (DWORD *) &(g_MenuData.lfFont.lfWeight));
		RegGetDword(hKey, KEY_LF_ITALIC, (DWORD *) &(g_MenuData.lfFont.lfItalic));
		RegGetDword(hKey, KEY_LF_UNDERLINE, (DWORD *) &(g_MenuData.lfFont.lfUnderline));
		RegGetDword(hKey, KEY_LF_STRIKEOUT, (DWORD *) &(g_MenuData.lfFont.lfStrikeOut));
		RegGetDword(hKey, KEY_LF_CHARSET, (DWORD *) &(g_MenuData.lfFont.lfCharSet));
		RegGetDword(hKey, KEY_LF_OUTPRECISION, (DWORD *) &(g_MenuData.lfFont.lfOutPrecision));
		RegGetDword(hKey, KEY_LF_CLIPPRECISION, (DWORD *) &(g_MenuData.lfFont.lfClipPrecision));
		RegGetDword(hKey, KEY_LF_QUALITY, (DWORD *) &(g_MenuData.lfFont.lfQuality));
		RegGetDword(hKey, KEY_LF_PITCHANDFAMILY, (DWORD *) &(g_MenuData.lfFont.lfPitchAndFamily));
		RegGetStr(hKey, KEY_LF_FACENAME, g_MenuData.lfFont.lfFaceName, LF_FACESIZE);
	} else
		::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &(g_MenuData.lfFont));

	RegClose(hKey);

	g_MenuData.crMenuBg		= ::GetSysColor(COLOR_MENU);
	g_MenuData.crSelMenuBg	= ::GetSysColor(COLOR_HIGHLIGHT);
	g_MenuData.crSelMenuTxt	= ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	g_MenuData.hFont		= ::CreateFontIndirect(&(g_MenuData.lfFont));

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
BOOL CreateTooltip(void)
{
	::InitCommonControls(); 

	g_hWndTip = ::CreateWindowEx(0,
								TOOLTIPS_CLASS,
								(LPSTR) NULL,
								TTS_ALWAYSTIP,
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

	AddTooltip(BUTTON_SET);
	AddTooltip(BUTTON_DELETE);
	AddTooltip(BUTTON_ETC);
	AddTooltip(CHECK_TTSSH);

	g_hHook = ::SetWindowsHookEx(WH_GETMESSAGE,
								GetMsgProc,
								(HINSTANCE) NULL,
								::GetCurrentThreadId()); 

	if (g_hHook == (HHOOK) NULL)
		return FALSE; 

	return TRUE; 
}

/* ==========================================================================
	Function Name	: (BOOL) ManageWMNotify_Config()
	Outline			: 設定ダイアログのWM_NOTIFYを処理する
	Arguments		: LPARAM	lParam
	Return Value	: 処理 TRUE / 未処理 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL ManageWMNotify_Config(LPARAM lParam)
{
	int				idCtrl;
	LPTOOLTIPTEXT	lpttt;

	if ((((LPNMHDR) lParam)->code) == TTN_NEEDTEXT) {
		idCtrl	= ::GetDlgCtrlID((HWND) ((LPNMHDR) lParam)->idFrom);
		lpttt	= (LPTOOLTIPTEXT) lParam;
		switch (idCtrl) {
		case BUTTON_SET:
			lpttt->lpszText	= "Regist";
			return TRUE; 
		case BUTTON_DELETE:
			lpttt->lpszText	= "Delete";
			return TRUE; 
		case BUTTON_ETC:
			lpttt->lpszText	= "Configure";
			return TRUE; 
		case CHECK_TTSSH:
			lpttt->lpszText	= "use SSH";
			return TRUE; 
		}
    }

	return FALSE; 
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

	// マルチモニタ環境では LOWORD(), HIWORD() を使ってはいけない。(2005.10.13 yutaka)
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

	// マルチモニタ環境では LOWORD(), HIWORD() を使ってはいけない。(2005.10.13 yutaka)
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
	char	szPath[MAX_PATH];
	DWORD	dwCnt = 0;
	DWORD	dwIndex = 0;

	::SendDlgItemMessage(hWnd, LIST_HOST, LB_RESETCONTENT, 0, 0);

	while (::lstrlen(g_MenuData.szName[dwIndex]) != 0) {
		if (GetApplicationFilename(g_MenuData.szName[dwIndex], szPath) == TRUE) {
			::SendDlgItemMessage(hWnd, LIST_HOST, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) g_MenuData.szName[dwIndex]);
			::SendDlgItemMessage(hWnd, LIST_HOST, LB_SETITEMDATA, (WPARAM) dwCnt, (LPARAM) dwIndex);
			dwCnt++;
		}
		dwIndex++;
	}

	return TRUE;
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
	HICON	g_hIconLeft;
	HICON	g_hIconRight;
	LOGFONT	logfont;
	HFONT	font;
	char	uimsg[MAX_UIMSG], uitmp[MAX_UIMSG];

	font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", hWnd, &logfont, &g_ConfigFont, UILanguageFile)) {
		SendDlgItemMessage(hWnd, LBL_LIST, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LIST_HOST, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, GRP_CONFIG, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_ENTRY, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_ENTRY, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, GRP_LAUNCH, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, RADIO_LOGIN, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_HOST, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_HOST, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, CHECK_USER, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_USER, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, CHECK_PASSWORD, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_PASSWORD, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, RADIO_MACRO, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_MACRO, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_MACRO, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, RADIO_DIRECT, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_ETC, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, CHECK_STARTUP, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, CHECK_TTSSH, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDC_KEYFILE_LABEL, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDC_KEYFILE_PATH, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDC_KEYFILE_BUTTON, WM_SETFONT, (WPARAM)g_ConfigFont, MAKELPARAM(TRUE,0));
	}
	else {
		g_ConfigFont = NULL;
	}

	GetWindowText(hWnd, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_TITLE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetWindowText(hWnd, uimsg);
	GetDlgItemText(hWnd, LBL_LIST, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_ITEM", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_LIST, uimsg);
	GetDlgItemText(hWnd, GRP_CONFIG, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_CONFIG", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, GRP_CONFIG, uimsg);
	GetDlgItemText(hWnd, LBL_ENTRY, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_NAME", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_ENTRY, uimsg);
	GetDlgItemText(hWnd, GRP_LAUNCH, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_PATTERN", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, GRP_LAUNCH, uimsg);
	GetDlgItemText(hWnd, RADIO_LOGIN, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_AUTO", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, RADIO_LOGIN, uimsg);
	GetDlgItemText(hWnd, LBL_HOST, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_HOST", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_HOST, uimsg);
	GetDlgItemText(hWnd, CHECK_USER, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_USER", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, CHECK_USER, uimsg);
	GetDlgItemText(hWnd, CHECK_PASSWORD, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_PASS", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, CHECK_PASSWORD, uimsg);
	GetDlgItemText(hWnd, RADIO_MACRO, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_MACRO", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, RADIO_MACRO, uimsg);
	GetDlgItemText(hWnd, RADIO_DIRECT, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_LAUNCH", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, RADIO_DIRECT, uimsg);
	GetDlgItemText(hWnd, CHECK_STARTUP, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_STARTUP", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, CHECK_STARTUP, uimsg);
	GetDlgItemText(hWnd, CHECK_TTSSH, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_SSH", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, CHECK_TTSSH, uimsg);
	GetDlgItemText(hWnd, IDC_KEYFILE_LABEL, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_KEYFILE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDC_KEYFILE_LABEL, uimsg);
	GetDlgItemText(hWnd, IDC_CHALLENGE_CHECK, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_CHALLENGE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDC_CHALLENGE_CHECK, uimsg);
	GetDlgItemText(hWnd, IDC_PAGEANT_CHECK, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_PAGEANT", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDC_PAGEANT_CHECK, uimsg);
	GetDlgItemText(hWnd, BUTTON_ETC, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_CONFIG_DETAIL", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, BUTTON_ETC, uimsg);

	memset(&g_JobInfo, 0, sizeof(JobInfo));

	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_SIZE, MF_BYCOMMAND);

	g_hIconLeft		= ::LoadIcon(g_hI, (LPCSTR)ICON_LEFT);
	g_hIconRight	= ::LoadIcon(g_hI, (LPCSTR)ICON_RIGHT);
	::SendDlgItemMessage(hWnd, BUTTON_SET, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM)(HANDLE) g_hIconLeft);
	::SendDlgItemMessage(hWnd, BUTTON_DELETE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM)(HANDLE) g_hIconRight);

	::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_MACRO, RADIO_LOGIN);
	EnableItem(hWnd, EDIT_MACRO, FALSE);
	EnableItem(hWnd, BUTTON_MACRO, FALSE);
	::CheckDlgButton(hWnd, CHECK_USER, 1);
	::CheckDlgButton(hWnd, CHECK_PASSWORD, 1);
	::CheckDlgButton(hWnd, CHECK_INI_FILE, 1);

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
	char	szDefault[MAX_PATH] = DEFAULT_PATH;
	char	szTTermPath[MAX_PATH];
	LOGFONT	logfont;
	HFONT	font;
	char	uimsg[MAX_UIMSG], uitmp[MAX_UIMSG];

	font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", hWnd, &logfont, &g_DetailFont, UILanguageFile)) {
		SendDlgItemMessage(hWnd, LBL_TTMPATH, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_TTMPATH, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_TTMPATH, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_OPTION, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_OPTION, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, GRP_INITFILE, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_INITFILE, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_INITFILE, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, GRP_AUTOLOGIN, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_LOG, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_LOG, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_LOG, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, GRP_PROMPT, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_PROMPT_USER, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_PROMPT_USER, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, LBL_PROMPT_PASS, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, EDIT_PROMPT_PASS, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDOK, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, BUTTON_DEFAULT, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDCANCEL, WM_SETFONT, (WPARAM)g_DetailFont, MAKELPARAM(TRUE,0));	}
	else {
		g_DetailFont = NULL;
	}

	GetWindowText(hWnd, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_TITLE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetWindowText(hWnd, uimsg);
	GetDlgItemText(hWnd, LBL_TTMPATH, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_APP", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_TTMPATH, uimsg);
	GetDlgItemText(hWnd, LBL_OPTION, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_OPTION", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_OPTION, uimsg);
	GetDlgItemText(hWnd, GRP_INITFILE, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_CONFIG", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, GRP_INITFILE, uimsg);
	GetDlgItemText(hWnd, GRP_AUTOLOGIN, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_AUTO", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, GRP_AUTOLOGIN, uimsg);
	GetDlgItemText(hWnd, LBL_LOG, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_LOGFILE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_LOG, uimsg);
	GetDlgItemText(hWnd, GRP_PROMPT, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_PROMPT", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, GRP_PROMPT, uimsg);
	GetDlgItemText(hWnd, LBL_PROMPT_USER, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_USER", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_PROMPT_USER, uimsg);
	GetDlgItemText(hWnd, LBL_PROMPT_PASS, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ETC_PASS", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, LBL_PROMPT_PASS, uimsg);
	GetDlgItemText(hWnd, BUTTON_DEFAULT, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("BTN_DEFAULT", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, BUTTON_DEFAULT, uimsg);
	GetDlgItemText(hWnd, IDOK, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDOK, uimsg);
	GetDlgItemText(hWnd, IDCANCEL, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDCANCEL, uimsg);

	if (::lstrlen(g_JobInfo.szTeraTerm) == 0) {
		::GetProfileString("Tera Term Pro", "Path", szDefault, szTTermPath, MAX_PATH);
		::wsprintf(g_JobInfo.szTeraTerm, "%s\\%s", szTTermPath, g_JobInfo.bTtssh ? TTSSH : TERATERM);
	}
	if (g_JobInfo.bTtssh == TRUE && lstrstri(g_JobInfo.szTeraTerm, TTSSH) == NULL)
		::wsprintf(g_JobInfo.szTeraTerm, "%s\\%s", szTTermPath, TTSSH);
	if (::lstrlen(g_JobInfo.szLoginPrompt) == 0) {
		::lstrcpy(g_JobInfo.szLoginPrompt, LOGIN_PROMPT);
	}
	if (::lstrlen(g_JobInfo.szPasswdPrompt) == 0) {
		::lstrcpy(g_JobInfo.szPasswdPrompt, PASSWORD_PROMPT);
	}

	::SetDlgItemText(hWnd, EDIT_TTMPATH, g_JobInfo.szTeraTerm);
	::SetDlgItemText(hWnd, EDIT_INITFILE, g_JobInfo.szInitFile);
	::SetDlgItemText(hWnd, EDIT_OPTION, g_JobInfo.szOption);
	::SetDlgItemText(hWnd, EDIT_PROMPT_USER, g_JobInfo.szLoginPrompt);
	::SetDlgItemText(hWnd, EDIT_PROMPT_PASS, g_JobInfo.szPasswdPrompt);

	::SetDlgItemText(hWnd, EDIT_LOG, g_JobInfo.szLog);

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
	int a, b, c, d;
	char app[_MAX_PATH], buf[1024], buf2[1024];
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG], uitmp[MAX_UIMSG];

	font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_TAHOMA_FONT", hWnd, &logfont, &g_AboutFont, UILanguageFile)) {
		SendDlgItemMessage(hWnd, IDC_VERSION, WM_SETFONT, (WPARAM)g_AboutFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(hWnd, IDC_INCLUDE, WM_SETFONT, (WPARAM)g_AboutFont, MAKELPARAM(TRUE,0));
	}
	else {
		g_AboutFont = NULL;
	}

	GetWindowText(hWnd, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("DLG_ABOUT_TITLE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetWindowText(hWnd, uimsg);
	GetDlgItemText(hWnd, IDOK, uitmp, sizeof(uitmp));
	UTIL_get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
	SetDlgItemText(hWnd, IDOK, uimsg);

	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_SIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_MINIMIZE, MF_BYCOMMAND);
	::DeleteMenu(::GetSystemMenu(hWnd, FALSE), SC_RESTORE, MF_BYCOMMAND);

	GetModuleFileName(NULL, app, sizeof(app));
	get_file_version(app, &a, &b, &c, &d);
	UTIL_get_lang_msg("DLG_ABOUT_APPNAME", uimsg, sizeof(uimsg), "launch tool", UILanguageFile);

	GetDlgItemText(hWnd, IDC_VERSION, buf, sizeof(buf));
	_snprintf(buf2, sizeof(buf2), buf, uimsg, a, b);
	SetDlgItemText(hWnd, IDC_VERSION, buf2);

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
	char	szDefault[MAX_PATH] = DEFAULT_PATH;
	char	szTTermPath[MAX_PATH];

	::GetProfileString("Tera Term Pro", "Path", szDefault, szTTermPath, MAX_PATH);
	::wsprintf(szTTermPath, "%s\\%s", szTTermPath, g_JobInfo.bTtssh ? TTSSH : TERATERM);

	::SetDlgItemText(hWnd, EDIT_TTMPATH, szTTermPath);
	::SetDlgItemText(hWnd, EDIT_INITFILE, "");
	// デフォルトオプションに /KT , /KR を追加 (2005.1.25 yutaka)
	::SetDlgItemText(hWnd, EDIT_OPTION, "/KT=UTF8 /KR=UTF8");
//	::SetDlgItemText(hWnd, EDIT_OPTION, "");
	::SetDlgItemText(hWnd, EDIT_PROMPT_USER, LOGIN_PROMPT);
	::SetDlgItemText(hWnd, EDIT_PROMPT_PASS, PASSWORD_PROMPT);

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
	NOTIFYICONDATA	nid;

	memset(&nid, 0, sizeof(nid));
	nid.cbSize				= sizeof(nid);
	nid.hWnd				= hWnd;
	nid.uID					= TTERM_ICON;
	nid.uFlags				= NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage	= WM_TMENU_NOTIFY;
	nid.hIcon				= g_hIcon;
	lstrcpy(nid.szTip, "TeraTerm Menu");

	::Shell_NotifyIcon(dwMessage, &nid);

	return TRUE;
}

/* ==========================================================================
	Function Name	: (BOOL) MakeTTL()
	Outline			: 自動ログイン用マクロファイルを生成する。
	Arguments		: char		*TTLName	(In) マクロファイル名
					: JobInfo	JobInfo		(In) 設定情報構造体
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL MakeTTL(char *TTLName, JobInfo *jobInfo)
{
	char	buf[1024];
	DWORD	dwWrite;
	HANDLE	hFile;

	hFile = ::CreateFile(TTLName,
						GENERIC_WRITE, 
						FILE_SHARE_WRITE | FILE_SHARE_READ, 
						NULL,
						CREATE_ALWAYS, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	::wsprintf(buf, "filedelete '%s'\r\n", TTLName);
	::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);

	if (::lstrlen(jobInfo->szLog) != 0) {
		::wsprintf(buf, "logopen '%s' 0 1\r\n", jobInfo->szLog);
		::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);
	}

	// telnetポート番号を付加する (2004.12.3 yutaka)
	::wsprintf(buf, "connect '%s:23'\r\n", jobInfo->szHostName);
	::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);

	if (jobInfo->bUsername == TRUE) {
		if (::lstrlen(jobInfo->szLoginPrompt) == 0)
			::lstrcpy(jobInfo->szLoginPrompt, LOGIN_PROMPT);
		::wsprintf(buf, "UsernamePrompt = '%s'\r\nUsername = '%s'\r\n", jobInfo->szLoginPrompt, jobInfo->szUsername);
		::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);
	}

	if (jobInfo->bPassword == TRUE) {
		if (::lstrlen(jobInfo->szPasswdPrompt) == 0)
			::lstrcpy(jobInfo->szPasswdPrompt, PASSWORD_PROMPT);
		::wsprintf(buf, "PasswordPrompt = '%s'\r\nPassword = '%s'\r\n", jobInfo->szPasswdPrompt, jobInfo->szPassword);
		::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);
	}

	if (jobInfo->bUsername == TRUE) {
		::wsprintf(buf, "wait   UsernamePrompt\r\nsendln Username\r\n");
		::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);
	}

	if (jobInfo->bPassword == TRUE) {
		::wsprintf(buf, "wait   PasswordPrompt\r\nsendln Password\r\n");
		::WriteFile(hFile, buf, ::lstrlen(buf), &dwWrite, NULL);
	}

	::CloseHandle(hFile);

	return TRUE;
}


static void _dquote_string(char *str, char *dst, int dst_len)
{
	int i, len, n;
	
	len = strlen(str);
	n = 0;
	for (i = 0 ; i < len ; i++) {
		if (str[i] == '"')
			n++;
	}
	if (dst_len < (len + 2*n + 2 + 1))
		return;

	*dst++ = '"';
	for (i = 0 ; i < len ; i++) {
		if (str[i] == '"') {
			*dst++ = '"';
			*dst++ = '"';

		} else {
			*dst++ = str[i];

		}
	}
	*dst++ = '"';
	*dst = '\0';
}

static void dquote_string(char *str, char *dst, int dst_len)
{
	// " で始まるか、スペースが含まれる場合にはクオートする
	if (str[0] == '"' || strchr(str, '" ') != NULL) {
		_dquote_string(str, dst, dst_len);
		return;
	}
	// そのままコピーして戻る
	strncpy_s(dst, dst_len, str, _TRUNCATE);
}

#ifdef USE_ATCMDLINE
// 空白を @ に置き換える。@自身は@@にする。(2005.1.28 yutaka)
static void replace_blank_to_mark(char *str, char *dst, int dst_len)
{
	int i, len, n;

	len = strlen(str);
	n = 0;
	for (i = 0 ; i < len ; i++) {
		if (str[i] == '@')
			n++;
	}
	if (dst_len < (len + 2*n)) 
		return;

	for (i = 0 ; i < len ; i++) {
		if (str[i] == '@') {
			*dst++ = '@';
			*dst++ = '@';

		} else if (str[i] == ' ') {
			*dst++ = '@';

		} else {
			*dst++ = str[i];

		}
	}
	*dst = '\0';

}
#endif


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
BOOL ConnectHost(HWND hWnd, UINT idItem, char *szJobName)
{
	char	szName[MAX_PATH];
	char	szDefault[MAX_PATH] = DEFAULT_PATH;

	char	szDirectory[MAX_PATH];
	char	szHostName[MAX_PATH];
	char	szTempPath[MAX_PATH];
	char	szMacroFile[MAX_PATH];
	char	szArgment[MAX_PATH] = "";
	char	*pHostName;
	TCHAR	*pt;
	JobInfo	jobInfo;
	char	cur[MAX_PATH], modulePath[MAX_PATH], fullpath[MAX_PATH];

	DWORD	dwErr;
	char uimsg[MAX_UIMSG];

	::lstrcpy(szName, (szJobName == NULL) ? g_MenuData.szName[idItem - ID_MENU_MIN] : szJobName);

	if (RegLoadLoginHostInformation(szName, &jobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msg("MSG_ERROR_MAKETTL", uimsg, sizeof(uimsg),
		                  "Couldn't read the registory data.\n", UILanguageFile);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	if (szJobName != NULL && jobInfo.bStartup == FALSE)
		return TRUE;

	if (::lstrlen(jobInfo.szTeraTerm) == 0) {
		::GetProfileString("Tera Term Pro", "Path", szDefault, jobInfo.szTeraTerm, MAX_PATH);
		::wsprintf(jobInfo.szTeraTerm, "%s\\%s", jobInfo.szTeraTerm, jobInfo.bTtssh ? TTSSH : TERATERM);
	}

	::lstrcpy(szHostName, jobInfo.szHostName);
	if ((pHostName = _tcstok(szHostName, _T(" ([{'\"|*"))) != NULL)
		pHostName = szHostName;

	if (jobInfo.dwMode != MODE_DIRECT)
		if (::lstrlen(jobInfo.szInitFile) != 0)
			::wsprintf(szArgment, "/F=\"%s\"", jobInfo.szInitFile);

	switch (jobInfo.dwMode) {
	case MODE_AUTOLOGIN:
		::GetTempPath(MAX_PATH, szTempPath);
		::GetTempFileName(szTempPath, "ttm", 0, szMacroFile);
		if (MakeTTL(szMacroFile, &jobInfo) == FALSE) {
			dwErr = ::GetLastError();
			UTIL_get_lang_msg("MSG_ERROR_MAKETTL", uimsg, sizeof(uimsg),
			                  "Could not make 'ttpmenu.TTL'\n", UILanguageFile);
			ErrorMessage(hWnd, dwErr, uimsg);
			return FALSE;
		}
		break;
	case MODE_MACRO:
		::lstrcpy(szMacroFile, jobInfo.szMacroFile);
		break;
	}

	// SSH自動ログインの場合はマクロは不要 (2005.1.25 yutaka)
	if (jobInfo.bTtssh != TRUE) {
		if (jobInfo.dwMode != MODE_DIRECT)
			::wsprintf(szArgment, "%s /M=\"%s\"", szArgment, szMacroFile);
	}

	if (::lstrlen(jobInfo.szOption) != 0)
		::wsprintf(szArgment, "%s %s", szArgment, jobInfo.szOption);

	// TTSSHが有効の場合は、自動ログインのためのコマンドラインを付加する。(2004.12.3 yutaka)
	// ユーザのパラメータを指定できるようにする (2005.1.25 yutaka)
	// 公開鍵認証をサポート (2005.1.27 yutaka)
	// /challengeをサポート (2007.11.14 yutaka)
	// /pageantをサポート (2008.5.26 maya)
	if (jobInfo.dwMode == MODE_AUTOLOGIN) {
		if (jobInfo.bTtssh == TRUE) {
			char tmp[MAX_PATH];
			char passwd[MAX_PATH], keyfile[MAX_PATH];

			strcpy(tmp, szArgment);
#ifdef USE_ATCMDLINE
			replace_blank_to_mark(jobInfo.szPassword, passwd, sizeof(passwd));
			replace_blank_to_mark(jobInfo.PrivateKeyFile, keyfile, sizeof(keyfile));
#else
			dquote_string(jobInfo.szPassword, passwd, sizeof(passwd));
			dquote_string(jobInfo.PrivateKeyFile, keyfile, sizeof(keyfile));
#endif

			if (jobInfo.bChallenge) { // keyboard-interactive
				_snprintf(szArgment, sizeof(szArgment), "%s:22 /ssh /auth=challenge /user=%s /passwd=%s %s", 
					jobInfo.szHostName,
					jobInfo.szUsername,
					passwd,
					tmp
					);

			} else if (jobInfo.bPageant) { // Pageant
				_snprintf(szArgment, sizeof(szArgment), "%s:22 /ssh /auth=pageant /user=%s %s", 
					jobInfo.szHostName,
					jobInfo.szUsername,
					tmp
					);

			} else if (jobInfo.PrivateKeyFile[0] == NULL) { // password authentication
				_snprintf(szArgment, sizeof(szArgment), "%s:22 /ssh /auth=password /user=%s /passwd=%s %s", 
					jobInfo.szHostName,
					jobInfo.szUsername,
					passwd,
					tmp
					);

			} else { // publickey
				_snprintf(szArgment, sizeof(szArgment), "%s:22 /ssh /auth=publickey /user=%s /passwd=%s /keyfile=%s %s", 
					jobInfo.szHostName,
					jobInfo.szUsername,
					passwd,
					keyfile,
					tmp
					);

			}

		} else {
			// SSHを使わない場合、/nossh オプションを付けておく。
			::wsprintf(szArgment, "%s /nossh", szArgment);
		}
	}

	// フルパス化する
	GetCurrentDirectory(sizeof(cur), cur);
	GetModuleFileName(NULL, modulePath, sizeof(modulePath));
	ExtractDirName(modulePath, modulePath);
	SetCurrentDirectory(modulePath);
	_fullpath(fullpath, jobInfo.szTeraTerm, sizeof(fullpath));
	::lstrcpy(jobInfo.szTeraTerm, fullpath);

	::lstrcpy(szDirectory, jobInfo.szTeraTerm);
	if ((::GetFileAttributes(jobInfo.szTeraTerm) & FILE_ATTRIBUTE_DIRECTORY) == 0)
		if ((pt = _tcsrchr(szDirectory, '\\')) != NULL)
			*pt	= '\0';

	SHELLEXECUTEINFO	ExecInfo;
	memset((void *) &ExecInfo, 0, sizeof(SHELLEXECUTEINFO));
	ExecInfo.cbSize			= sizeof(SHELLEXECUTEINFO);
	ExecInfo.fMask			= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	ExecInfo.hwnd			= hWnd;
	ExecInfo.lpVerb			= (LPCSTR) NULL;
	ExecInfo.lpFile			= (LPCSTR) jobInfo.szTeraTerm;
	ExecInfo.lpParameters	= (LPCSTR) szArgment;
	ExecInfo.lpDirectory	= (LPCSTR) szDirectory;
	ExecInfo.nShow			= SW_SHOWNORMAL;
	ExecInfo.hInstApp		= g_hI;

	if (::ShellExecuteEx(&ExecInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msg("MSG_ERROR_LAUNCH", uimsg, sizeof(uimsg),
		                  "Launching the application was failure.\n", UILanguageFile);
		ErrorMessage(hWnd, dwErr, uimsg);
		::DeleteFile(szTempPath);
	}

	if (::lstrlen(jobInfo.szLog) != 0) {
		Sleep(500);
		HWND hLog = ::FindWindow(NULL, "Tera Term: Log");
		if (hLog != NULL)
			ShowWindow(hLog, SW_HIDE);
	}

	SetCurrentDirectory(cur);

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
	char	uimsg[MAX_UIMSG], uitmp[MAX_UIMSG];

	for (int cnt = 0; cnt < MAXJOBNUM; cnt++) {
		g_MenuData.hLargeIcon[cnt] = NULL;
		g_MenuData.hSmallIcon[cnt] = NULL;
	}

	if (g_hListMenu == NULL) {
		g_hMenu			= ::LoadMenu(g_hI, (LPCSTR) TTERM_MENU);
		g_hSubMenu		= ::GetSubMenu(g_hMenu, 0);
		g_hListMenu		= ::CreateMenu();
		g_hConfigMenu	= ::GetSubMenu(g_hSubMenu, 1);
		if (g_hMenu == NULL || g_hSubMenu == NULL || g_hListMenu == NULL)
			return FALSE;

		GetMenuString(g_hSubMenu, ID_TMENU_ADD, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_LISTCONFIG", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hSubMenu, ID_TMENU_ADD, MF_BYCOMMAND, ID_TMENU_ADD, uimsg);
		GetMenuString(g_hSubMenu, 1, uitmp, sizeof(uitmp), MF_BYPOSITION);
		UTIL_get_lang_msg("MENU_SETTING", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hSubMenu, 1, MF_BYPOSITION, 1, uimsg);
		GetMenuString(g_hSubMenu, ID_VERSION, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_VERSION", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hSubMenu, ID_VERSION, MF_BYCOMMAND, ID_VERSION, uimsg);
		GetMenuString(g_hSubMenu, ID_EXEC, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_EXEC", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hSubMenu, ID_EXEC, MF_BYCOMMAND, ID_EXEC, uimsg);
		GetMenuString(g_hSubMenu, ID_TMENU_CLOSE, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_CLOSE", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hSubMenu, ID_TMENU_CLOSE, MF_BYCOMMAND, ID_TMENU_CLOSE, uimsg);
		GetMenuString(g_hConfigMenu, ID_ICON, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_ICON", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hConfigMenu, ID_ICON, MF_BYCOMMAND, ID_ICON, uimsg);
		GetMenuString(g_hConfigMenu, ID_FONT, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_FONT", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hConfigMenu, ID_FONT, MF_BYCOMMAND, ID_FONT, uimsg);
		GetMenuString(g_hConfigMenu, ID_LEFTPOPUP, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_LEFTPOPUP", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hConfigMenu, ID_LEFTPOPUP, MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
		GetMenuString(g_hConfigMenu, ID_HOTKEY, uitmp, sizeof(uitmp), MF_BYCOMMAND);
		UTIL_get_lang_msg("MENU_HOTKEY", uimsg, sizeof(uimsg), uitmp, UILanguageFile);
		ModifyMenu(g_hConfigMenu, ID_HOTKEY, MF_BYCOMMAND, ID_HOTKEY, uimsg);

		UTIL_get_lang_msg("MENU_EXEC", uimsg, sizeof(uimsg), "Execute", UILanguageFile);
		::ModifyMenu(g_hSubMenu, ID_EXEC, MF_BYCOMMAND | MF_POPUP, (UINT) g_hListMenu, (LPCTSTR) uimsg);
	}

	return TRUE;
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
	char	szPath[MAX_PATH];
	char	szEntryName[MAX_PATH];
	HKEY	hKey;
	DWORD	dwCnt;
	DWORD	dwIndex = 0;
	DWORD	dwSize = MAX_PATH;

	for (int cnt = 0; cnt < MAXJOBNUM; cnt++) {
		memset(g_MenuData.szName, 0, MAX_PATH);
		if (g_MenuData.hLargeIcon[cnt] != NULL) {
			::DestroyIcon(g_MenuData.hLargeIcon[cnt]);
			g_MenuData.hLargeIcon[cnt] = NULL;
		}
		if (g_MenuData.hSmallIcon[cnt] != NULL) {
			::DestroyIcon(g_MenuData.hSmallIcon[cnt]);
			g_MenuData.hSmallIcon[cnt] = NULL;
		}
	}

	if ((hKey = RegOpen(HKEY_CURRENT_USER, TTERM_KEY)) != INVALID_HANDLE_VALUE) {
		while (RegEnumEx(hKey, dwIndex, szEntryName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			::lstrcpy(g_MenuData.szName[dwIndex++], szEntryName);
			dwSize = MAX_PATH;
		}
		::lstrcpy(g_MenuData.szName[dwIndex], "");
		RegClose(hKey);

		for (dwCnt = 0; dwCnt < dwIndex; dwCnt++)
			if (GetApplicationFilename(g_MenuData.szName[dwCnt], szPath) == TRUE)
				ExtractAssociatedIconEx(szPath, &g_MenuData.hLargeIcon[dwCnt], &g_MenuData.hSmallIcon[dwCnt]);
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
	char		szPath[MAX_PATH];
	HDC			hDC;
	HWND		hWndItem;
	DWORD		itemNum;
	DWORD		desktopHeight;
	DWORD		dwCnt = 0;
	DWORD		dwValidCnt = 0;
	TEXTMETRIC	textMetric;
	char		uimsg[MAX_UIMSG];

	::DeleteMenu(g_hListMenu, ID_NOENTRY, MF_BYCOMMAND);
	num = ::GetMenuItemCount(g_hListMenu);
	for (dwCnt = 0; dwCnt < (DWORD) num; dwCnt++)
		if (::DeleteMenu(g_hListMenu, ID_MENU_MIN + dwCnt, MF_BYCOMMAND) == FALSE)
			num++;
	
	hWndItem	= ::GetDlgItem((HWND) g_hListMenu, ID_MENU_MIN);
	hDC			= ::GetWindowDC(hWndItem);
	if (g_MenuData.hFont != NULL)
		::SelectObject(hDC, (HGDIOBJ) g_MenuData.hFont);
	::GetTextMetrics(hDC, &textMetric);
	if (g_MenuData.dwIconMode == MODE_SMALLICON)
		g_MenuData.dwMenuHeight	= (ICONSPACE_SMALL > textMetric.tmHeight) ? ICONSPACE_SMALL : textMetric.tmHeight;
	else
		g_MenuData.dwMenuHeight	= (ICONSPACE_LARGE > textMetric.tmHeight) ? ICONSPACE_LARGE : textMetric.tmHeight;
	ReleaseDC(hWndItem, hDC);

	desktopHeight	= ::GetSystemMetrics(SM_CYSCREEN);
	itemNum			= desktopHeight / g_MenuData.dwMenuHeight;

	dwCnt = 0;
	while (::lstrlen(g_MenuData.szName[dwCnt]) != 0) {
		if (GetApplicationFilename(g_MenuData.szName[dwCnt], szPath) == TRUE) {
			if (dwCnt % itemNum == 0 && dwCnt != 0)
				::AppendMenu(g_hListMenu, MF_OWNERDRAW | MF_MENUBARBREAK, ID_MENU_MIN + dwCnt, (LPCTSTR) dwCnt);
			else
				::AppendMenu(g_hListMenu, MF_OWNERDRAW | MF_POPUP, ID_MENU_MIN + dwCnt, (LPCTSTR) dwCnt);
			dwValidCnt++;
		}
		dwCnt++;
	}
	if (dwValidCnt == 0) {
		UTIL_get_lang_msg("MENU_NOTRAY", uimsg, sizeof(uimsg), STR_NOENTRY, UILanguageFile);
		::AppendMenu(g_hListMenu, MF_STRING | MF_GRAYED, ID_NOENTRY, (LPCTSTR) uimsg);
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
	char	szSubKey[MAX_PATH];
	char	szEncodePassword[MAX_PATH];

	::wsprintf(szSubKey, "%s\\%s", TTERM_KEY, jobInfo->szName);
	if ((hKey = RegCreate(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	RegSetStr(hKey, KEY_HOSTNAME, jobInfo->szHostName);
	RegSetDword(hKey, KEY_MODE, jobInfo->dwMode);

	RegSetDword(hKey, KEY_USERFLAG, (DWORD) jobInfo->bUsername);
	RegSetStr(hKey, KEY_USERNAME, jobInfo->szUsername);
	RegSetDword(hKey, KEY_PASSWDFLAG, (DWORD) jobInfo->bPassword);
	EncodePassword(jobInfo->szPassword, szEncodePassword);
	RegSetBinary(hKey, KEY_PASSWORD, szEncodePassword, ::lstrlen(szEncodePassword) + 1);

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
	Arguments		: char			*szName		(In) 設定情報名
					: JobInfo		*jobInfo	(In) 設定情報構造体
	Return Value	: 成功 TRUE / 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL RegLoadLoginHostInformation(char *szName, JobInfo *job_Info)
{
	HKEY	hKey;
	char	szSubKey[MAX_PATH];
	char	szEncodePassword[MAX_PATH];
	DWORD	dwSize = MAX_PATH;
	JobInfo jobInfo;

	memset(&jobInfo, 0, sizeof(JobInfo));

	::wsprintf(szSubKey, "%s\\%s", TTERM_KEY, szName);
	if ((hKey = RegOpen(HKEY_CURRENT_USER, szSubKey)) == INVALID_HANDLE_VALUE)
		return FALSE;

	::lstrcpy(jobInfo.szName, szName);

	RegGetStr(hKey, KEY_HOSTNAME, jobInfo.szHostName, MAX_PATH);
	RegGetDword(hKey, KEY_MODE, &(jobInfo.dwMode));

	RegGetDword(hKey, KEY_USERFLAG, (DWORD *) &(jobInfo.bUsername));
	RegGetStr(hKey, KEY_USERNAME, jobInfo.szUsername, MAX_PATH);
	RegGetDword(hKey, KEY_PASSWDFLAG, (DWORD *) &(jobInfo.bPassword));
	RegGetBinary(hKey, KEY_PASSWORD, szEncodePassword, &dwSize);
	EncodePassword(szEncodePassword, jobInfo.szPassword);

	RegGetStr(hKey, KEY_TERATERM, jobInfo.szTeraTerm, MAX_PATH);
	RegGetStr(hKey, KEY_INITFILE, jobInfo.szInitFile, MAX_PATH);
	RegGetStr(hKey, KEY_OPTION, jobInfo.szOption, MAX_PATH);
	RegGetStr(hKey, KEY_LOGIN_PROMPT, jobInfo.szLoginPrompt, MAX_PATH);
	RegGetStr(hKey, KEY_PASSWORD_PROMPT, jobInfo.szPasswdPrompt, MAX_PATH);

	RegGetStr(hKey, KEY_MACROFILE, jobInfo.szMacroFile, MAX_PATH);

	RegGetDword(hKey, KEY_TTSSH, (LPDWORD) &(jobInfo.bTtssh));
	RegGetDword(hKey, KEY_STARTUP, (LPDWORD) &(jobInfo.bStartup));

	RegGetStr(hKey, KEY_LOG, jobInfo.szLog, MAX_PATH);

	// SSH2
	ZeroMemory(jobInfo.PrivateKeyFile, sizeof(jobInfo.PrivateKeyFile));
	RegGetStr(hKey, KEY_KEYFILE, jobInfo.PrivateKeyFile, MAX_PATH);
	RegGetDword(hKey, KEY_CHALLENGE, (LPDWORD) &(jobInfo.bChallenge));
	RegGetDword(hKey, KEY_PAGEANT, (LPDWORD) &(jobInfo.bPageant));

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
	::GetDlgItemText(hWnd, EDIT_TTMPATH, g_JobInfo.szTeraTerm, MAX_PATH);
	::GetDlgItemText(hWnd, EDIT_INITFILE, g_JobInfo.szInitFile, MAX_PATH);
	::GetDlgItemText(hWnd, EDIT_OPTION, g_JobInfo.szOption, MAX_PATH);
	::GetDlgItemText(hWnd, EDIT_PROMPT_USER, g_JobInfo.szLoginPrompt, MAX_PATH);
	::GetDlgItemText(hWnd, EDIT_PROMPT_PASS, g_JobInfo.szPasswdPrompt, MAX_PATH);

	::GetDlgItemText(hWnd, EDIT_LOG, g_JobInfo.szLog, MAX_PATH);

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
	long	index;
	char	szDefault[MAX_PATH] = DEFAULT_PATH;
	char	szTTermPath[MAX_PATH];
	char	szName[MAX_PATH];
	DWORD	dwErr;
	char	uimsg[MAX_UIMSG];
	char	cur[MAX_PATH], modulePath[MAX_PATH];

	if (::GetDlgItemText(hWnd, EDIT_ENTRY, g_JobInfo.szName, MAX_PATH) == 0) {
		UTIL_get_lang_msg("MSG_ERROR_NOREGNAME", uimsg, sizeof(uimsg),
		                  "error: no registry name", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}
	if (_tcschr(g_JobInfo.szName, '\\') != NULL) {
		UTIL_get_lang_msg("MSG_ERROR_INVALIDREGNAME", uimsg, sizeof(uimsg),
		                  "can't use \"\\\" in registry name", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	if (::IsDlgButtonChecked(hWnd, RADIO_LOGIN) == 1)
		g_JobInfo.dwMode = MODE_AUTOLOGIN;
	if (::IsDlgButtonChecked(hWnd, RADIO_MACRO) == 1)
		g_JobInfo.dwMode = MODE_MACRO;
	if (::IsDlgButtonChecked(hWnd, RADIO_DIRECT) == 1)
		g_JobInfo.dwMode = MODE_DIRECT;

	if (::GetDlgItemText(hWnd, EDIT_HOST, g_JobInfo.szHostName, MAX_PATH) == 0 && g_JobInfo.dwMode == MODE_AUTOLOGIN) {
		UTIL_get_lang_msg("MSG_ERROR_NOHOST", uimsg, sizeof(uimsg),
		                  "error: no host name", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	g_JobInfo.bUsername	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_USER);
	::GetDlgItemText(hWnd, EDIT_USER, g_JobInfo.szUsername, MAX_PATH);

	g_JobInfo.bPassword	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_PASSWORD);
	::GetDlgItemText(hWnd, EDIT_PASSWORD, g_JobInfo.szPassword, MAX_PATH);

	if (::GetDlgItemText(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile, MAX_PATH) == 0 && g_JobInfo.dwMode == MODE_MACRO) {
		UTIL_get_lang_msg("MSG_ERROR_NOMACRO", uimsg, sizeof(uimsg),
		                  "error: no macro filename", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	g_JobInfo.bStartup	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_STARTUP);

	g_JobInfo.bTtssh	= (BOOL) ::IsDlgButtonChecked(hWnd, CHECK_TTSSH);
	if (g_JobInfo.bTtssh == TRUE && lstrstri(g_JobInfo.szTeraTerm, TTSSH) == NULL) {
		::GetProfileString("Tera Term Pro", "Path", szDefault, szTTermPath, MAX_PATH);
		::wsprintf(g_JobInfo.szTeraTerm, "%s\\%s", szTTermPath, TTSSH);
	} else if (::lstrlen(g_JobInfo.szTeraTerm) == 0) {
		::GetProfileString("Tera Term Pro", "Path", szDefault, szTTermPath, MAX_PATH);
		::wsprintf(g_JobInfo.szTeraTerm, "%s\\%s", szTTermPath, g_JobInfo.bTtssh ? TTSSH : TERATERM);
	}
	
	GetCurrentDirectory(sizeof(cur), cur);
	GetModuleFileName(NULL, modulePath, sizeof(modulePath));
	ExtractDirName(modulePath, modulePath);
	SetCurrentDirectory(modulePath);
	if (::GetFileAttributes(g_JobInfo.szTeraTerm) == 0xFFFFFFFF) {
		dwErr = ::GetLastError();
		if (dwErr == ERROR_FILE_NOT_FOUND || dwErr == ERROR_PATH_NOT_FOUND) {
			UTIL_get_lang_msg("MSG_ERROR_CHECKFILE", uimsg, sizeof(uimsg),
			                  "checking [%s] file was failure.\n", UILanguageFile);
			ErrorMessage(hWnd, dwErr, uimsg, g_JobInfo.szTeraTerm);
			SetCurrentDirectory(cur);
			return FALSE;
		}
	}

	// 秘密鍵ファイルの追加 (2005.1.28 yutaka)
	if (::GetDlgItemText(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile, MAX_PATH) == 0) {
		ZeroMemory(g_JobInfo.PrivateKeyFile, sizeof(g_JobInfo.PrivateKeyFile));
	}
	if (g_JobInfo.bTtssh) {
		g_JobInfo.bChallenge = (BOOL) ::IsDlgButtonChecked(hWnd, IDC_CHALLENGE_CHECK);
		g_JobInfo.bPageant = (BOOL) ::IsDlgButtonChecked(hWnd, IDC_PAGEANT_CHECK);
	}

	if (RegSaveLoginHostInformation(&g_JobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msg("MSG_ERROR_SAVEREG", uimsg, sizeof(uimsg),
		                  "error: couldn't save to registory.\n", UILanguageFile);
		ErrorMessage(hWnd, dwErr, uimsg);
		SetCurrentDirectory(cur);
		return FALSE;
	}

	InitListMenu(hWnd);
	InitListBox(hWnd);

	index = 0;
	while ((index = ::SendDlgItemMessage(hWnd, LIST_HOST, LB_SELECTSTRING, index, (LPARAM)(LPCTSTR) g_JobInfo.szName)) != LB_ERR) {
		::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETTEXT, index, (LPARAM)(LPCTSTR) szName);
		if (::lstrcmpi(g_JobInfo.szName, szName) == 0)
			break;
	}

	SetCurrentDirectory(cur);

	return TRUE;
}

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
	long	index;
//	char	*pt;
	char	szName[MAX_PATH];
	char	uimsg[MAX_UIMSG];
	DWORD	dwErr;

	index = ::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETCURSEL, 0, 0);
	::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETTEXT, (WPARAM) index, (LPARAM) (LPCTSTR) szName);

	if (RegLoadLoginHostInformation(szName, &g_JobInfo) == FALSE) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msg("MSG_ERROR_OPENREG", uimsg, sizeof(uimsg),
		                  "Couldn't open the registory.\n", UILanguageFile);
		ErrorMessage(hWnd, dwErr, uimsg);
		return FALSE;
	}

	switch (g_JobInfo.dwMode) {
	case MODE_AUTOLOGIN:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_LOGIN);
		EnableItem(hWnd, EDIT_HOST, TRUE);
		EnableItem(hWnd, CHECK_USER, TRUE);
		EnableItem(hWnd, EDIT_USER, g_JobInfo.bUsername);
		EnableItem(hWnd, CHECK_PASSWORD, TRUE);
		EnableItem(hWnd, EDIT_PASSWORD, g_JobInfo.bPassword);
		EnableItem(hWnd, EDIT_MACRO, FALSE);
		EnableItem(hWnd, BUTTON_MACRO, FALSE);
		break;
	case MODE_MACRO:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_MACRO);
		EnableItem(hWnd, EDIT_HOST, FALSE);
		EnableItem(hWnd, CHECK_USER, FALSE);
		EnableItem(hWnd, EDIT_USER, FALSE);
		EnableItem(hWnd, CHECK_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_MACRO, TRUE);
		EnableItem(hWnd, BUTTON_MACRO, TRUE);
		break;
	case MODE_DIRECT:
		::CheckRadioButton(hWnd, RADIO_LOGIN, RADIO_DIRECT, RADIO_DIRECT);
		EnableItem(hWnd, EDIT_HOST, FALSE);
		EnableItem(hWnd, CHECK_USER, FALSE);
		EnableItem(hWnd, EDIT_USER, FALSE);
		EnableItem(hWnd, CHECK_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_MACRO, FALSE);
		EnableItem(hWnd, BUTTON_MACRO, FALSE);
		break;
	}

	if (::lstrlen(g_JobInfo.szName) == 0)
		::lstrcpy(g_JobInfo.szName, g_JobInfo.szHostName);

	::SetDlgItemText(hWnd, EDIT_ENTRY, g_JobInfo.szName);
	::SetDlgItemText(hWnd, EDIT_HOST, g_JobInfo.szHostName);
	::SetDlgItemText(hWnd, EDIT_USER, g_JobInfo.szUsername);
	::SetDlgItemText(hWnd, EDIT_PASSWORD, g_JobInfo.szPassword);

	::SetDlgItemText(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile);

	::CheckDlgButton(hWnd, CHECK_USER, g_JobInfo.bUsername);

	::CheckDlgButton(hWnd, CHECK_PASSWORD, g_JobInfo.bPassword);

	::CheckDlgButton(hWnd, CHECK_TTSSH, g_JobInfo.bTtssh);

	// 秘密鍵ファイルの追加 (2005.1.28 yutaka)
	::SetDlgItemText(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile);
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

	// ttssh.exeは廃止したので下記チェックは削除する。(2004.12.3 yutaka)
#if 0
	if ((pt = lstrstri(g_JobInfo.szTeraTerm, TTSSH)) != NULL)
		if (::lstrcmpi(pt, TTSSH) == 0)
			::CheckDlgButton(hWnd, CHECK_TTSSH, TRUE);
#endif

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
	long	index;
	char	szEntryName[MAX_PATH];
	char	szSubKey[MAX_PATH];
	char	uimsg[MAX_UIMSG];
	DWORD	dwErr;

	if ((index = ::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETCURSEL, 0, 0)) == LB_ERR) {
		UTIL_get_lang_msg("MSG_ERROR_SELECTREG", uimsg, sizeof(uimsg),
		                  "Select deleted registry name", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	if (::SendDlgItemMessage(hWnd, LIST_HOST, LB_GETTEXT, (WPARAM) index, (LPARAM) (LPCTSTR) szEntryName) == LB_ERR) {
		UTIL_get_lang_msg("MSG_ERROR_GETDELETEREG", uimsg, sizeof(uimsg),
		                  "Couldn't get the deleting entry", UILanguageFile);
		::MessageBox(hWnd, uimsg, "TeraTerm Menu", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	::wsprintf(szSubKey, "%s\\%s", TTERM_KEY, szEntryName);
	if (RegDelete(HKEY_CURRENT_USER, szSubKey) != ERROR_SUCCESS) {
		dwErr = ::GetLastError();
		UTIL_get_lang_msg("MSG_ERROR_DELETEREG", uimsg, sizeof(uimsg),
		                  "Couldn't delete the registory.\n", UILanguageFile);
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
	char *pt;
	int ret = 0;
	char title[MAX_UIMSG], filter[MAX_UIMSG];

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
		g_hWndMenu = NULL;
		::EndDialog(hWnd, TRUE);
		if (g_ConfigFont != NULL) {
			DeleteObject(g_ConfigFont);
		}
		return TRUE;
	case BUTTON_SET:
		SaveLoginHostInformation(hWnd);
		return TRUE;
	case BUTTON_DELETE:
		DeleteLoginHostInformation(hWnd);
		return TRUE;
	case BUTTON_ETC:
		::GetDlgItemText(hWnd, EDIT_ENTRY, g_JobInfo.szName, MAX_PATH);
		g_JobInfo.bTtssh	= ::IsDlgButtonChecked(hWnd, CHECK_TTSSH);
		if (::DialogBox(g_hI, (LPCTSTR) DIALOG_ETC, hWnd, DlgCallBack_Etc) == TRUE) {
			::CheckDlgButton(hWnd, CHECK_TTSSH, 0);
			if ((pt = lstrstri(g_JobInfo.szTeraTerm, TTSSH)) != NULL)
				if (::lstrcmpi(pt, TTSSH) == 0)
					::CheckDlgButton(hWnd, CHECK_TTSSH, 1);
		}
		return TRUE;
	case LIST_HOST:
		if (HIWORD(wParam) == LBN_SELCHANGE)
			LoadLoginHostInformation(hWnd);
		return TRUE;
	case CHECK_USER:
		if (IsDlgButtonChecked(hWnd, CHECK_USER) == 1)
			EnableItem(hWnd, EDIT_USER, TRUE);
		else {
			EnableItem(hWnd, EDIT_USER, FALSE);
			::CheckDlgButton(hWnd, CHECK_PASSWORD, 0);
			::PostMessage(hWnd, WM_COMMAND, (WPARAM) CHECK_PASSWORD, (LPARAM) 0);
		}
		return TRUE;
	case CHECK_PASSWORD:
		if (IsDlgButtonChecked(hWnd, CHECK_PASSWORD) == 1)
			EnableItem(hWnd, EDIT_PASSWORD, TRUE);
		else
			EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		return TRUE;
	case CHECK_INI_FILE:
		if (IsDlgButtonChecked(hWnd, CHECK_INI_FILE) == 1)
			EnableItem(hWnd, COMBO_INI_FILE, TRUE);
		else
			EnableItem(hWnd, COMBO_INI_FILE, FALSE);
		return TRUE;
	case BUTTON_MACRO:
		::GetDlgItemText(hWnd, EDIT_MACRO, g_JobInfo.szMacroFile, MAX_PATH);
		UTIL_get_lang_msg("FILEDLG_MACRO_TITLE", title, sizeof(title),
		                  "specifying macro file", UILanguageFile);
		UTIL_get_lang_msg("FILEDLG_MACRO_FILTER", filter, sizeof(filter),
		                  "macro file(*.ttl)\\0*.ttl\\0all files(*.*)\\0*.*\\0\\0", UILanguageFile);
		OpenFileDlg(hWnd, EDIT_MACRO, title, filter, g_JobInfo.szMacroFile);
		return TRUE;

	case IDC_KEYFILE_BUTTON:
		::GetDlgItemText(hWnd, IDC_KEYFILE_PATH, g_JobInfo.PrivateKeyFile, MAX_PATH);
		UTIL_get_lang_msg("FILEDLG_KEY_TITLE", title, sizeof(title),
		                  "specifying private key file", UILanguageFile);
		UTIL_get_lang_msg("FILEDLG_KEY_FILTER", filter, sizeof(filter),
		                  "identity files\\0identity;id_rsa;id_dsa\\0identity(RSA1)\\0identity\\0id_rsa(SSH2)\\0id_rsa\\0id_dsa(SSH2)\\0id_dsa\\0all(*.*)\\0*.*\\0\\0", UILanguageFile);
		OpenFileDlg(hWnd, IDC_KEYFILE_PATH, title, filter, g_JobInfo.PrivateKeyFile);
		return TRUE;

	case RADIO_LOGIN:
		EnableItem(hWnd, EDIT_HOST, TRUE);
		EnableItem(hWnd, CHECK_USER, TRUE);
		if (IsDlgButtonChecked(hWnd, CHECK_USER) == 1)
			EnableItem(hWnd, EDIT_USER, TRUE);
		else {
			EnableItem(hWnd, EDIT_USER, FALSE);
		}
		EnableItem(hWnd, CHECK_PASSWORD, TRUE);
		if (IsDlgButtonChecked(hWnd, CHECK_PASSWORD) == 1)
			EnableItem(hWnd, EDIT_PASSWORD, TRUE);
		else
			EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_MACRO, FALSE);
		EnableItem(hWnd, BUTTON_MACRO, FALSE);
		return TRUE;
	case RADIO_MACRO:
		EnableItem(hWnd, EDIT_HOST, FALSE);
		EnableItem(hWnd, CHECK_USER, FALSE);
		EnableItem(hWnd, EDIT_USER, FALSE);
		EnableItem(hWnd, CHECK_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_MACRO, TRUE);
		EnableItem(hWnd, BUTTON_MACRO, TRUE);
		return TRUE;
	case RADIO_DIRECT:
		EnableItem(hWnd, EDIT_HOST, FALSE);
		EnableItem(hWnd, CHECK_USER, FALSE);
		EnableItem(hWnd, EDIT_USER, FALSE);
		EnableItem(hWnd, CHECK_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_PASSWORD, FALSE);
		EnableItem(hWnd, EDIT_MACRO, FALSE);
		EnableItem(hWnd, BUTTON_MACRO, FALSE);
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
	char	szPath[MAX_PATH];
	char	title[MAX_UIMSG], filter[MAX_UIMSG];

	switch(LOWORD(wParam)) {
	case IDOK:
		SaveEtcInformation(hWnd);
		::EndDialog(hWnd, TRUE);
		if (g_DetailFont != NULL) {
			DeleteObject(g_DetailFont);
		}
		return TRUE;
	case IDCANCEL:
		::EndDialog(hWnd, FALSE);
		if (g_DetailFont != NULL) {
			DeleteObject(g_DetailFont);
		}
		return TRUE;
	case BUTTON_DEFAULT:
		SetDefaultEtcDlg(hWnd);
		return TRUE;
	case BUTTON_TTMPATH:
		::GetDlgItemText(hWnd, EDIT_TTMPATH, szPath, MAX_PATH);
		UTIL_get_lang_msg("FILEDLG_TERATERM_TITLE", title, sizeof(title),
		                  "specifying TeraTerm", UILanguageFile);
		UTIL_get_lang_msg("FILEDLG_TERATERM_FILTER", filter, sizeof(filter),
		                  "execute file(*.exe)\\0*.exe\\0all files(*.*)\\0*.*\\0\\0", UILanguageFile);
		OpenFileDlg(hWnd, EDIT_TTMPATH, title, filter, szPath);
		return TRUE;
	case BUTTON_INITFILE:
		::GetDlgItemText(hWnd, EDIT_INITFILE, szPath, MAX_PATH);
		UTIL_get_lang_msg("FILEDLG_INI_TITLE", title, sizeof(title),
		                  "specifying config file", UILanguageFile);
		UTIL_get_lang_msg("FILEDLG_INI_FILTER", filter, sizeof(filter),
		                  "config file(*.ini)\\0*.ini\\0all files(*.*)\\0*.*\\0\\0", UILanguageFile);
		OpenFileDlg(hWnd, EDIT_INITFILE, title, filter, szPath);
		return TRUE;
	case BUTTON_LOG:
		::GetDlgItemText(hWnd, EDIT_LOG, szPath, MAX_PATH);
		UTIL_get_lang_msg("FILEDLG_LOG_TITLE", title, sizeof(title),
		                  "specifying log file", UILanguageFile);
		UTIL_get_lang_msg("FILEDLG_LOG_FILTER", filter, sizeof(filter),
		                  "log file(*.log)\\0*.log\\0all files(*.*)\\0*.*\\0\\0", UILanguageFile);
		OpenFileDlg(hWnd, EDIT_LOG, title, filter, szPath);
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
		if (g_AboutFont != NULL) {
			DeleteObject(g_AboutFont);
		}
		return TRUE;
	case IDCANCEL:
		::EndDialog(hWnd, TRUE);
		if (g_AboutFont != NULL) {
			DeleteObject(g_AboutFont);
		}
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
	char	uimsg[MAX_UIMSG];

	switch(LOWORD(wParam)) {
	case ID_TMENU_ADD:
		::DialogBox(g_hI, (LPCTSTR) DIALOG_CONFIG, 0, DlgCallBack_Config);
		return TRUE;
	case ID_TMENU_CLOSE:
		::DestroyWindow(hWnd);
		return	TRUE;
	case ID_VERSION:
		::DialogBox(g_hI, (LPCTSTR) DIALOG_VERSION, hWnd, DlgCallBack_Version);
		return TRUE;
	case ID_ICON:
		if (GetMenuState(g_hConfigMenu, ID_ICON, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msg("MENU_ICON", uimsg, sizeof(uimsg), STR_ICONMODE, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_ICON, MF_BYCOMMAND, ID_ICON, uimsg);
			g_MenuData.dwIconMode = MODE_SMALLICON;
		} else {
			UTIL_get_lang_msg("MENU_ICON", uimsg, sizeof(uimsg), STR_ICONMODE, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_ICON, MF_CHECKED | MF_BYCOMMAND, ID_ICON, uimsg);
			g_MenuData.dwIconMode = MODE_LARGEICON;
		}
		RedrawMenu(hWnd);
		return	TRUE;
	case ID_LEFTPOPUP:
		if (GetMenuState(g_hConfigMenu, ID_LEFTPOPUP, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msg("MENU_LEFTPOPUP", uimsg, sizeof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_LEFTPOPUP, MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
			g_MenuData.bLeftButtonPopup = FALSE;
		} else {
			UTIL_get_lang_msg("MENU_LEFTPOPUP", uimsg, sizeof(uimsg), STR_LEFTBUTTONPOPUP, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_LEFTPOPUP, MF_CHECKED | MF_BYCOMMAND, ID_LEFTPOPUP, uimsg);
			g_MenuData.bLeftButtonPopup = TRUE;
		}
		return	TRUE;
	case ID_HOTKEY:
		if (GetMenuState(g_hConfigMenu, ID_HOTKEY, MF_BYCOMMAND & MF_CHECKED) != 0) {
			UTIL_get_lang_msg("MENU_HOTKEY", uimsg, sizeof(uimsg), STR_HOTKEY, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_HOTKEY, MF_BYCOMMAND, ID_HOTKEY, uimsg);
			::UnregisterHotKey(g_hWnd, WM_MENUOPEN);
			g_MenuData.bHotkey = FALSE;
		} else {
			UTIL_get_lang_msg("MENU_HOTKEY", uimsg, sizeof(uimsg), STR_HOTKEY, UILanguageFile);
			::ModifyMenu(g_hConfigMenu, ID_HOTKEY, MF_CHECKED | MF_BYCOMMAND, ID_HOTKEY, uimsg);
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
BOOL CALLBACK DlgCallBack_Config(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		::SetClassLong(hWnd, GCL_HICON, (LONG) g_hIcon);
		CreateTooltip();
		crText		= ::GetSysColor(COLOR_WINDOWTEXT);
		crBkgnd		= ::GetSysColor(COLOR_WINDOW);
		crSelText	= ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		crSelBkgnd	= ::GetSysColor(COLOR_HIGHLIGHT);
		InitConfigDlg(hWnd);
		return TRUE;
	case WM_COMMAND:
		return ManageWMCommand_Config(hWnd, wParam);
	case WM_NOTIFY:
		return ManageWMNotify_Config(lParam);
	case WM_DESTROY:
		::UnhookWindowsHookEx(g_hHook);
		return TRUE;
	case WM_MEASUREITEM:
		lpmis = (LPMEASUREITEMSTRUCT) lParam;
		lpmis->itemHeight = LISTBOX_HEIGHT;
		return TRUE;
	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT) lParam;
		if (lpdis->itemID == -1)
			return TRUE;
		if (lpdis->itemState & ODS_SELECTED) {
			::SetTextColor(lpdis->hDC, crSelText);
			::SetBkColor(lpdis->hDC, crSelBkgnd);
		} else {
			::SetTextColor(lpdis->hDC, crText);
			::SetBkColor(lpdis->hDC, crBkgnd);
		}
		::GetTextMetrics(lpdis->hDC, &textMetric);
		::ExtTextOut(lpdis->hDC,
					lpdis->rcItem.left + LISTBOX_WIDTH,
					lpdis->rcItem.top + (ICONSIZE_SMALL - textMetric.tmHeight) / 2,
					ETO_OPAQUE,
					&lpdis->rcItem,
					g_MenuData.szName[lpdis->itemData],
					::lstrlen(g_MenuData.szName[lpdis->itemData]),
					NULL);
		::DrawIconEx(lpdis->hDC,
					lpdis->rcItem.left + (LISTBOX_WIDTH - ICONSIZE_SMALL) / 2,
					lpdis->rcItem.top + (LISTBOX_HEIGHT - ICONSIZE_SMALL) / 2,
					g_MenuData.hSmallIcon[lpdis->itemData],
					ICONSIZE_SMALL,
					ICONSIZE_SMALL,
					NULL,
					NULL,
					DI_NORMAL);
		return TRUE;
	}

	return FALSE;
}

/* ==========================================================================
	Function Name	: (BOOL CALLBACK) DlgCallBack_Config()
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
BOOL CALLBACK DlgCallBack_Etc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	Function Name	: (BOOL CALLBACK) DlgCallBack_Config()
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
BOOL CALLBACK DlgCallBack_Version(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_INITDIALOG:
		SetDlgPos(hWnd, POSITION_CENTER);
		::SetClassLong(hWnd, GCL_HICON, (LONG) g_hIcon);
		InitVersionDlg(hWnd);
		return TRUE;
	case WM_COMMAND:
		return ManageWMCommand_Version(hWnd, wParam);
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

	g_hWnd	= hWnd;

	switch(uMsg) {
	case WM_CREATE:
		::SetClassLong(hWnd, GCL_HICON, (LONG) g_hIcon);
		SetDlgPos(hWnd, POSITION_CENTER);
		::ShowWindow(hWnd, SW_HIDE);
		SetTaskTray(hWnd, NIM_ADD);
		WM_TASKBAR_RESTART = ::RegisterWindowMessage("TaskbarCreated");
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
	case WM_DESTROY:
		SaveConfig();
		SetTaskTray(hWnd, NIM_DELETE);
		::UnregisterHotKey(hWnd, WM_MENUOPEN);
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
		::GetTextExtentPoint32(hDC, g_MenuData.szName[lpmis->itemData], ::lstrlen(g_MenuData.szName[lpmis->itemData]), &size);
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
		if (lpdis->itemID == -1)
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
		::ExtTextOut(lpdis->hDC,
					lpdis->rcItem.left + dwIconSpace,
					lpdis->rcItem.top + (g_MenuData.dwMenuHeight - textMetric.tmHeight) / 2,
					ETO_OPAQUE,
					&lpdis->rcItem,
					g_MenuData.szName[lpdis->itemData],
					::lstrlen(g_MenuData.szName[lpdis->itemData]),
					NULL);
		::DrawIconEx(lpdis->hDC,
					lpdis->rcItem.left + (dwIconSpace - dwIconSize) / 2,
					lpdis->rcItem.top + (g_MenuData.dwMenuHeight - dwIconSize) / 2,
					(g_MenuData.dwIconMode == MODE_LARGEICON) ? g_MenuData.hLargeIcon[lpdis->itemData] : g_MenuData.hSmallIcon[lpdis->itemData],
					dwIconSize,
					dwIconSize,
					NULL,
					NULL,
					DI_NORMAL);
		return TRUE;
	}

	if (WM_TASKBAR_RESTART != 0 && uMsg == WM_TASKBAR_RESTART)
		SetTaskTray(hWnd, NIM_ADD);

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
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
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR nCmdLine, int nCmdShow)
{
	MSG			msg;
	HWND		hWnd;
	WNDCLASS	winClass;
	char		uimsg[MAX_UIMSG];
	DWORD		dwErr;

	// インストーラで実行を検出するために mutex を作成する (2006.8.12 maya)
	// 2重起動防止のためではないので、特に返り値は見ない
	HANDLE hMutex;
	hMutex = CreateMutex(NULL, TRUE, "TeraTermMenuAppMutex");

	checkIniFile();		//INIファイル/レジストリ切替

	GetUILanguageFile(UILanguageFile, sizeof(UILanguageFile));

	g_hI	= hI;
	g_hIcon	= ::LoadIcon(g_hI, (LPCSTR) TTERM_ICON);

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

	if (::FindWindow(TTPMENU_CLASS, NULL) == NULL) {
		if (::RegisterClass(&winClass) == 0) {
			dwErr = ::GetLastError();
			UTIL_get_lang_msg("MSG_ERROR_WINCLASS", uimsg, sizeof(uimsg),
			                  "Couldn't register the window class.\n", UILanguageFile);
			ErrorMessage(NULL, dwErr, uimsg);
			return FALSE;
		}
	}
	
	hWnd	= ::CreateWindowEx(0,
							TTPMENU_CLASS,
							"Main Window",
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

	while (::GetMessage(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return TRUE;
}
