/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, file transfer routines */
#include "stdafx.h"
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "tt_res.h"
#include "ftdlg.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "helpid.h"
#include "dlglib.h"
#include "vtterm.h"

#include "filesys.h"
#include "ftlib.h"

#include "buffer.h"

#include <io.h>
#include <process.h>

#define FS_BRACKET_NONE  0
#define FS_BRACKET_START 1
#define FS_BRACKET_END   2

PFileVar LogVar = NULL;
PFileVar SendVar = NULL;
PFileVar FileVar = NULL;
static PCHAR ProtoVar = NULL;
static int ProtoId;

static BYTE LogLast = 0;
BOOL FileLog = FALSE;
BOOL BinLog = FALSE;
BOOL DDELog = FALSE;
static BOOL FileRetrySend, FileRetryEcho, FileCRSend, FileReadEOF, BinaryMode;
static BYTE FileByte;

#define FILE_SEND_BUF_SIZE  8192
struct FileSendHandler {
	CHAR buf[FILE_SEND_BUF_SIZE];
	int pos;
	int end;
};
static struct FileSendHandler FileSendHandler;
static int FileDlgRefresh;

static int FileBracketMode = FS_BRACKET_NONE;
static int FileBracketPtr = 0;
static char BracketStartStr[] = "\033[200~";
static char BracketEndStr[] = "\033[201~";

static BOOL FSend = FALSE;

HWND HWndLog = NULL; //steven add

static HMODULE HTTFILE = NULL;
static int TTFILECount = 0;

PGetSetupFname GetSetupFname;
PGetTransFname GetTransFname;
PGetMultiFname GetMultiFname;
PGetGetFname GetGetFname;
PSetFileVar SetFileVar;
PGetXFname GetXFname;
PProtoInit ProtoInit;
PProtoParse ProtoParse;
PProtoTimeOutProc ProtoTimeOutProc;
PProtoCancel ProtoCancel;

#define IdGetSetupFname  1
#define IdGetTransFname  2
#define IdGetMultiFname  3
#define IdGetGetFname	 4
#define IdSetFileVar	 5
#define IdGetXFname	 6

#define IdProtoInit	 7
#define IdProtoParse	 8
#define IdProtoTimeOutProc 9
#define IdProtoCancel	 10

/*
   Line Head flag for timestamping
   2007.05.24 Gentaro
*/
enum enumLineEnd {
	Line_Other = 0,
	Line_LineHead = 1,
	Line_FileHead = 2,
};

enum enumLineEnd eLineEnd = Line_LineHead;


// 遅延書き込み用スレッドのメッセージ
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void CloseFileSync(PFileVar ptr);


BOOL LoadTTFILE()
{
	BOOL Err;

	if (HTTFILE != NULL)
	{
		TTFILECount++;
		return TRUE;
	}
	else
		TTFILECount = 0;

	HTTFILE = LoadLibrary("TTPFILE.DLL");
	if (HTTFILE == NULL)
		return FALSE;

	TTFILESetUILanguageFile(ts.UILanguageFile);
	TTFILESetFileSendFilter(ts.FileSendFilter);

	Err = FALSE;
	GetSetupFname = (PGetSetupFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetSetupFname));
	if (GetSetupFname==NULL)
		Err = TRUE;

	GetTransFname = (PGetTransFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetTransFname));
	if (GetTransFname==NULL)
		Err = TRUE;

	GetMultiFname = (PGetMultiFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetMultiFname));
	if (GetMultiFname==NULL)
		Err = TRUE;

	GetGetFname = (PGetGetFname)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdGetGetFname));
	if (GetGetFname==NULL)
		Err = TRUE;

	SetFileVar = (PSetFileVar)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdSetFileVar));
	if (SetFileVar==NULL)
		Err = TRUE;

	GetXFname = (PGetXFname)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdGetXFname));
	if (GetXFname==NULL)
		Err = TRUE;

	ProtoInit = (PProtoInit)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdProtoInit));
	if (ProtoInit==NULL)
		Err = TRUE;

	ProtoParse = (PProtoParse)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdProtoParse));
	if (ProtoParse==NULL)
		Err = TRUE;

	ProtoTimeOutProc = (PProtoTimeOutProc)GetProcAddress(HTTFILE,
	                                                     MAKEINTRESOURCE(IdProtoTimeOutProc));
	if (ProtoTimeOutProc==NULL)
		Err = TRUE;

	ProtoCancel = (PProtoCancel)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdProtoCancel));
	if (ProtoCancel==NULL)
		Err = TRUE;

	if (Err)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
		return FALSE;
	}
	else {
		TTFILECount = 1;
		return TRUE;
	}
}

BOOL FreeTTFILE()
{
	if (TTFILECount==0)
		return FALSE;
	TTFILECount--;
	if (TTFILECount>0)
		return TRUE;
	if (HTTFILE!=NULL)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
	}
	return TRUE;
}

static PFileTransDlg FLogDlg = NULL;
static PFileTransDlg SendDlg = NULL;
static PProtoDlg PtDlg = NULL;

