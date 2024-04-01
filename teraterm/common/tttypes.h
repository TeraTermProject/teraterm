/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

/* Constants and types for Tera Term */
#pragma once

#include "teraterm.h"
#include "tttypes_charset.h"

#define IdBreakTimer         1
#define IdDelayTimer         2
#define IdProtoTimer         3
#define IdDblClkTimer        4
#define IdScrollTimer        5
#define IdComEndTimer        6
#define IdCaretTimer         7
#define IdPrnStartTimer      8
#define IdPrnProcTimer       9
#define IdCancelConnectTimer 10  // add (2007.1.10 yutaka)
#define IdPasteDelayTimer    11

  /* Window Id */
#define IdVT  1
#define IdTEK 2

  /* Talker mode */
typedef enum {
	IdTalkKeyb = 0,
	IdTalkFile = 2,
	IdTalkQuiet = 3,
	IdTalkSendMem,
} IdTalk;

  /* Character sets */
#define IdASCII    0
#define IdKatakana 1
#define IdKanji    2
#define IdSpecial  3

  /* Color codes */
#define IdBack    0
#define IdRed     1
#define IdGreen   2
#define IdYellow  3
#define IdBlue    4
#define IdMagenta 5
#define IdCyan    6
#define IdFore    7

  /* internal WM_USER messages */
#define WM_USER_ACCELCOMMAND WM_USER+1
#define WM_USER_CHANGEMENU   WM_USER+2
#define WM_USER_CLOSEIME     WM_USER+3
#define WM_USER_COMMNOTIFY   WM_USER+4
#define WM_USER_COMMOPEN     WM_USER+5
#define WM_USER_COMMSTART    WM_USER+6
#define WM_USER_DLGHELP2     WM_USER+7
#define WM_USER_GETHOST      WM_USER+8
//#define WM_USER_FTCANCEL     WM_USER+9
#define WM_USER_PROTOCANCEL  WM_USER+10
#define WM_USER_CHANGETBAR   WM_USER+11
#define WM_USER_KEYCODE      WM_USER+12
#define WM_USER_GETSERIALNO  WM_USER+13
#define WM_USER_CHANGETITLE  WM_USER+14
#define WM_USER_NOTIFYICON   WM_USER+15
#define WM_USER_DROPNOTIFY   WM_USER+16

#define WM_USER_DDEREADY     WM_USER+21
#define WM_USER_DDECMNDEND   WM_USER+22
#define WM_USER_DDECOMREADY  WM_USER+23
#define WM_USER_DDEEND       WM_USER+24
#define WM_USER_MACROBRINGUP WM_USER+25

#define WM_USER_MSTATBRINGUP WM_USER+31

#define WM_USER_NONCONFIRM_CLOSE  WM_USER+40

  /* port type ID */
#define IdTCPIP  1
#define IdSerial 2
#define IdFile   3
#define IdNamedPipe   4

  /* XMODEM option */
#define XoptCheck   1
#define XoptCRC     2
#define Xopt1kCRC   3
#define Xopt1kCksum 4

#define IsXoptCRC(x)	((x) & 2)
#define IsXopt1k(x)	(((x)-1) & 2)

  /* YMODEM option */
#define Yopt1K 1
#define YoptG   2
#define YoptSingle    3

  /* KERMIT option */
#define KmtOptLongPacket 1
#define KmtOptFileAttr 2
#define KmtOptSlideWin 4

// log rotate mode
enum rotate_mode {
	ROTATE_NONE,
	ROTATE_SIZE
};

// Log Timestamp Type
enum LogTimestampType {
    TIMESTAMP_LOCAL,
    TIMESTAMP_UTC,
    TIMESTAMP_ELAPSED_LOGSTART,
    TIMESTAMP_ELAPSED_CONNECTED
};

// log flags (used in ts.LogFlag)
#define LOG_TEL 1
#define LOG_KMT 2
#define LOG_X   4
#define LOG_Z   8
#define LOG_BP  16
#define LOG_QV  32
#define LOG_Y   64

