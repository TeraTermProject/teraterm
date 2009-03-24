#ifndef TTPMENU_H
#define TTPMENU_H
/* ========================================================================
	Project  Name		: TeraTerm Menu
	Outline				: TeraTerm Menu Header
	Version				: 0.94
	Create				: 1998-11-22(Sun)
	Update				: 2001-11-01(Thu)
    Reference			: Copyright (C) S.Hayakawa 1997-2001
	======================================================================== */

#include	<windows.h>

// 各種定数
#define		WM_TMENU_NOTIFY			(WM_USER + 101)
#define		WM_MENUOPEN				(WM_USER + 102)
#define		ID_NOENTRY				49999
#define		ID_MENU_MIN				50000
#define		MAXJOBNUM				1024
#define		ICONSIZE_LARGE			32
#define		ICONSIZE_SMALL			16
#define		ICONSPACE_LARGE			40
#define		ICONSPACE_SMALL			24
#define		LISTBOX_HEIGHT			18
#define		LISTBOX_WIDTH			20
#define		TERATERM				"ttermpro.exe"
//#define		TTSSH					"ttssh.exe"
// ttssh.exeはUTF-8 TeraTermでは不要。TeraTerm本体と同じでよい。(2004.12.2 yutaka)
#define		TTSSH					TERATERM
#define		TTERM_KEY				"Software\\ShinpeiTools\\TTermMenu"
#define		LOGIN_PROMPT			"login:"
#define		PASSWORD_PROMPT			"Password:"
#define		TTPMENU_CLASS			"TMenuClass"

#define		DATA_NOENTRY			0xffffffff

#define		MODE_SMALLICON			0x0000
#define		MODE_LARGEICON			0x0001

#define		MODE_AUTOLOGIN			0x0000
#define		MODE_MACRO				0x0001
#define		MODE_DIRECT				0x0002

// レジストリの値名（一般設定）
#define		KEY_ICONMODE			"IconMode"
#define		KEY_LEFTBUTTONPOPUP		"LeftButtonPopup"
#define		KEY_MENUTEXTCOLOR		"MenuTextColor"
#define		KEY_LF_HEIGHT			"lfHeight"
#define		KEY_LF_WIDTH			"lfWidth"
#define		KEY_LF_ESCAPEMENT		"lfEscapement"
#define		KEY_LF_ORIENTATION		"lfOrientation"
#define		KEY_LF_WEIGHT			"lfWeight"
#define		KEY_LF_ITALIC			"lfItalic"
#define		KEY_LF_UNDERLINE		"lfUnderline"
#define		KEY_LF_STRIKEOUT		"lfStrikeOut"
#define		KEY_LF_CHARSET			"lfCharSet"
#define		KEY_LF_OUTPRECISION		"lfOutPrecision"
#define		KEY_LF_CLIPPRECISION	"lfClipPrecision"
#define		KEY_LF_QUALITY			"lfQuality"
#define		KEY_LF_PITCHANDFAMILY	"lfPitchAndFamily"
#define		KEY_LF_FACENAME			"lfFaceName"
#define		KEY_HOTKEY				"Hotkey"

// レジストリの値名（ジョブ設定）
#define		KEY_MODE				"Mode"
#define		KEY_HOSTNAME			"HostName"
#define		KEY_USERFLAG			"UserFlag"
#define		KEY_USERNAME			"UserName"
#define		KEY_PASSWDFLAG			"PasswdFlag"
#define		KEY_PASSWORD			"Password"
#define		KEY_INITFILE			"INI_File"
#define		KEY_TERATERM			"TeraTerm"
#define		KEY_OPTION				"Option"
#define		KEY_LOGIN_PROMPT		"LoginPrompt"
#define		KEY_PASSWORD_PROMPT		"PasswdPrompt"
#define		KEY_MACROFILE			"MacroFile"
#define		KEY_TTSSH				"TeraTerm Mode"
#define		KEY_LOG					"Log"
#define		KEY_STARTUP				"Startup"
#define		KEY_KEYFILE				"PrivateKeyFile"  // add (2005.1.27 yutaka)
#define		KEY_CHALLENGE			"Challenge"       // add (2007.11.14 yutaka)
#define		KEY_PAGEANT				"Pageant"         // add (2008.5.26 maya)

#define		STR_ICONMODE			"showing large icon"
#define		STR_LEFTBUTTONPOPUP		"showing list by left-click"
#define		STR_HOTKEY				"showing list hotkey(Ctl+Alt+M)"
#define		STR_NOENTRY				"(none)"