BOOL OpenFTDlg(PFileVar fv)
{
	PFileTransDlg FTDlg;
	HWND HFTDlg;
	char uimsg[MAX_UIMSG];

	FTDlg = new CFileTransDlg();

	fv->StartTime = 0;
	fv->ProgStat = 0;

	if (fv->OpId != OpLog) {
		fv->HideDialog = ts.FTHideDialog;
	}

	if (FTDlg!=NULL)
	{
		FTDlg->Create(fv, &cv, &ts);
		FTDlg->RefreshNum();
		if (fv->OpId == OpLog) {
			HWndLog = FTDlg->m_hWnd; // steven add
		}
	}

	if (fv->OpId==OpLog)
		FLogDlg = FTDlg; /* Log */
	else
		SendDlg = FTDlg; /* File send */

	HFTDlg=FTDlg->GetSafeHwnd();

	GetDlgItemText(HFTDlg, IDC_TRANS_FILENAME, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_FILENAME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_TRANS_FILENAME, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDC_FULLPATH_LABEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_FULLPATH", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_FULLPATH_LABEL, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDC_TRANS_TRANS, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_TRNAS", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_TRANS_TRANS, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDC_TRANS_ELAPSED, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_ELAPSED", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_TRANS_ELAPSED, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDCANCEL, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_CLOSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDCANCEL, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDC_TRANSPAUSESTART, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_FILETRANS_PAUSE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_TRANSPAUSESTART, ts.UIMsg);
	GetDlgItemText(HFTDlg, IDC_TRANSHELP, uimsg, sizeof(uimsg));
	get_lang_msg("BTN_HELP", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(HFTDlg, IDC_TRANSHELP, ts.UIMsg);

	if (fv->OpId == OpSendFile) {
		fv->StartTime = GetTickCount();
		InitDlgProgress(HFTDlg, IDC_TRANSPROGRESS, &fv->ProgStat);
		ShowWindow(GetDlgItem(HFTDlg, IDC_TRANS_ELAPSED), SW_SHOW);
	}

	return (FTDlg!=NULL);
}

void ShowFTDlg(WORD OpId)
{
	if (OpId == OpLog) {
		if (FLogDlg != NULL) {
			FLogDlg->ShowWindow(SW_SHOWNORMAL);
			SetForegroundWindow(FLogDlg->GetSafeHwnd());
		}
	}
	else {
		if (SendDlg != NULL) {
			SendDlg->ShowWindow(SW_SHOWNORMAL);
			SetForegroundWindow(SendDlg->GetSafeHwnd());
		}
	}
}

BOOL NewFileVar(PFileVar *fv)
{
	if ((*fv)==NULL)
	{
		*fv = (PFileVar)malloc(sizeof(TFileVar));
		if ((*fv)!=NULL)
		{
			memset(*fv, 0, sizeof(TFileVar));
			strncpy_s((*fv)->FullName, sizeof((*fv)->FullName),ts.FileDir, _TRUNCATE);
			AppendSlash((*fv)->FullName,sizeof((*fv)->FullName));
			(*fv)->DirLen = strlen((*fv)->FullName);
			(*fv)->FileOpen = FALSE;
			(*fv)->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
			(*fv)->HMainWin = HVTWin;
			(*fv)->Success = FALSE;
			(*fv)->NoMsg = FALSE;
			(*fv)->HideDialog = FALSE;
		}
	}

	return ((*fv)!=NULL);
}

void FreeFileVar(PFileVar *fv)
{
	if ((*fv)!=NULL)
	{
		CloseFileSync(*fv);
		//if ((*fv)->FileOpen) _lclose((*fv)->FileHandle);
		if ((*fv)->FnStrMemHandle>0)
		{
			GlobalUnlock((*fv)->FnStrMemHandle);
			GlobalFree((*fv)->FnStrMemHandle);
		}
		free(*fv);
		*fv = NULL;
	}
}

// &h をホスト名に置換 (2007.5.14)
// &p をTCPポート番号に置換 (2009.6.12)
void ConvertLogname(char *c, int destlen)
{
	char buf[MAXPATHLEN], buf2[MAXPATHLEN], *p = c;
	char tmphost[1024];
	char tmpuser[256+1];
	DWORD len_user = sizeof(tmpuser);

	memset(buf, 0, sizeof(buf));

	while(*p != '\0') {
		if (*p == '&' && *(p+1) != '\0') {
			switch (*(p+1)) {
			  case 'h':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						// ホスト名がIPv6アドレスだと、ファイル名に使用できない文字が入るため、
						// 余計な文字は削除する。
						// (2013.3.9 yutaka)
						strncpy_s(tmphost, sizeof(tmphost), ts.HostName, _TRUNCATE);
						//strncpy_s(tmphost, sizeof(tmphost), "2001:0db8:bd05:01d2:288a:1fc0:0001:10ee", _TRUNCATE);
						replaceInvalidFileNameChar(tmphost, '_');
						strncat_s(buf,sizeof(buf), tmphost, _TRUNCATE);
					}
					else if (cv.PortType == IdSerial) {
						strncpy_s(buf2,sizeof(buf2),buf,_TRUNCATE);
						_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%sCOM%d", buf2, ts.ComPort);
					}
				}
				break;
			  case 'p':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						char port[6];
						_snprintf_s(port, sizeof(port), _TRUNCATE, "%d", ts.TCPPort);
						strncat_s(buf,sizeof(buf),port,_TRUNCATE);
					}
				}
				break;
			  case 'u':
				if (GetUserName(tmpuser, &len_user) != 0) {
					strncat_s(buf,sizeof(buf),tmpuser,_TRUNCATE);
				}
				break;
			  default:
				strncpy_s(buf2,sizeof(buf2),p,2);
				strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
			}
			p++;
		}
		else {
			strncpy_s(buf2,sizeof(buf2),p,1);
			strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
		}
		p++;
	}
	strncpy_s(c, destlen, buf, _TRUNCATE);
}

void FixLogOption()
{
	if (ts.LogBinary) {
		ts.LogTypePlainText = false;
		ts.LogTimestamp = false;
	}
}


// スレッドの終了とファイルのクローズ
static void CloseFileSync(PFileVar ptr)
{
	BOOL ret;
	DWORD code;

	if (!ptr->FileOpen)
		return;

	if (ptr->LogThread != (HANDLE)-1) {
		// スレッドの終了待ち
		ret = PostThreadMessage(ptr->LogThreadId, WM_QUIT, 0, 0);
		if (ret != 0) {
			// スレッドキューにエンキューできた場合のみ待ち合わせを行う。
			WaitForSingleObject(ptr->LogThread, INFINITE);
		}
		else {
			code = GetLastError();
		}
		CloseHandle(ptr->LogThread);
		ptr->LogThread = (HANDLE)-1;
	}
#ifdef FileVarWin16
	_lclose(ptr->FileHandle);
#else
	CloseHandle((HANDLE)ptr->FileHandle);
#endif
}

// 遅延書き込み用スレッド
static unsigned _stdcall DeferredLogWriteThread(void *arg)
{
	MSG msg;
	PFileVar fv = (PFileVar)arg;
	PCHAR buf;
	DWORD buflen;
	DWORD wrote;

	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// スレッドキューの作成が終わったことをスレッド生成元へ通知する。
	if (fv->LogThreadEvent != NULL) {
		SetEvent(fv->LogThreadEvent);
	}

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {
			case WM_DPC_LOGTHREAD_SEND:
				buf = (PCHAR)msg.wParam;
				buflen = (DWORD)msg.lParam;
#ifdef FileVarWin16
				_lwrite(fv->FileHandle, buf, buflen );
#else
				WriteFile((HANDLE)LogVar->FileHandle, buf, buflen, &wrote, NULL);
#endif
				free(buf);   // ここでメモリ解放
				break;

			case WM_QUIT:
				goto end;
				break;
		}
	}

end:
	_endthreadex(0);
	return (0);
}