// file transfer flags (used in ts.FTFlag)
#define FT_ZESCCTL  1
#define FT_ZAUTO    2
#define FT_BPESCCTL 4
#define FT_BPAUTO   8
#define FT_RENAME   16

// menu flags (used in ts.MenuFlag)
#define MF_NOSHOWMENU   1
#define MF_NOPOPUP      2
#define MF_NOLANGUAGE   4
#define MF_SHOWWINMENU  8

// Terminal flags (used in ts.TermFlag)
#define TF_FIXEDJIS           1
#define TF_AUTOINVOKE         2
#define TF_CTRLINKANJI        8
#define TF_ALLOWWRONGSEQUENCE 16
#define TF_ACCEPT8BITCTRL     32
#define TF_ENABLESLINE        64
#define TF_BACKWRAP           128
#define TF_CLEARONRESIZE      256
#define TF_ALTSCR             512
#define TF_LOCKTUID           1024
#define TF_INVALIDDECRPSS     2048
#define TF_PRINTERCTRL        4096
#define TF_REMOTECLEARSBUFF   8192

// ANSI/Attribute color flags (used in ts.ColorFlag)
#define CF_PCBOLD16     1
#define CF_AIXTERM16    2
#define CF_XTERM256     4
#define CF_FULLCOLOR    (CF_PCBOLD16 | CF_AIXTERM16 | CF_XTERM256)

#define CF_ANSICOLOR    8

#define CF_BOLDCOLOR    16
#define CF_BLINKCOLOR   32
#define CF_REVERSECOLOR 64
#define CF_URLCOLOR     128

#define CF_USETEXTCOLOR 256
#define CF_REVERSEVIDEO 512
#define CF_UNDERLINE	1024

// Font flags (used in ts.FontFlag)
#define FF_BOLD         1
#define FF_FAINT        2   // Not used
#define FF_ITALIC       4   // Not used
#define FF_UNDERLINE    8
#define FF_BLINK        16  // Not used
#define FF_RAPIDBLINK   32  // Not used
#define FF_REVERSE      64  // Not used
#define FF_INVISIBLE    128 // Not used
#define FF_STRIKEOUT    256 // Not used
#define FF_URLUNDERLINE 512

// port flags (used in ts.PortFlag)
#define PF_CONFIRMDISCONN 1
#define PF_BEEPONCONNECT  2

// Window flags (used in ts.WindowFlag)
#define WF_CURSORCHANGE    1
#define WF_WINDOWCHANGE    2
#define WF_WINDOWREPORT    4
#define WF_TITLEREPORT     24 // (8 | 16)
#define WF_IMECURSORCHANGE 32

// Tab Stop flags (used in ts.TabStopFlag)
#define TABF_NONE       0
#define TABF_HTS7       1
#define TABF_HTS8       2
#define TABF_TBC0       4
#define TABF_TBC3       8
#define TABF_HTS        (TABF_HTS7 | TABF_HTS8)
#define TABF_TBC        (TABF_TBC0 | TABF_TBC3)
#define TABF_ALL        (TABF_HTS | TABF_TBC)

// ISO 2022 Shift flags (used in ts.ISO2022Flag)
#define ISO2022_SHIFT_NONE   0x0000
#define ISO2022_SI           0x0001
#define ISO2022_SO           0x0002
#define ISO2022_LS2          0x0004
#define ISO2022_LS3          0x0008
#define ISO2022_LS1R         0x0010
#define ISO2022_LS2R         0x0020
#define ISO2022_LS3R         0x0040
#define ISO2022_SS2          0x0100
#define ISO2022_SS3          0x0200
#define ISO2022_LS           (ISO2022_SI | ISO2022_SO | ISO2022_LS2 | ISO2022_LS3)
#define ISO2022_LSR          (ISO2022_LS1R | ISO2022_LS2R | ISO2022_LS3R)
#define ISO2022_SS           (ISO2022_SS2 | ISO2022_SS3)
#define ISO2022_SHIFT_ALL    (ISO2022_LS | ISO2022_LSR | ISO2022_SS)

