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
#define		TERATERM				L"ttermpro.exe"
#define		TTERM_KEY				L"Software\\ShinpeiTools\\TTermMenu"
#define		LOGIN_PROMPT			L"login:"
#define		PASSWORD_PROMPT			L"Password:"
#define		TTPMENU_CLASS			L"TMenuClass"

#define		DATA_NOENTRY			0xffffffff

#define		MODE_SMALLICON			0x0000
#define		MODE_LARGEICON			0x0001

// レジストリの値名（一般設定）
#define		KEY_ICONMODE			L"IconMode"
#define		KEY_LEFTBUTTONPOPUP		L"LeftButtonPopup"
#define		KEY_MENUTEXTCOLOR		L"MenuTextColor"
#define		KEY_LF_HEIGHT			L"lfHeight"
#define		KEY_LF_WIDTH			L"lfWidth"
#define		KEY_LF_ESCAPEMENT		L"lfEscapement"
#define		KEY_LF_ORIENTATION		L"lfOrientation"
#define		KEY_LF_WEIGHT			L"lfWeight"
#define		KEY_LF_ITALIC			L"lfItalic"
#define		KEY_LF_UNDERLINE		L"lfUnderline"
#define		KEY_LF_STRIKEOUT		L"lfStrikeOut"
#define		KEY_LF_CHARSET			L"lfCharSet"
#define		KEY_LF_OUTPRECISION		L"lfOutPrecision"
#define		KEY_LF_CLIPPRECISION	L"lfClipPrecision"
#define		KEY_LF_QUALITY			L"lfQuality"
#define		KEY_LF_PITCHANDFAMILY	L"lfPitchAndFamily"
#define		KEY_LF_FACENAME			L"lfFaceName"
#define		KEY_HOTKEY				L"Hotkey"

// レジストリの値名（ジョブ設定）
#define		KEY_MODE				L"Mode"
#define		KEY_HOSTNAME			L"HostName"
#define		KEY_USERFLAG			L"UserFlag"
#define		KEY_USERNAME			L"UserName"
#define		KEY_PASSWDFLAG			L"PasswdFlag"
#define		KEY_PASSWORD			L"Password"
#define		KEY_LOCKBOXFLAG			L"LockBoxFlag"
#define		KEY_INITFILE			L"INI_File"
#define		KEY_TERATERM			L"TeraTerm"
#define		KEY_OPTION				L"Option"
#define		KEY_LOGIN_PROMPT		L"LoginPrompt"
#define		KEY_PASSWORD_PROMPT		L"PasswdPrompt"
#define		KEY_MACROFILE			L"MacroFile"
#define		KEY_TTSSH				L"TeraTerm Mode"
#define		KEY_LOG					L"Log"
#define		KEY_STARTUP				L"Startup"
#define		KEY_KEYFILE				L"PrivateKeyFile"  // add (2005.1.27 yutaka)
#define		KEY_CHALLENGE			L"Challenge"       // add (2007.11.14 yutaka)
#define		KEY_PAGEANT				L"Pageant"         // add (2008.5.26 maya)

#define		STR_ICONMODE			L"showing large icon"
#define		STR_LEFTBUTTONPOPUP		L"showing list by left-click"
#define		STR_HOTKEY				L"showing list hotkey(Ctl+Alt+M)"
#define		STR_NOENTRY				L"(none)"

typedef enum {
	MODE_AUTOLOGIN	=		0x0000,		// 自動でログイン,sshではないときマクロを使用する
	MODE_MACRO		=		0x0001,		// 指定マクロを起動する
	MODE_DIRECT		=		0x0002,		
} JobMode;

// 設定情報構造体
struct JobInfo {
	wchar_t	szName[MAX_PATH];			// ジョブ名
	BOOL	bStartup;					// 起動時にジョブを実行するかどうかのフラグ
	BOOL	bTtssh;						// ttsshを使用するかどうかのフラグ
	JobMode	dwMode;						// ジョブの種類

	// 自動ログイン用設定
	wchar_t	szHostName[MAX_PATH];		// ホスト名
	BOOL	bUsername;					// ユーザ名を入力するかどうかのフラグ
	wchar_t	szUsername[MAX_PATH];		// ユーザ名
	BOOL	bPassword;					// パスワードを入力するかどうかのフラグ
	char	szPassword[MAX_PATH];		// パスワード
	BOOL	bLockBox;					// パスワードの暗号化/復号するかどうかのフラグ