extern "C" {
BOOL LogStart()
{
	LONG Option;
	char *logdir;
	unsigned tid;
	DWORD ofs, size, written_size;
	char buf[512];
	const char *crlf = "\r\n";
	DWORD crlf_len = 2;

	if ((FileLog) || (BinLog)) return FALSE;

	if (! LoadTTFILE()) return FALSE;
	if (! NewFileVar(&LogVar))
	{
		FreeTTFILE();
		return FALSE;
	}
	LogVar->OpId = OpLog;

	if (strlen(ts.LogDefaultPath) > 0) {
		logdir = ts.LogDefaultPath;
	}
	else if (strlen(ts.FileDir) > 0) {
		logdir = ts.FileDir;
	}
	else {
		logdir = ts.HomeDir;
	}

	if (strlen(&(LogVar->FullName[LogVar->DirLen]))==0)
	{
		// LOWORD
		// 0x0001 = Binary
		// HIWORD
		// 0x0001 = Append
		// 0x1000 = plain text (2005.2.20 yutaka)
		// 0x2000 = timestamp (2006.7.23 maya)
		// 0x4000 = hide file transfer dialog (2008.1.30 maya)
		// 0x8000 = Include screen buffer (2013.9.29 yutaka)
		// teraterm.iniの設定を見てからデフォルトオプションを決める。(2005.5.7 yutaka)
		Option = MAKELONG(ts.LogBinary,
		                  ts.Append |
		                  (0x1000 * ts.LogTypePlainText) |
		                  (0x2000 * ts.LogTimestamp) |
		                  (0x4000 * ts.LogHideDialog) |
		                  (0x8000 * ts.LogAllBuffIncludedInFirst)
		         );

		// ログのデフォルトファイル名を設定 (2006.8.28 maya)
		strncat_s(LogVar->FullName, sizeof(LogVar->FullName), ts.LogDefaultName, _TRUNCATE);

		ParseStrftimeFileName(LogVar->FullName, sizeof(LogVar->FullName));

		// &h をホスト名に置換 (2007.5.14)
		ConvertLogname(LogVar->FullName, sizeof(LogVar->FullName));

		strncpy_s(LogVar->LogDefaultPath, sizeof(LogVar->LogDefaultPath), ts.LogDefaultPath, _TRUNCATE);
		if (! (*GetTransFname)(LogVar, logdir, GTF_LOG, &Option))
		{
			FreeFileVar(&LogVar);
			FreeTTFILE();
			return FALSE;
		}
		ts.LogBinary = LOWORD(Option);
		ts.Append = HIWORD(Option);

		if (ts.Append & 0x1000) {
			ts.LogTypePlainText = 1;
		} else {
			ts.LogTypePlainText = 0;
		}

		if (ts.Append & 0x2000) {
			ts.LogTimestamp = 1;
		}
		else {
			ts.LogTimestamp = 0;
		}

		if (ts.Append & 0x4000) {
			ts.LogHideDialog = 1;
		}
		else {
			ts.LogHideDialog = 0;
		}

		if (ts.Append & 0x8000) {
			ts.LogAllBuffIncludedInFirst = 1;
		}
		else {
			ts.LogAllBuffIncludedInFirst = 0;
		}

		ts.Append &= 0x1; // 1bitにマスクする

	}
	else {
		// LogVar->DirLen = 0 だとここに来る
		// フルパス・相対パスともに LogVar->FullName に入れておく必要がある
		char FileName[MAX_PATH];

		// フルパス化
		strncpy_s(FileName, sizeof(FileName), LogVar->FullName, _TRUNCATE);
		ConvFName(logdir,FileName,sizeof(FileName),"",LogVar->FullName,sizeof(LogVar->FullName));

		ParseStrftimeFileName(LogVar->FullName, sizeof(LogVar->FullName));

		// &h をホスト名に置換 (2007.5.14)
		ConvertLogname(LogVar->FullName, sizeof(LogVar->FullName));
		(*SetFileVar)(LogVar);

		FixLogOption();
	}

	if (ts.LogBinary > 0)
	{
		BinLog = TRUE;
		FileLog = FALSE;
		if (! CreateBinBuf())
		{
			FileTransEnd(OpLog);
			return FALSE;
		}
	}
	else {
		BinLog = FALSE;
		FileLog = TRUE;
		if (! CreateLogBuf())
		{
			FileTransEnd(OpLog);
			return FALSE;
		}
	}
	cv.LStart = cv.LogPtr;
	cv.LCount = 0;
	if (ts.LogHideDialog)
		LogVar->HideDialog = 1;

	HelpId = HlpFileLog;
	/* 2007.05.24 Gentaro */
	eLineEnd = Line_LineHead;

	if (ts.Append > 0)
	{
		int dwShareMode = FILE_SHARE_READ;
		if (!ts.LogLockExclusive) {
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		LogVar->FileHandle = (int)CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
		                                     OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (LogVar->FileHandle>0){
#ifdef FileVarWin16
			_llseek(LogVar->FileHandle,0,2);
#else
			SetFilePointer((HANDLE)LogVar->FileHandle, 0, NULL, FILE_END);
#endif
			/* 2007.05.24 Gentaro
				If log file already exists,
				a newline is inserted before the first timestamp.
			*/
			eLineEnd = Line_FileHead;
		}
	}
	else {
		int dwShareMode = FILE_SHARE_READ;
		if (!ts.LogLockExclusive) {
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		LogVar->FileHandle = (int)CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
		                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	LogVar->FileOpen = (LogVar->FileHandle>0);
	if (! LogVar->FileOpen)
	{
		char msg[128];

		// ファイルオープンエラー時のメッセージ表示を追加した。(2008.7.9 yutaka)
		if (LogVar->NoMsg == FALSE) {
			_snprintf_s(msg, sizeof(msg), _TRUNCATE, "Can not create a `%s' file. (%d)", LogVar->FullName, GetLastError());
			MessageBox(NULL, msg, "Tera Term: File open error", MB_OK | MB_ICONERROR);
		}

		FileTransEnd(OpLog);
		return FALSE;
	}
	LogVar->ByteCount = 0;

	// Log rotate configuration
	LogVar->RotateMode = ts.LogRotate;
	LogVar->RotateSize = ts.LogRotateSize;
	LogVar->RotateStep = ts.LogRotateStep;

	// Log rotateが有効の場合、初期ファイルサイズを設定する。
	// 最初のファイルが設定したサイズでローテートしない問題の修正。
	// (2016.4.9 yutaka)
	if (LogVar->RotateMode != ROTATE_NONE) {
		size = GetFileSize((HANDLE)LogVar->FileHandle, NULL);
		if (size != -1)
			LogVar->ByteCount = size;
	}

	if (! OpenFTDlg(LogVar)) {
		FileTransEnd(OpLog);
		return FALSE;
	}

	// 遅延書き込み用スレッドを起こす。
	// (2013.4.19 yutaka)
	// DeferredLogWriteThread スレッドが起床して、スレッドキューが作成されるより前に、
	// ログファイルのクローズ(CloseFileSync)が行われると、エンキューが失敗し、デッドロック
	// するという問題を修正した。
	// スレッド間の同期を行うため、名前なしイベントオブジェクトを使って、スレッドキューの
	// 作成まで待ち合わせするようにした。名前付きイベントオブジェクトを使う場合は、
	// システム(Windows OS)上でユニークな名前にする必要がある。
	// (2016.9.23 yutaka)
	LogVar->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	LogVar->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, LogVar, 0, &tid);
	LogVar->LogThreadId = tid;
	if (LogVar->LogThreadEvent != NULL) {
		WaitForSingleObject(LogVar->LogThreadEvent, INFINITE);
		CloseHandle(LogVar->LogThreadEvent);
	}

	// 現在バッファにあるデータをすべて書き出してから、
	// ログ採取を開始する。
	// (2013.9.29 yutaka)
	if (ts.LogAllBuffIncludedInFirst) {
		for (ofs = 0 ;  ; ofs++ ) {
			// 1つの行を取得する。文字だけなので、エスケープシーケンスは含まれない。
			size = BuffGetAnyLineData(ofs, buf, sizeof(buf));
			if (size == -1)
				break;

#if 0
			if (ts.DeferredLogWriteMode) { // 遅延書き込み
				char *pbuf = (char *)malloc(size + 2);
				memcpy(pbuf, buf, size);
				pbuf[size] = '\r';
				pbuf[size+1] = '\n';
				Sleep(1);  // スレッドキューが作られるように、コンテキストスイッチを促す。
				PostThreadMessage(LogVar->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)pbuf, size + 2);
			} else { // 直書き。ネットワーク経由だと遅い。
#endif
				WriteFile((HANDLE)LogVar->FileHandle, buf, size, &written_size, NULL);
				WriteFile((HANDLE)LogVar->FileHandle, crlf, crlf_len, &written_size, NULL);
#if 0
			}
#endif
		}
	}

	return TRUE;
}
}

void LogPut1(BYTE b)
{
	LogLast = b;
	cv.LogBuf[cv.LogPtr] = b;
	cv.LogPtr++;
	if (cv.LogPtr>=InBuffSize)
		cv.LogPtr = cv.LogPtr-InBuffSize;

	if (FileLog)
	{
		if (cv.LCount>=InBuffSize)
		{
			cv.LCount = InBuffSize;
			cv.LStart = cv.LogPtr;
		}
		else
			cv.LCount++;
	}
	else
		cv.LCount = 0;

	if (DDELog)
	{
		if (cv.DCount>=InBuffSize)
		{
			cv.DCount = InBuffSize;
			cv.DStart = cv.LogPtr;
		}
		else
			cv.DCount++;
	}
	else {
		cv.DCount = 0;
		// ログ採取中にマクロがストールする問題への修正。
		// ログ採取中に一度マクロを止めると、バッファのインデックスが同期取れなくなり、
		// 再度マクロを流しても正しいデータが送れないのが原因。
		// マクロを停止させた状態でもインデックスの同期を取るようにした。
		// (2006.12.26 yutaka)
		cv.DStart = cv.LogPtr;
	}
}

void Log1Byte(BYTE b)
{
	if (b==0x0d)
	{
		LogLast = b;
		return;
	}
	if ((b==0x0a) && (LogLast==0x0d))
		LogPut1(0x0d);
	LogPut1(b);
}

static BOOL Get1(PCHAR Buf, int *Start, int *Count, PBYTE b)
{
	if (*Count<=0) return FALSE;
	*b = Buf[*Start];
	(*Start)++;
	if (*Start>=InBuffSize)
		*Start = *Start-InBuffSize;
	(*Count)--;
	return TRUE;
}



static CRITICAL_SECTION g_filelog_lock;   /* ロック用変数 */

void logfile_lock_initialize(void)
{
	InitializeCriticalSection(&g_filelog_lock);
}

static inline void logfile_lock(void)
{
	EnterCriticalSection(&g_filelog_lock);
}

static inline void logfile_unlock(void)
{
	LeaveCriticalSection(&g_filelog_lock);
}

 // コメントをログへ追加する
void CommentLogToFile(char *buf, int size)
{
	DWORD wrote;

	if (LogVar == NULL || !LogVar->FileOpen) {
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_COMMENT_LOG_OPEN_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "It is not opened by the log file yet.", ts.UILanguageFile);
		::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	logfile_lock();
	WriteFile((HANDLE)LogVar->FileHandle, buf, size, &wrote, NULL);
	WriteFile((HANDLE)LogVar->FileHandle, "\r\n", 2, &wrote, NULL); // 改行
	/* Set Line End Flag
		2007.05.24 Gentaro
	*/
	eLineEnd = Line_LineHead;
	logfile_unlock();
}

// ログをローテートする。
// (2013.3.21 yutaka)
static void LogRotate(void)
{
	int loopmax = 10000;  // XXX
	char filename[1024];
	char newfile[1024], oldfile[1024];
	int i, k;
	int dwShareMode = FILE_SHARE_READ;
	unsigned tid;

	if (! LogVar->FileOpen) return;

	if (LogVar->RotateMode == ROTATE_NONE)
		return;

	if (LogVar->RotateMode == ROTATE_SIZE) {
		if (LogVar->ByteCount <= LogVar->RotateSize)
			return;
		//OutputDebugPrintf("%s: mode %d size %ld\n", __FUNCTION__, LogVar->RotateMode, LogVar->ByteCount);
	} else {
		return;
	}

	logfile_lock();
	// ログサイズを再初期化する。
	LogVar->ByteCount = 0;

	// いったん今のファイルをクローズして、別名のファイルをオープンする。
	CloseFileSync(LogVar);
	//_lclose(LogVar->FileHandle);

	// 世代ローテーションのステップ数の指定があるか
	if (LogVar->RotateStep > 0)
		loopmax = LogVar->RotateStep;

	for (i = 1 ; i <= loopmax ; i++) {
		_snprintf_s(filename, sizeof(filename), _TRUNCATE, "%s.%d", LogVar->FullName, i);
		if (_access_s(filename, 0) != 0)
			break;
	}
	if (i > loopmax) {
		// 世代がいっぱいになったら、最古のファイルから廃棄する。
		i = loopmax;
	}

	// 別ファイルにリネーム。
	for (k = i-1 ; k >= 0 ; k--) {
		if (k == 0)
			strncpy_s(oldfile, sizeof(oldfile), LogVar->FullName, _TRUNCATE);
		else
			_snprintf_s(oldfile, sizeof(oldfile), _TRUNCATE, "%s.%d", LogVar->FullName, k);
		_snprintf_s(newfile, sizeof(newfile), _TRUNCATE, "%s.%d", LogVar->FullName, k+1);
		remove(newfile);
		if (rename(oldfile, newfile) != 0) {
			OutputDebugPrintf("%s: rename %d\n", __FUNCTION__, errno);
		}
	}

	// 再オープン
	if (!ts.LogLockExclusive) {
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	LogVar->FileHandle = (int)CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
	                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// 遅延書き込み用スレッドを起こす。
	// (2013.4.19 yutaka)
	// DeferredLogWriteThread スレッドが起床して、スレッドキューが作成されるより前に、
	// ログファイルのクローズ(CloseFileSync)が行われると、エンキューが失敗し、デッドロック
	// するという問題を修正した。
	// スレッド間の同期を行うため、名前なしイベントオブジェクトを使って、スレッドキューの
	// 作成まで待ち合わせするようにした。名前付きイベントオブジェクトを使う場合は、
	// システム(Windows OS)上でユニークな名前にする必要がある。
	// (2016.9.26 yutaka)
	LogVar->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	LogVar->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, LogVar, 0, &tid);
	LogVar->LogThreadId = tid;
	if (LogVar->LogThreadEvent != NULL) {
		WaitForSingleObject(LogVar->LogThreadEvent, INFINITE);
		CloseHandle(LogVar->LogThreadEvent);
	}

	logfile_unlock();

}

void LogToFile()
{
	PCHAR Buf;
	int Start, Count;
	BYTE b;
	PCHAR WriteBuf;
	DWORD WriteBufMax, WriteBufLen;
	CHAR tmp[128];
	DWORD wrote;

	if (! LogVar->FileOpen) return;
	if (FileLog)
	{
		Buf = cv.LogBuf;
		Start = cv.LStart;
		Count = cv.LCount;
	}
	else if (BinLog)
	{
		Buf = cv.BinBuf;
		Start = cv.BStart;
		Count = cv.BCount;
	}
	else
		return;

	if (Buf==NULL) return;
	if (Count==0) return;

	// ロックを取る(2004.8.6 yutaka)
	logfile_lock();

	if (ts.DeferredLogWriteMode) {
		WriteBufMax = 8192;
		WriteBufLen = 0;
		WriteBuf = (PCHAR)malloc(WriteBufMax);
		while (Get1(Buf,&Start,&Count,&b)) {
			if (((cv.FilePause & OpLog)==0) && (! cv.ProtoFlag))
			{
				tmp[0] = 0;
				if ( ts.LogTimestamp && eLineEnd ) {
					char *strtime = mctimelocal(ts.LogTimestampFormat, ts.LogTimestampUTC);
					/* 2007.05.24 Gentaro */
					if( eLineEnd == Line_FileHead ){
						strncat_s(tmp, sizeof(tmp), "\r\n", _TRUNCATE);
					}
					strncat_s(tmp, sizeof(tmp), "[", _TRUNCATE);
					strncat_s(tmp, sizeof(tmp), strtime, _TRUNCATE);
					strncat_s(tmp, sizeof(tmp), "] ", _TRUNCATE);
				}

				/* 2007.05.24 Gentaro */
				if( b == 0x0a ){
					eLineEnd = Line_LineHead; /* set endmark*/
				}
				else {
					eLineEnd = Line_Other; /* clear endmark*/
				}

				if (WriteBufLen >= (WriteBufMax*4/5)) {
					WriteBufMax *= 2;
					WriteBuf = (PCHAR)realloc(WriteBuf, WriteBufMax);
				}
				memcpy(&WriteBuf[WriteBufLen], tmp, strlen(tmp));
				WriteBufLen += strlen(tmp);
				WriteBuf[WriteBufLen++] = b;

				(LogVar->ByteCount)++;
			}
		}

		PostThreadMessage(LogVar->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)WriteBuf, WriteBufLen);

	} else {

		while (Get1(Buf,&Start,&Count,&b))
		{
			if (((cv.FilePause & OpLog)==0) && (! cv.ProtoFlag))
			{
				// 時刻を書き出す(2006.7.23 maya)
				// 日付フォーマットを日本ではなく世界標準に変更した (2006.7.23 yutaka)
				/* 2007.05.24 Gentaro */
				// ミリ秒も表示するように変更 (2009.5.23 maya)
				if ( ts.LogTimestamp && eLineEnd ) {
	#if 1
	#if 0
					SYSTEMTIME	LocalTime;
					GetLocalTime(&LocalTime);
					char strtime[27];

					// format time
					sprintf(strtime, "[%04d/%02d/%02d %02d:%02d:%02d.%03d] ",
							LocalTime.wYear, LocalTime.wMonth,LocalTime.wDay,
							LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond,
							LocalTime.wMilliseconds);
	#else
					char *strtime = mctimelocal(ts.LogTimestampFormat, ts.LogTimestampUTC);
	#endif
	#else
						time_t tick = time(NULL);
						char *strtime = ctime(&tick);
	#endif
					/* 2007.05.24 Gentaro */
					if( eLineEnd == Line_FileHead ){
#ifdef FileVarWin16
						_lwrite(LogVar->FileHandle,"\r\n",2);
#else
						WriteFile((HANDLE)LogVar->FileHandle, "\r\n", 2, &wrote, NULL);
#endif
					}
#ifdef FileVarWin16
					_lwrite(LogVar->FileHandle,"[",1);
					_lwrite(LogVar->FileHandle, strtime, strlen(strtime));
					_lwrite(LogVar->FileHandle,"] ",2);
#else
					WriteFile((HANDLE)LogVar->FileHandle, "[", 1, &wrote, NULL);
					WriteFile((HANDLE)LogVar->FileHandle, strtime, strlen(strtime), &wrote, NULL);
					WriteFile((HANDLE)LogVar->FileHandle, "] ", 2, &wrote, NULL);
#endif
				}

				/* 2007.05.24 Gentaro */
				if( b == 0x0a ){
					eLineEnd = Line_LineHead; /* set endmark*/
				}
				else {
					eLineEnd = Line_Other; /* clear endmark*/
				}

#ifdef FileVarWin16
				_lwrite(LogVar->FileHandle,(PCHAR)&b,1);
#else
				WriteFile((HANDLE)LogVar->FileHandle, (PCHAR)&b, 1, &wrote, NULL);
#endif
				(LogVar->ByteCount)++;
			}
		}

	}

	logfile_unlock();

	if (FileLog)
	{
		cv.LStart = Start;
		cv.LCount = Count;
	}
	else {
		cv.BStart = Start;
		cv.BCount = Count;
	}
	if (((cv.FilePause & OpLog) !=0) || cv.ProtoFlag) return;
	if (FLogDlg!=NULL)
		FLogDlg->RefreshNum();

	// ログ・ローテート
	LogRotate();

}