// Control Sequence flags (used in ts.CtrlFlag)
#define CSF_CBNONE	0
#define CSF_CBWRITE     1
#define CSF_CBREAD      2
#define CSF_CBRW        (CSF_CBREAD | CSF_CBWRITE)
#define CSF_CBMASK	CSF_CBRW

// Debug Flags (used in ts.DebugModes)
#define DBGF_NONE	0
#define DBGF_NORM	1
#define DBGF_HEXD	2
#define DBGF_NOUT	4
#define DBGF_ALL	(DBGF_NORM | DBGF_HEXD | DBGF_NOUT)

// Clipboard Paste Flags (used in ts.PasteFlag)
#define CPF_DISABLE_RBUTTON        0x0001
#define CPF_CONFIRM_RBUTTON        0x0002
#define CPF_DISABLE_MBUTTON        0x0004
#define CPF_CONFIRM_CHANGEPASTE    0x0010
#define CPF_CONFIRM_CHANGEPASTE_CR 0x0020
#define CPF_TRIM_TRAILING_NL       0x0100

// Title Reporting Type
#define IdTitleReportIgnore 0
#define IdTitleReportAccept 8
#define IdTitleReportEmpty  24

// iconf flags (used in ts.VTIcon and ts.TEKIcon)
#define IdIconDefault 0

// Beep type
#define IdBeepOff    0
#define IdBeepOn     1
#define IdBeepVisual 2

// TitleChangeRequest types
#define IdTitleChangeRequestOff       0
#define IdTitleChangeRequestOverwrite 1
#define IdTitleChangeRequestAhead     2
#define IdTitleChangeRequestLast      3
#define IdTitleChangeRequestMax       IdTitleChangeRequestLast

// Meta8Bit mode
#define IdMeta8BitOff   0
#define IdMeta8BitRaw   1
#define IdMeta8BitText  2

#define TitleBuffSize  50

// Eterm lookfeel alphablend structure
typedef struct {
	int BGEnable;	// 0/1/2 = theme�g�p���Ȃ�/�Œ�e�[�}/�����_���e�[�}
	int BGUseAlphaBlendAPI;
	char reserve_BGSPIPath[MAX_PATH];
	int BGFastSizeMove;
	int BGNoCopyBits;
	int BGNoFrame;
	char reserver_BGThemeFile[MAX_PATH - sizeof(wchar_t *) * 2];
	wchar_t *BGThemeFileW;
	wchar_t *BGSPIPathW;
} eterm_lookfeel_t;

/**
 *	TTTSet
 *		�Z�b�V�����̕�����
 *		���̍\���͕̂�����ɃR�s�[�����
 *		path���A���I�Ɋm�ۂ��������̃R�s�[��
 *		../ttcmn_dup.cpp ���Q��
 */
struct tttset {
/*------ VTSet --------*/
	/* Tera Term home directory */
	char HomeDir[MAXPATHLEN];		// �l�ݒ�t�@�C���̂���t�H���_ switch HomeDirW

	/* Setup file name */
	char SetupFName[MAX_PATH];
	char reserve_KeyCnfFN[MAX_PATH];	// switch KeyCnfFNW
	char LogFN[MAX_PATH];
	char reserve_MacroFN[MAX_PATH];		// switch MacroFNW
	char HostName[1024];