	// マクロ実行用設定
	wchar_t	szMacroFile[MAX_PATH];		// 実行するマクロファイルのファイル名

	// 詳細設定
	wchar_t	szTeraTerm[MAX_PATH];		// 起動アプリ（TeraTerm）のファイル名
	wchar_t	szInitFile[MAX_PATH];		// TeraTermの設定ファイル（起動のみ意外）
	wchar_t	szOption[MAX_PATH];			// アプリケーションのオプション/引数
	wchar_t	szLog[MAX_PATH];			// ログファイル名（自動ログインのみ）
	wchar_t	szLoginPrompt[MAX_PATH];	// ログインプロンプト（自動ログインのみ）
	wchar_t	szPasswdPrompt[MAX_PATH];	// パスワードプロンプト（自動ログインのみ）

	wchar_t PrivateKeyFile[MAX_PATH];   // 秘密鍵ファイル (2005.1.27 yutaka)
	BOOL    bChallenge;                 // keyboard-interative method(/challenge)
	BOOL    bPageant;                   // use Pageant(/pageant)
};

// 表示設定構造体
struct MenuData {
	wchar_t		szName[MAXJOBNUM][MAX_PATH];
	HICON		hLargeIcon[MAXJOBNUM];
	HICON		hSmallIcon[MAXJOBNUM];
	DWORD		dwMenuHeight;
	DWORD		dwIconMode;
	BOOL		bLeftButtonPopup;
	BOOL		bHotkey;
	HFONT		hFont;
	LOGFONTW	lfFont;
	COLORREF	crMenuBg;
	COLORREF	crMenuTxt;
	COLORREF	crSelMenuBg;
	COLORREF	crSelMenuTxt;
};

// 「LockBox入力」ダイアログ LPARAM用構造体
typedef struct {
	int		nMessageFlag;				// in : 0:通常メッセージ、1:LockBox誤りメッセージ
	BOOL	bLockBox;					// in : パスワードの暗号化/復号するかどうかのフラグ
	const char	*pEncryptPassword;			// in : パスワード(暗号文)
	char	*pDecryptPassword;			// out: パスワード(平文)
} LockBoxDlgPrivateData;

// 関数一覧
void	PopupMenu(HWND hWnd);
void	PopupListMenu(HWND hWnd);
BOOL	AddTooltip(int idControl);
BOOL	ConnectHost(HWND hWnd, UINT idItem, const wchar_t *szJobName = NULL);
BOOL	CreateTooltip(HWND hWnd);
BOOL	DecryptPassword(const char *szEncryptPassword, char *szDecryptPassword, HWND hWnd);
BOOL	DeleteLoginHostInformation(HWND hWnd);
BOOL	ErrorMessage(HWND hWnd, LPTSTR msg,...);
BOOL	ExtractAssociatedIconEx(char *szPath, HICON *hLargeIcon, HICON *hSmallIcon);
BOOL	ExecStartup(HWND hWnd);
BOOL	InitConfigDlg(HWND hWnd);
BOOL	InitEtcDlg(HWND hWnd);
BOOL	InitListMenu(HWND hWnd);
BOOL	InitMenu(void);
BOOL	InitVersionDlg(HWND hWnd);
BOOL	LoadConfig(void);
BOOL	LoadLoginHostInformation(HWND hWnd);
BOOL	MakeTTL(HWND hWnd, char *TTLName, JobInfo *jobInfo);
BOOL	ManageWMCommand_Config(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Etc(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Menu(HWND hWnd, WPARAM wParam);
BOOL	ManageWMCommand_Version(HWND hWnd, WPARAM wParam);
BOOL	ManageWMNotify_Config(LPARAM lParam);
BOOL	RedrawMenu(HWND hWnd);
BOOL	RegLoadLoginHostInformation(const wchar_t *szName, JobInfo *jobInfo);
BOOL	RegSaveLoginHostInformation(JobInfo *jobInfo);
BOOL	SaveConfig(void);
BOOL	SaveEtcInformation(HWND hWnd);
BOOL	SaveLoginHostInformation(HWND hWnd);
BOOL	SetDefaultEtcDlg(HWND hWnd);
BOOL	SetMenuFont(HWND hWnd);
BOOL	SetTaskTray(HWND hWnd, DWORD dwMessage);
INT_PTR	CALLBACK DlgCallBack_Config(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR	CALLBACK DlgCallBack_Etc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR	CALLBACK DlgCallBack_LockBox(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR	CALLBACK DlgCallBack_Version(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT	CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT	CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