// 設定情報構造体
struct JobInfo {
	char	szName[MAX_PATH];			// ジョブ名
	BOOL	bStartup;					// 起動時にジョブを実行するかどうかのフラグ
	BOOL	bTtssh;						// ttsshを使用するかどうかのフラグ
	DWORD	dwMode;						// ジョブの種類

	// 自動ログイン用設定
	char	szHostName[MAX_PATH];		// ホスト名
	BOOL	bUsername;					// ユーザ名を入力するかどうかのフラグ
	char	szUsername[MAX_PATH];		// ユーザ名
	BOOL	bPassword;					// パスワードを入力するかどうかのフラグ
	char	szPassword[MAX_PATH];		// パスワード

	// マクロ実行用設定
	char	szMacroFile[MAX_PATH];		// 実行するマクロファイルのファイル名

	// 詳細設定
	char	szTeraTerm[MAX_PATH];		// 起動アプリ（TeraTerm）のファイル名
	char	szInitFile[MAX_PATH];		// TeraTermの設定ファイル（起動のみ意外）
	char	szOption[MAX_PATH];			// アプリケーションのオプション/引数
	char	szLog[MAX_PATH];			// ログファイル名（自動ログインのみ）
	char	szLoginPrompt[MAX_PATH];	// ログインプロンプト（自動ログインのみ）
	char	szPasswdPrompt[MAX_PATH];	// パスワードプロンプト（自動ログインのみ）

	char    PrivateKeyFile[MAX_PATH];   // 秘密鍵ファイル (2005.1.27 yutaka)
	BOOL    bChallenge;                 // keyboard-interative method(/challenge)
	BOOL    bPageant;                   // use Pageant(/pageant)
};

// 表示設定構造体
struct MenuData {
	TCHAR		szName[MAXJOBNUM][MAX_PATH];
	HICON		hLargeIcon[MAXJOBNUM];
	HICON		hSmallIcon[MAXJOBNUM];
	DWORD		dwMenuHeight;
	DWORD		dwIconMode;
	BOOL		bLeftButtonPopup;
	BOOL		bHotkey;
	HFONT		hFont;
	LOGFONT		lfFont;
	COLORREF	crMenuBg;
	COLORREF	crMenuTxt;
	COLORREF	crSelMenuBg;
	COLORREF	crSelMenuTxt;
};

// 関数一覧
void	PopupMenu(HWND hWnd);
void	PopupListMenu(HWND hWnd);
BOOL	AddTooltip(int idControl);
BOOL	ConnectHost(HWND hWnd, UINT idItem, char *szJobName = NULL);
BOOL	CreateTooltip(void);
BOOL	DeleteLoginHostInformation(HWND hWnd);
BOOL	ErrorMessage(HWND hWnd, LPTSTR msg,...);
BOOL	ExtractAssociatedIconEx(char *szPath, HICON *hLargeIcon, HICON *hSmallIcon);
BOOL	ExecStartup(HWND hWnd);
BOOL	GetApplicationFilename(char *szName, char *szPath);
BOOL	InitConfigDlg(HWND hWnd);
BOOL	InitEtcDlg(HWND hWnd);
BOOL	InitListMenu(HWND hWnd);
BOOL	InitMenu(void);
BOOL	InitVersionDlg(HWND hWnd);
BOOL	LoadConfig(void);
BOOL	LoadLoginHostInformation(HWND hWnd);
BOOL	MakeTTL(char *TTLName, JobInfo *jobInfo);
BOOL	ManageWMCommand_Config(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Etc(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Menu(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Version(HWND hWnd, WPARAM wParam);
BOOL	ManageWMNotify_Config(LPARAM lParam);
BOOL	RedrawMenu(HWND hWnd);
BOOL	RegLoadLoginHostInformation(char *szName, JobInfo *jobInfo);
BOOL	RegSaveLoginHostInformation(JobInfo *jobInfo);
BOOL	SaveConfig(void);
BOOL	SaveEtcInformation(HWND hWnd);
BOOL	SaveLoginHostInformation(HWND hWnd);
BOOL	SetDefaultEtcDlg(HWND hWnd);
BOOL	SetMenuFont(HWND hWnd);
BOOL	SetTaskTray(HWND hWnd, DWORD dwMessage);
BOOL	CALLBACK DlgCallBack_Config(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL	CALLBACK DlgCallBack_Etc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL	CALLBACK DlgCallBack_Version(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT	CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT	CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