	POINT VTPos;
	char VTFont[LF_FACESIZE];
	POINT VTFontSize;
	int VTFontCharSet;
	int FontDW, FontDH, FontDX, FontDY;
	char PrnFont[LF_FACESIZE];
	POINT PrnFontSize;
	int PrnFontCharSet;
	POINT VTPPI, TEKPPI;
	int PrnMargin[4];
	char PrnDev[80];
	WORD PassThruDelay;
	WORD PrnConvFF;
	WORD FontFlag;
	WORD reserve_RussFont;
	int ScrollThreshold;
	WORD Debug;
	WORD LogFlag;
	WORD FTFlag;
	WORD TransBin, Append;
	WORD XmodemOpt, XmodemBin;
	int ZmodemDataLen, ZmodemWinSize;
	int QVWinSize;
	char reserve_FileDir[256];		// switch FileDirW
	char FileSendFilter[128];
	WORD Language;
	char reserver_DelimList[52];
	WORD DelimDBCS;
	WORD Minimize;
	WORD HideWindow;
	WORD MenuFlag;
	WORD SelOnActive;
	WORD AutoTextCopy;
/*------ TEKSet --------*/
	POINT TEKPos;
	char TEKFont[LF_FACESIZE];
	POINT TEKFontSize;
	int TEKFontCharSet;
	int GINMouseCode;
/*------ TermSet --------*/
	int TerminalWidth;
	int TerminalHeight;
	WORD TermIsWin;
	WORD AutoWinResize;
	WORD CRSend;
	WORD CRReceive;
	WORD LocalEcho;
	char Answerback[32];
	int AnswerbackLen;
	WORD KanjiCode;
	WORD KanjiCodeSend;
	WORD JIS7Katakana;
	WORD JIS7KatakanaSend;
	WORD KanjiIn;
	WORD KanjiOut;
	WORD reserve_RussHost;
	WORD reserve_RussClient;
	WORD reserve_RussPrint;
	WORD AutoWinSwitch;
	WORD TerminalID;
	WORD TermFlag;
/*------ WinSet --------*/
	WORD VTFlag;
	HFONT SampleFont;
	WORD reserve_TmpColor[12][6];
	/* Tera Term window setup variables */
	char Title[TitleBuffSize];
	WORD TitleFormat;
	WORD CursorShape;
	WORD NonblinkingCursor;
	WORD EnableScrollBuff;
	LONG ScrollBuffSize;
	LONG ScrollBuffMax;
	WORD HideTitle;
	WORD PopupMenu;
	int ColorFlag;
	WORD TEKColorEmu;
	COLORREF VTColor[2];
	COLORREF TEKColor[2];
	COLORREF URLColor[2];
	COLORREF VTBoldColor[2];       // SGR 1
	COLORREF VTFaintColor[2];      // SGR 2
	COLORREF VTItalicColor[2];     // SGR 3
	COLORREF VTUnderlineColor[2];  // SGR 4
	COLORREF VTBlinkColor[2];      // SGR 5
	COLORREF VTRapidBlinkColor[2]; // SGR 6
	COLORREF VTReverseColor[2];    // SGR 7
	COLORREF VTInvisibleColor[2];  // SGR 8
	COLORREF VTStrikeoutColor[2];  // SGR 9
	COLORREF DummyColor[2];
	WORD Beep;
/*------ KeybSet --------*/
	WORD BSKey;
	WORD DelKey;
	WORD UseIME;
	WORD IMEInline;
	WORD MetaKey;
	WORD RussKeyb;
/*------ PortSet --------*/
	WORD PortType;
	/* TCP/IP */
	WORD TCPPort;
	WORD Telnet;
	WORD TelPort;
	WORD TelBin;
	WORD TelEcho;
	char TermType[40];
	WORD AutoWinClose;
	WORD PortFlag;
	WORD TCPCRSend;
	WORD TCPLocalEcho;
	WORD HistoryList;
	/* Serial */
	WORD ComPort;
	WORD Baud_; /* not in use */
	WORD Parity;
	WORD DataBit;
	WORD StopBit;
	WORD Flow;
	WORD DelayPerChar;
	WORD DelayPerLine;
	WORD MaxComPort;
	WORD ComAutoConnect;
	WORD EnableContinuedLineCopy;
	COLORREF ANSIColor[16];
	/* protocol used in connect() */
	int ProtocolFamily;
	char MouseCursorName[16];
	BYTE AlphaBlendActive;
	BYTE AlphaBlendInactive;
	BYTE reserve_0[2];				// ���g�p
	char CygwinDirectory[MAX_PATH];
	char reserve_Locale[80];
	int reserve_CodePage;
	int DuplicateSession;
	char ViewlogEditor[MAX_PATH];
	WORD LogTypePlainText;
	WORD LogTimestamp;
	char reserve_LogDefaultName[80];		// �p�~,LogDefaultNameW �ֈڍs
	char reserve_LogDefaultPath[MAX_PATH];	// �p�~,LogDefaultPathW �ֈڍs
	WORD LogAutoStart;
	unsigned int PasteFlag;
	WORD FallbackToCP932;
	WORD DisableAcceleratorSendBreak;
	int EnableClickableUrl;
	eterm_lookfeel_t EtermLookfeel;
	WORD UseNormalBGColor;
	char UILanguageFile[MAX_PATH];			// ��΃p�X
	char reserve_UIMsg[1024/*MAX_UIMSG*/];
	WORD BroadcastCommandHistory;
	WORD AcceptBroadcast;		// 337: 2007/03/20
	WORD DisableTCPEchoCR;  // TCPLocalEcho/TCPCRSend �𖳌��ɂ��� (maya 2007.4.25)
	int ConnectingTimeout;
	WORD VTCompatTab;
	WORD TelKeepAliveInterval;
	WORD MaxBroadcatHistory;
	WORD DisableAppKeypad;
	WORD DisableAppCursor;
	WORD ClearComBuffOnOpen;
	WORD Send8BitCtrl;
	char UILanguageFile_ini[MAX_PATH];		// ���΃p�X,ini�t�@�C���̒��g
	WORD SelectOnlyByLButton;
	WORD TelAutoDetect;
	char XModemRcvCommand[MAX_PATH];
	char ZModemRcvCommand[MAX_PATH];
	WORD ConfirmFileDragAndDrop;
	WORD TranslateWheelToCursor;
	WORD HostDialogOnStartup;
	WORD MouseEventTracking;
	WORD KillFocusCursor;
	WORD LogHideDialog;
	WORD FTHideDialog;
	int TerminalOldWidth;
	int TerminalOldHeight;
	WORD MaximizedBugTweak;
	WORD NotifyClipboardAccess;
	WORD SaveVTWinPos;
	WORD LogTimestampType;
	int MouseWheelScrollLine;
	WORD CRSend_ini;
	WORD LocalEcho_ini;
	WORD UnicodeDecSpMapping;
	WORD VTIcon;
	WORD TEKIcon;
	WORD ScrollWindowClearScreen;
	WORD AutoScrollOnlyInBottomLine;
	WORD reserve_UnknownUnicodeCharaAsWide;
	char YModemRcvCommand[MAX_PATH];
	WORD AcceptTitleChangeRequest;
	SIZE PasteDialogSize;
	WORD DisableMouseTrackingByCtrl;
	WORD DisableWheelToCursorByCtrl;
	WORD StrictKeyMapping;
	WORD Wait4allMacroCommand;
	WORD DisableMenuSendBreak;
	WORD ClearScreenOnCloseConnection;
	WORD DisableAcceleratorDuplicateSession;
	int PasteDelayPerLine;
	WORD FontScaling;
	WORD Meta8Bit;
	WORD WindowFlag;
	WORD EnableLineMode;
	char ConfirmChangePasteStringFile[MAX_PATH];
	DWORD Baud;
	WORD LogBinary;
	WORD DisableMenuDuplicateSession;
	WORD DisableMenuNewConnection;
	char TerminalUID[9];
	WORD ISO2022Flag;
	WORD JumpList;
	WORD TabStopFlag;
	DWORD CtrlFlag;
	char ClickableUrlBrowser[MAX_PATH];
	char ClickableUrlBrowserArg[MAX_PATH];
	WORD LogLockExclusive;
	WORD KermitOpt;
	WORD FontQuality;
	char ScpSendDir[MAXPATHLEN];				// SCP ���M��(�z�X�g�̃f�B���N�g��)
	char reserver_BGImageFilePath[MAX_PATH];
	int LogRotate;		//	enum rotate_mode LogRotate;
	DWORD LogRotateSize;
	WORD LogRotateSizeType;
	WORD LogRotateStep;
	WORD DeferredLogWriteMode;
	DWORD BeepOverUsedCount;
	DWORD BeepOverUsedTime;
	DWORD BeepSuppressTime;
	WORD LogAllBuffIncludedInFirst;
	unsigned int MaxOSCBufferSize;
	WORD JoinSplitURL;
	char JoinSplitURLIgnoreEOLChar;
	char MulticastName[MAX_PATH];
	WORD DebugModes;
	WORD AcceleratorNewConnection;
	WORD AcceleratorCygwinConnection;
	int SendBreakTime;
	WORD FileSendHighSpeedMode;
	WORD AutoComPortReconnect;
	WORD reserver_BGImgBrightness;
	char reserve_CygtermSettings[720];
	int XmodemTimeOutInit;
	int XmodemTimeOutInitCRC;
	int XmodemTimeOutShort;
	int XmodemTimeOutLong;
	int XmodemTimeOutVLong;
	int YmodemTimeOutInit;
	int YmodemTimeOutInitCRC;
	int YmodemTimeOutShort;
	int YmodemTimeOutLong;
	int YmodemTimeOutVLong;
	int ZmodemTimeOutNormal;
	int ZmodemTimeOutTCPIP;
	int ZmodemTimeOutInit;
	int ZmodemTimeOutFin;
	WORD WaitCom;
	WORD ListHiddenFonts;
	char LogTimestampFormat[48];
	int TerminalInputSpeed;
	int TerminalOutputSpeed;
	char reserve_DialogFontName[LF_FACESIZE];	// DialogFontNameW �ֈڍs
	int DialogFontPoint;
	int DialogFontCharSet;
	int ConfigVersion;
	int RunningVersion;
	DWORD SelectStartDelay;
	BYTE UnicodeAmbiguousWidth;
	BYTE UnicodeEmojiOverride;
	BYTE UnicodeEmojiWidth;
	wchar_t *HomeDirW;		// �l�ݒ�t�@�C���̂���t�H���_
	wchar_t *SetupFNameW;	// "TERATERM.INI" �̃t���p�X
	wchar_t *KeyCnfFNW;		// "KEYBOARD.CNF" �̃t���p�X
	wchar_t *LogFNW;
	wchar_t *MacroFNW;
	wchar_t *UILanguageFileW;			// ��΃p�X
	wchar_t *UILanguageFileW_ini;		// ���΃p�X,ini�t�@�C���̒��g
	wchar_t *ExeDirW;					// ttermpro.exe �̂���t�H���_
	wchar_t *LogDirW;					// log��_���v��u���t�H���_
	wchar_t *FileDirW;					// �_�E�����[�h�p�X("%APPDATA%" �����܂܂��,�g�p�O�Ɋ��ϐ���W�J���邱��)
	wchar_t *LogDefaultPathW;			// ���O�t�H���_([file]/[log]���j���[�̃��O)
	HINSTANCE PluginVTIconInstance;
	WORD PluginVTIconID;
	HINSTANCE TeraTermInstance;
	WORD WindowCornerDontround;
	wchar_t DialogFontNameW[LF_FACESIZE];
	BOOL NotifySound;
	wchar_t *LogDefaultNameW;
	BOOL Dec2Unicode;
	wchar_t *DelimListW;
	WORD IniAutoBackup;