BOOL CreateLogBuf()
{
	if (cv.HLogBuf==NULL)
	{
		cv.HLogBuf = GlobalAlloc(GMEM_MOVEABLE,InBuffSize);
		cv.LogBuf = NULL;
		cv.LogPtr = 0;
		cv.LStart = 0;
		cv.LCount = 0;
		cv.DStart = 0;
		cv.DCount = 0;
	}
	return (cv.HLogBuf!=NULL);
}

void FreeLogBuf()
{
	if ((cv.HLogBuf==NULL) || FileLog || DDELog)
		return;
	if (cv.LogBuf!=NULL)
		GlobalUnlock(cv.HLogBuf);
	GlobalFree(cv.HLogBuf);
	cv.HLogBuf = NULL;
	cv.LogBuf = NULL;
	cv.LogPtr = 0;
	cv.LStart = 0;
	cv.LCount = 0;
	cv.DStart = 0;
	cv.DCount = 0;
}

BOOL CreateBinBuf()
{
	if (cv.HBinBuf==NULL)
	{
		cv.HBinBuf = GlobalAlloc(GMEM_MOVEABLE,InBuffSize);
		cv.BinBuf = NULL;
		cv.BinPtr = 0;
		cv.BStart = 0;
		cv.BCount = 0;
	}
	return (cv.HBinBuf!=NULL);
}