	// Experimental
	BYTE ExperimentalTreeProprtySheetEnable;
	BYTE ExperimentalDontUseFontDialog;
};

typedef struct tttset TTTSet, *PTTSet;

  /* New Line modes */
#define IdCR   1
#define IdCRLF 2
#define IdLF   3
#define IdAUTO 4

  /* Terminal ID */
#define IdVT100  1
#define IdVT100J 2
#define IdVT101  3
#define IdVT102  4
#define IdVT102J 5
#define IdVT220J 6
#define IdVT282  7
#define IdVT320  8
#define IdVT382  9
#define IdVT420  10
#define IdVT520  11
#define IdVT525  12

#define TermWidthMax  1000
#define TermHeightMax 500

  /* Cursor shapes */
#define IdBlkCur 1
#define IdVCur   2
#define IdHCur   3

#define IdBS  1
#define IdDEL 2

#define IdMetaOff   0
#define IdMetaOn    1
#define IdMetaLeft  2
#define IdMetaRight 3

  /* Mouse tracking mode */
#define IdMouseTrackNone     0
#define IdMouseTrackDECELR   1
#define IdMouseTrackX10      2
#define IdMouseTrackVT200    3
#define IdMouseTrackVT200Hl  4  // not supported
#define IdMouseTrackBtnEvent 5
#define IdMouseTrackAllEvent 6
#define IdMouseTrackNetTerm  7
#define IdMouseTrackJSBTerm  8

  /* Extended mouse tracking mode */
#define IdMouseTrackExtNone  0
#define IdMouseTrackExtUTF8  1
#define IdMouseTrackExtSGR   2
#define IdMouseTrackExtURXVT 3

  /* Mouse event */
#define IdMouseEventCurStat  0
#define IdMouseEventBtnDown  1
#define IdMouseEventBtnUp    2
#define IdMouseEventMove     3
#define IdMouseEventWheel    4

  /* Mouse buttons */
#define IdLeftButton    0
#define IdMiddleButton  1
#define IdRightButton   2
#define IdButtonRelease 3

  /* Serial port ID */
#define IdCOM1 1
#define IdCOM2 2
#define IdCOM3 3
#define IdCOM4 4
  /* Baud rate ID */
#define BaudNone    0

  /* Parity ID */
#define IdParityNone  1
#define IdParityOdd   2
#define IdParityEven  3
#define IdParityMark  4
#define IdParitySpace 5
  /* Data bit ID */
#define IdDataBit7 1
#define IdDataBit8 2
  /* Stop bit ID */
#define IdStopBit1 1
#define IdStopBit2 2
  /* Flow control ID */
#define IdFlowX 1
#define IdFlowHard 2    // RTS/CTS(hardware flow)
#define IdFlowNone 3
#define IdFlowHardDsrDtr 4  // DSR/DTR(hardware flow)