void FreeBinBuf()
{
	if ((cv.HBinBuf==NULL) || BinLog)
		return;
	if (cv.BinBuf!=NULL)
		GlobalUnlock(cv.HBinBuf);
	GlobalFree(cv.HBinBuf);
	cv.HBinBuf = NULL;
	cv.BinBuf = NULL;
	cv.BinPtr = 0;
	cv.BStart = 0;
	cv.BCount = 0;
}

extern "C" {
void FileSendStart()
{
	LONG Option;

	if (! cv.Ready || FSend) return;
	if (cv.ProtoFlag)
	{
		FreeFileVar(&SendVar);
		return;
	}

	if (! LoadTTFILE())
		return;
	if (! NewFileVar(&SendVar))
	{
		FreeTTFILE();
		return;
	}
	SendVar->OpId = OpSendFile;

	FSend = TRUE;

	if (strlen(&(SendVar->FullName[SendVar->DirLen]))==0)
	{
		Option = MAKELONG(ts.TransBin,0);
		SendVar->FullName[0] = 0;
		if (! (*GetTransFname)(SendVar, ts.FileDir, GTF_SEND, &Option))
		{
			FileTransEnd(OpSendFile);
			return;
		}
		ts.TransBin = LOWORD(Option);
	}
	else
		(*SetFileVar)(SendVar);

#ifdef FileVarWin16
	SendVar->FileHandle = _lopen(SendVar->FullName,OF_READ);
#else
	SendVar->FileHandle = (int)CreateFile(SendVar->FullName, GENERIC_READ, FILE_SHARE_READ, NULL,
	                                      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#endif
	SendVar->FileOpen = (SendVar->FileHandle>0);
	if (! SendVar->FileOpen)
	{
		FileTransEnd(OpSendFile);
		return;
	}
	SendVar->ByteCount = 0;
	SendVar->FileSize = GetFSize(SendVar->FullName);

	TalkStatus = IdTalkFile;
	FileRetrySend = FALSE;
	FileRetryEcho = FALSE;
	FileCRSend = FALSE;
	FileReadEOF = FALSE;
	FileSendHandler.pos = 0;
	FileSendHandler.end = 0;
	FileDlgRefresh = 0;

	if (BracketedPasteMode()) {
		FileBracketMode = FS_BRACKET_START;
		FileBracketPtr = 0;
		BinaryMode = TRUE;
	}
	else {
		FileBracketMode = FS_BRACKET_NONE;
		BinaryMode = ts.TransBin;
	}

	if (! OpenFTDlg(SendVar))
		FileTransEnd(OpSendFile);
	}
}

void FileTransEnd(WORD OpId)
/* OpId = 0: close Log and FileSend
      OpLog: close Log
 OpSendFile: close FileSend */
{
	if (((OpId==0) || (OpId==OpLog)) && (FileLog || BinLog))
	{
		FileLog = FALSE;
		BinLog = FALSE;
		if (FLogDlg!=NULL)
		{
			FLogDlg->DestroyWindow();
			FLogDlg = NULL;
			HWndLog = NULL; // steven add
		}
		FreeFileVar(&LogVar);
		FreeLogBuf();
		FreeBinBuf();
		FreeTTFILE();
	}

	if (((OpId==0) || (OpId==OpSendFile)) && FSend)
	{
		FSend = FALSE;
		TalkStatus = IdTalkKeyb;
		if (SendDlg!=NULL)
		{
			SendDlg->DestroyWindow();
			SendDlg = NULL;
		}
		FreeFileVar(&SendVar);
		FreeTTFILE();
	}

	EndDdeCmnd(0);
}

int FSOut1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryOut(&cv,(PCHAR)&b,1);
	else if ((b>=0x20) || (b==0x09) || (b==0x0A) || (b==0x0D))
		return CommTextOut(&cv,(PCHAR)&b,1);
	else
		return 1;
}

int FSEcho1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	else
		return CommTextEcho(&cv,(PCHAR)&b,1);
}

extern "C" {
// 以下の時はこちらの関数を使う
// - BinaryMode == true
// - FileBracketMode == false
// - cv.TelFlag == false
// - ts.LocalEcho == 0
void FileSendBinayBoost()
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if ((SendDlg == NULL) ||
		((cv.FilePause & OpSendFile) != 0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
			FileSendHandler.end - FileSendHandler.pos);
		FileSendHandler.pos += c;
		FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
		if (FileRetrySend)
			return;
	}

	do {
		if (FileSendHandler.pos == FileSendHandler.end) {
#ifdef FileVarWin16
			fc = _lread(SendVar->FileHandle, &(FileSendHandler.buf[0]), sizeof(FileSendHandler.buf));
#else
			ReadFile((HANDLE)SendVar->FileHandle, &(FileSendHandler.buf[0]), sizeof(FileSendHandler.buf), &read_bytes, NULL);
			fc = LOWORD(read_bytes);
#endif
			FileSendHandler.pos = 0;
			FileSendHandler.end = fc;
		} else {
			fc = FileSendHandler.end - FileSendHandler.end;
		}

		if (fc != 0)
		{
			c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
				FileSendHandler.end - FileSendHandler.pos);
			FileSendHandler.pos += c;
			FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
			SendVar->ByteCount = SendVar->ByteCount + c;
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld)
					SendDlg->RefreshNum();
				return;
			}
		}
		FileDlgRefresh = SendVar->ByteCount;
		SendDlg->RefreshNum();
		BCOld = SendVar->ByteCount;
		if (fc != 0)
			return;
	} while (fc != 0);

	FileTransEnd(OpSendFile);
}
}