/* Control Characters */

#define NUL  0x00
#define SOH  0x01
#define STX  0x02
#define ETX  0x03
#define EOT  0x04
#define ENQ  0x05
#define ACK  0x06
#define BEL  0x07
#define BS   0x08
#define HT   0x09
#define LF   0x0A
#define VT   0x0B
#define FF   0x0C
#define CR   0x0D
#define SO   0x0E
#define SI   0x0F
#define DLE  0x10
#define DC1  0x11
#define XON  0x11
#define DC2  0x12
#define DC3  0x13
#define XOFF 0x13
#define DC4  0x14
#define NAK  0x15
#define SYN  0x16
#define ETB  0x17
#define CAN  0x18
#define EM   0x19
#define SUB  0x1A
#define ESC  0x1B
#define FS   0x1C
#define GS   0x1D
#define RS   0x1E
#define US   0x1F

#define SP   0x20

#define DEL  0x7F

#define IND  0x84
#define NEL  0x85
#define SSA  0x86
#define ESA  0x87
#define HTS  0x88
#define HTJ  0x89
#define VTS  0x8A
#define PLD  0x8B
#define PLU  0x8C
#define RI   0x8D
#define SS2  0x8E
#define SS3  0x8F
#define DCS  0x90
#define PU1  0x91
#define PU2  0x92
#define STS  0x93
#define CCH  0x94
#define MW   0x95
#define SPA  0x96
#define EPA  0x97
#define SOS  0x98


#define CSI  0x9B
#define ST   0x9C
#define OSC  0x9D
#define PM   0x9E
#define APC  0x9F

#define InBuffSize  1024
#define OutBuffSize (1024*16)