extern "C" {
void FileSend()
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if (cv.PortType == IdSerial && ts.FileSendHighSpeedMode &&
	    BinaryMode && !FileBracketMode && !cv.TelFlag &&
	    (ts.LocalEcho == 0) && (ts.Baud >= 115200)) {
		return FileSendBinayBoost();
	}

	if ((SendDlg==NULL) ||
	    ((cv.FilePause & OpSendFile) !=0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		FileRetryEcho = (ts.LocalEcho>0);
		c = FSOut1(FileByte);
		FileRetrySend = (c==0);
		if (FileRetrySend)
			return;
	}

	if (FileRetryEcho)
	{
		c = FSEcho1(FileByte);
		FileRetryEcho = (c==0);
		if (FileRetryEcho)
			return;
	}

	do {
		if (FileBracketMode == FS_BRACKET_START) {
			FileByte = BracketStartStr[FileBracketPtr++];
			fc = 1;

			if (FileBracketPtr >= sizeof(BracketStartStr) - 1) {
				FileBracketMode = FS_BRACKET_END;
				FileBracketPtr = 0;
				BinaryMode = ts.TransBin;
			}
		}
		else if (! FileReadEOF) {
#ifdef FileVarWin16
			fc = _lread(SendVar->FileHandle,&FileByte,1);
#else
			ReadFile((HANDLE)SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
			fc = LOWORD(read_bytes);
#endif
			SendVar->ByteCount = SendVar->ByteCount + fc;

			if (FileCRSend && (fc==1) && (FileByte==0x0A)) {
#ifdef FileVarWin16
				fc = _lread(SendVar->FileHandle,&FileByte,1);
#else
				ReadFile((HANDLE)SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
				fc = LOWORD(read_bytes);
#endif
				SendVar->ByteCount = SendVar->ByteCount + fc;
			}
		}
		else {
			fc = 0;
		}

		if (fc == 0 && FileBracketMode == FS_BRACKET_END) {
			FileReadEOF = TRUE;
			FileByte = BracketEndStr[FileBracketPtr++];
			fc = 1;
			BinaryMode = TRUE;

			if (FileBracketPtr >= sizeof(BracketEndStr) - 1) {
				FileBracketMode = FS_BRACKET_NONE;
				FileBracketPtr = 0;
			}
		}


		if (fc!=0)
		{
			c = FSOut1(FileByte);
			FileCRSend = (ts.TransBin==0) && (FileByte==0x0D);
			FileRetrySend = (c==0);
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld)
					SendDlg->RefreshNum();
				return;
			}
			if (ts.LocalEcho>0)
			{
				c = FSEcho1(FileByte);
				FileRetryEcho = (c==0);
				if (FileRetryEcho)
					return;
			}
		}
		if ((fc==0) || ((SendVar->ByteCount % 100 == 0) && (FileBracketPtr == 0))) {
			SendDlg->RefreshNum();
			BCOld = SendVar->ByteCount;
			if (fc!=0)
				return;
		}
	} while (fc!=0);

	FileTransEnd(OpSendFile);
}
}

extern "C" {
void FLogChangeButton(BOOL Pause)
{
	if (FLogDlg!=NULL)
		FLogDlg->ChangeButton(Pause);
}
}

extern "C" {
void FLogRefreshNum()
{
	if (FLogDlg!=NULL)
		FLogDlg->RefreshNum();
}
}

BOOL OpenProtoDlg(PFileVar fv, int IdProto, int Mode, WORD Opt1, WORD Opt2)
{
	int vsize;
	PProtoDlg pd;
	HWND Hpd;
	char uimsg[MAX_UIMSG];

	ProtoId = IdProto;

	switch (ProtoId) {
		case PROTO_KMT:
			vsize = sizeof(TKmtVar);
			break;
		case PROTO_XM:
			vsize = sizeof(TXVar);
			break;
		case PROTO_YM:
			vsize = sizeof(TYVar);
			break;
		case PROTO_ZM:
			vsize = sizeof(TZVar);
			break;
		case PROTO_BP:
			vsize = sizeof(TBPVar);
			break;
		case PROTO_QV:
			vsize = sizeof(TQVVar);
			break;
	}
	ProtoVar = (PCHAR)malloc(vsize);
	if (ProtoVar==NULL)
		return FALSE;

	switch (ProtoId) {
		case PROTO_KMT:
			((PKmtVar)ProtoVar)->KmtMode = Mode;
			break;
		case PROTO_XM:
			((PXVar)ProtoVar)->XMode = Mode;
			((PXVar)ProtoVar)->XOpt = Opt1;
			((PXVar)ProtoVar)->TextFlag = 1 - (Opt2 & 1);
			break;
		case PROTO_YM:
			((PYVar)ProtoVar)->YMode = Mode;
			((PYVar)ProtoVar)->YOpt = Opt1;
			break;
		case PROTO_ZM:
			((PZVar)ProtoVar)->BinFlag = (Opt1 & 1) != 0;
			((PZVar)ProtoVar)->ZMode = Mode;
			break;
		case PROTO_BP:
			((PBPVar)ProtoVar)->BPMode = Mode;
			break;
		case PROTO_QV:
			((PQVVar)ProtoVar)->QVMode = Mode;
			break;
	}

	pd = new CProtoDlg();
	if (pd==NULL)
	{
		free(ProtoVar);
		ProtoVar = NULL;
		return FALSE;
	}
	pd->Create(fv,&ts);

	Hpd=pd->GetSafeHwnd();

	GetDlgItemText(Hpd, IDC_PROT_FILENAME, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_PROT_FILENAME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDC_PROT_FILENAME, ts.UIMsg);
	GetDlgItemText(Hpd, IDC_PROT_PROT, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_PROT_PROTO", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDC_PROT_PROT, ts.UIMsg);
	GetDlgItemText(Hpd, IDC_PROT_PACKET, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_PROT_PACKET", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDC_PROT_PACKET, ts.UIMsg);
	GetDlgItemText(Hpd, IDC_PROT_TRANS, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_PROT_TRANS", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDC_PROT_TRANS, ts.UIMsg);
	GetDlgItemText(Hpd, IDC_PROT_ELAPSED, uimsg, sizeof(uimsg));
	get_lang_msg("DLG_PROT_ELAPSED", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDC_PROT_ELAPSED, ts.UIMsg);
	GetDlgItemText(Hpd, IDCANCEL, uimsg, sizeof(uimsg));
	get_lang_msg("BTN_CANCEL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
	SetDlgItemText(Hpd, IDCANCEL, ts.UIMsg);

	(*ProtoInit)(ProtoId,FileVar,ProtoVar,&cv,&ts);

	PtDlg = pd;
	return TRUE;
}

extern "C" {
void CloseProtoDlg()
{
	if (PtDlg!=NULL)
	{
		PtDlg->DestroyWindow();
		PtDlg = NULL;

		::KillTimer(FileVar->HMainWin,IdProtoTimer);
		if ((ProtoId==PROTO_QV) &&
		    (((PQVVar)ProtoVar)->QVMode==IdQVSend))
			CommTextOut(&cv,"\015",1);
		if (FileVar->LogFlag)
			_lclose(FileVar->LogFile);
		FileVar->LogFile = 0;
		if (ProtoVar!=NULL)
		{
			free(ProtoVar);
			ProtoVar = NULL;
		}
	}
}
}

BOOL ProtoStart()
{
	if (cv.ProtoFlag)
		return FALSE;
	if (FSend)
	{
		FreeFileVar(&FileVar);
		return FALSE;
	}

	if (! LoadTTFILE())
		return FALSE;
	NewFileVar(&FileVar);

	if (FileVar==NULL)
	{
		FreeTTFILE();
		return FALSE;
	}
	cv.ProtoFlag = TRUE;
	return TRUE;
}

void ProtoEnd()
{
	if (! cv.ProtoFlag)
		return;
	cv.ProtoFlag = FALSE;

	/* Enable transmit delay (serial port) */
	cv.DelayFlag = TRUE;
	TalkStatus = IdTalkKeyb;

	CloseProtoDlg();

	if ((FileVar!=NULL) && FileVar->Success)
		EndDdeCmnd(1);
	else
		EndDdeCmnd(0);

	FreeTTFILE();
	FreeFileVar(&FileVar);
}

extern "C" {
int ProtoDlgParse()
{
	int P;

	P = ActiveWin;
	if (PtDlg==NULL)
		return P;

	if ((*ProtoParse)(ProtoId,FileVar,ProtoVar,&cv))
		P = 0; /* continue */
	else {
		CommSend(&cv);
		ProtoEnd();
	}
	return P;
}
}

extern "C" {
void ProtoDlgTimeOut()
{
	if (PtDlg!=NULL)
		(*ProtoTimeOutProc)(ProtoId,FileVar,ProtoVar,&cv);
}
}

extern "C" {
void ProtoDlgCancel()
{
	if ((PtDlg!=NULL) &&
	    (*ProtoCancel)(ProtoId,FileVar,ProtoVar,&cv))
		ProtoEnd();
}
}

extern "C" {
void KermitStart(int mode)
{
	WORD w;

	if (! ProtoStart())
		return;

	switch (mode) {
		case IdKmtSend:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_KERMIT,&w) ||
				    (FileVar->NumFname==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtReceive:
			FileVar->OpId = OpKmtRcv;
			break;
		case IdKmtGet:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (! (*GetGetFname)(FileVar->HMainWin,FileVar) ||
				    (strlen(FileVar->FullName)==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtFinish:
			FileVar->OpId = OpKmtFin;
			break;
		default:
			ProtoEnd();
			return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_KMT,mode,0,0))
		ProtoEnd();
}
}

extern "C" {
void XMODEMStart(int mode)
{
	LONG Option;
	int tmp;

	if (! ProtoStart())
		return;

	if (mode==IdXReceive)
		FileVar->OpId = OpXRcv;
	else
		FileVar->OpId = OpXSend;

	if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
	{
		Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		if (! (*GetXFname)(FileVar->HMainWin,
		                   mode==IdXReceive,&Option,FileVar,ts.FileDir))
		{
			ProtoEnd();
			return;
		}
		tmp = HIWORD(Option);
		if (mode == IdXReceive) {
			if (IsXoptCRC(tmp)) {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = XoptCRC;
				}
			}
			else {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCksum;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
			ts.XmodemBin = LOWORD(Option);
		}
		else {
			if (IsXopt1k(tmp)) {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = Xopt1kCksum;
				}
			}
			else {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = XoptCRC;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
		}
	}
	else
		(*SetFileVar)(FileVar);

	if (mode==IdXReceive)
		FileVar->FileHandle = _lcreat(FileVar->FullName,0);
	else
		FileVar->FileHandle = _lopen(FileVar->FullName,OF_READ);

	FileVar->FileOpen = FileVar->FileHandle>0;
	if (! FileVar->FileOpen)
	{
		ProtoEnd();
		return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_XM,mode,
	                   ts.XmodemOpt,ts.XmodemBin))
		ProtoEnd();
}
}

extern "C" {
void YMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode==IdYSend)
	{
		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		// TODO: "Yopt1K", "YoptG", "YoptSingle"を区別したいならば、IDD_FOPTを拡張する必要あり。
		Opt = Yopt1K;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_Y,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			//ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else {
		FileVar->OpId = OpYRcv;
		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		Opt = Yopt1K;
		(*SetFileVar)(FileVar);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM,mode,Opt,0))
		ProtoEnd();
}
}

extern "C" {
void ZMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode == IdZSend || mode == IdZAutoS)
	{
		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_Z,&Opt) ||
			    (FileVar->NumFname==0))
			{
				if (mode == IdZAutoS) {
					CommRawOut(&cv, "\030\030\030\030\030\030\030\030\b\b\b\b\b\b\b\b\b\b", 18);
				}
				ProtoEnd();
				return;
			}
			ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else /* IdZReceive or IdZAutoR */
		FileVar->OpId = OpZRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0))
		ProtoEnd();
}
}

extern "C" {
void BPStart(int mode)
{
	LONG Option;

	if (! ProtoStart())
		return;
	if (mode==IdBPSend)
	{
		FileVar->OpId = OpBPSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			FileVar->FullName[0] = 0;
			if (! (*GetTransFname)(FileVar, ts.FileDir, GTF_BP, &Option))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else /* IdBPReceive or IdBPAuto */
		FileVar->OpId = OpBPRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP,mode,0,0))
		ProtoEnd();
}
}

extern "C" {
void QVStart(int mode)
{
	WORD W;

	if (! ProtoStart())
		return;

	if (mode==IdQVSend)
	{
		FileVar->OpId = OpQVSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_QV, &W) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else
		FileVar->OpId = OpQVRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,mode,0,0))
		ProtoEnd();
}
}