typedef struct {
	BYTE InBuff[InBuffSize];
	int InBuffCount, InPtr;
	BYTE OutBuff[OutBuffSize];
	int OutBuffCount, OutPtr;

	HWND HWin;
	BOOL Ready;
	BOOL Open;
	WORD PortType;
	WORD ComPort;
	UINT_PTR s; /* SOCKET */
	WORD RetryCount;
	HANDLE ComID;
	BOOL CanSend, RRQ;

	BOOL reserve_SendKanjiFlag;
	BOOL reserve_EchoKanjiFlag;
	int SendCode;
	int EchoCode;
	BYTE reserve_SendKanjiFirst;
	BYTE reserve_EchoKanjiFirst;

	/* from VTSet */
	WORD Language;
	/* from TermSet */
	WORD CRSend;
	WORD KanjiCodeEcho;
	WORD JIS7KatakanaEcho;
	WORD KanjiCodeSend;
	WORD JIS7KatakanaSend;
	WORD KanjiIn;
	WORD KanjiOut;
	WORD reserve_RussHost;
	WORD reserve_RussClient;
	/* from PortSet */
	WORD DelayPerChar;
	WORD DelayPerLine;
	BOOL TelBinRecv, TelBinSend;

	BOOL DelayFlag;
	BOOL TelFlag, TelMode;
	BOOL IACFlag, TelCRFlag;
	BOOL TelCRSend, TelCRSendEcho;
	BOOL TelAutoDetect; /* TTPLUG */

	/* Text log */
	HANDLE reserve_2;		// HLogBuf;
	PCHAR reserve_4;		// LogBuf;
	int reserve_5[3];		// LogPtr, LStart, LCount;
	/* Binary log */
	HANDLE reserve_3;		// HBinBuf;
	PCHAR reserve_6;		// BinBuf;
	int reserve_7[3];		// BinPtr, BStart, BCount;
	int reserve_1[2];		// DStart, DCount;
	int reserve_8;			// BinSkip;
	WORD reserve_9;			// FilePause;
	BOOL reserve_10;		// ProtoFlag;
	/* message flag */
	WORD NoMsg;
	/* if TRUE, teraterm trys to connect other protocol family */
	BOOL RetryWithOtherProtocol;
	struct addrinfo * res0;
	struct addrinfo * res;
	char *reserve_11;	// Locale
	int *reserve_CodePage;
	int *ConnetingTimeout;

	time_t LastSendTime;
	WORD isSSH;
	char reserve_TitleRemote[50];	// 50 = TitleBuffSize

	BYTE LineModeBuff[OutBuffSize];
	int LineModeBuffCount, FlushLen;
	BOOL Flush;

	BOOL TelLineMode;
	void *reserve_12;	// _locale_t locale
	BOOL reserve_13;	// VirtualStoreEnabled;

	void *NotifyIcon;

	DWORD ConnectedTime;

	void (*Log1Byte)(BYTE b);
	void (*Log1Bin)(BYTE b);
	void (*LogBinSkip)(int add);

	TTTSet *ts;

	wchar_t *TitleRemoteW;
} TComVar;
typedef TComVar *PComVar;

#define ID_FILE          0
#define ID_EDIT          1
#define ID_SETUP         2
#define ID_CONTROL       3
#define ID_HELPMENU      4

#define ID_WINDOW_1            50801
#define ID_WINDOW_WINDOW       50810
#define ID_WINDOW_MINIMIZEALL  50811
#define ID_WINDOW_RESTOREALL   50812
#define ID_WINDOW_CASCADEALL   50813
#define ID_WINDOW_STACKED      50814
#define ID_WINDOW_SIDEBYSIDE   50815
#define ID_WINDOW_UNDO         50816
#define ID_TEKWINDOW_WINDOW    51810

#define ID_TRANSFER      9 // the position on [File] menu
#define ID_SHOWMENUBAR   995

#define MAXNWIN 256
#define MAXCOMPORT 4096
#define MAXHOSTLIST 500
