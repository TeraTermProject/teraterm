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

#include "filesys.h"
#include "ftlib.h"

PFileVar LogVar = NULL;
PFileVar SendVar = NULL;
PFileVar FileVar = NULL;
static PCHAR ProtoVar = NULL;
static int ProtoId;

static BYTE LogLast = 0;
BOOL FileLog = FALSE;
BOOL BinLog = FALSE;
BOOL DDELog = FALSE;
static BOOL FileRetrySend, FileRetryEcho, FileCRSend;
static BYTE FileByte;

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
		InitDlgProgress(HFTDlg, IDC_TRANSPROGRESS, &fv->ProgStat);
	}
	else {
		fv->ProgStat = -1;
	}

	return (FTDlg!=NULL);
}

void ShowFTDlg(WORD OpId)
{
	if (OpId == OpLog) {
		if (FLogDlg != NULL) {
			FLogDlg->ShowWindow(SW_SHOWNORMAL);
		}
	}
	else {
		if (SendDlg != NULL) {
			SendDlg->ShowWindow(SW_SHOWNORMAL);
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
		if ((*fv)->FileOpen) _lclose((*fv)->FileHandle);
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

	memset(buf, 0, sizeof(buf));

	while(*p != '\0') {
		if (*p == '&' && *(p+1) != '\0') {
			switch (*(p+1)) {
			  case 'h':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						strncat_s(buf,sizeof(buf),ts.HostName,_TRUNCATE);
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
	if (ts.TransBin) {
		ts.LogTypePlainText = false;
		ts.LogTimestamp = false;
	}
}

extern "C" {
BOOL LogStart()
{
	LONG Option;
	char *logdir;

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
		// teraterm.iniの設定を見てからデフォルトオプションを決める。(2005.5.7 yutaka)
		Option = MAKELONG(ts.TransBin,
		                  ts.Append |
		                  (0x1000 * ts.LogTypePlainText) |
		                  (0x2000 * ts.LogTimestamp) |
		                  (0x4000 * ts.LogHideDialog));

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
		ts.TransBin = LOWORD(Option);
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

	if (ts.TransBin > 0)
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
		LogVar->FileHandle = _lopen(LogVar->FullName,OF_WRITE);
		if (LogVar->FileHandle>0){
			_llseek(LogVar->FileHandle,0,2);
			/* 2007.05.24 Gentaro
				If log file already exists,
				a newline is inserted before the first timestamp.
			*/
			eLineEnd = Line_FileHead;
		}
		else
			LogVar->FileHandle = _lcreat(LogVar->FullName,0);
	}
	else
		LogVar->FileHandle = _lcreat(LogVar->FullName,0);
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

	if (! OpenFTDlg(LogVar)) {
		FileTransEnd(OpLog);
		return FALSE;
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

void LogToFile()
{
	PCHAR Buf;
	int Start, Count;
	BYTE b;

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

	while (Get1(Buf,&Start,&Count,&b))
	{
		if (((cv.FilePause & OpLog)==0) && (! cv.ProtoFlag))
		{
			// 時刻を書き出す(2006.7.23 maya)
			// 日付フォーマットを日本ではなく世界標準に変更した (2006.7.23 yutaka)
			/* 2007.05.24 Gentaro */
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
				// ミリ秒も表示するように変更 (2009.5.23 maya)
				SYSTEMTIME LocalTime;
				GetLocalTime(&LocalTime);
				char strtime[29];
				char week[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
				char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
				_snprintf_s(strtime, sizeof(strtime), _TRUNCATE,
				            "%s %s %02d %02d:%02d:%02d.%03d %04d",
				            week[LocalTime.wDayOfWeek],
				            month[LocalTime.wMonth-1],
				            LocalTime.wDay,
				            LocalTime.wHour,
				            LocalTime.wMinute,
				            LocalTime.wSecond,
				            LocalTime.wMilliseconds,
				            LocalTime.wYear);
#endif
#else
					time_t tick = time(NULL);
					char *strtime = ctime(&tick); 
#endif
				/* 2007.05.24 Gentaro */
				if( eLineEnd == Line_FileHead ){
					_lwrite(LogVar->FileHandle,"\r\n",2);
				}
				_lwrite(LogVar->FileHandle,"[",1);
				_lwrite(LogVar->FileHandle, strtime, strlen(strtime));
				_lwrite(LogVar->FileHandle,"] ",2);
			}
			
			/* 2007.05.24 Gentaro */
			if( b == 0x0a ){
				eLineEnd = Line_LineHead; /* set endmark*/
			}
			else {
				eLineEnd = Line_Other; /* clear endmark*/
			}

			_lwrite(LogVar->FileHandle,(PCHAR)&b,1);
			(LogVar->ByteCount)++;
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

	SendVar->FileHandle = _lopen(SendVar->FullName,OF_READ);
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

// vtwin.cpp からコールしたいので、extern "C"を付ける。(2004.11.3 yutaka)
extern "C" {
int FSOut1(BYTE b)
{
	if (ts.TransBin > 0)
		return CommBinaryOut(&cv,(PCHAR)&b,1);
	else if ((b>=0x20) || (b==0x09) || (b==0x0A) || (b==0x0D))
		return CommTextOut(&cv,(PCHAR)&b,1);
	else
		return 1;
	}

int FSEcho1(BYTE b)
{
	if (ts.TransBin > 0)
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	else
		return CommTextEcho(&cv,(PCHAR)&b,1);
	}
}

extern "C" {
void FileSend()
{
	WORD c, fc;
	LONG BCOld;

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
		fc = _lread(SendVar->FileHandle,&FileByte,1);
		SendVar->ByteCount = SendVar->ByteCount + fc;

		if (FileCRSend && (fc==1) && (FileByte==0x0A))
		{
			fc = _lread(SendVar->FileHandle,&FileByte,1);
			SendVar->ByteCount = SendVar->ByteCount + fc;
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
		if ((fc==0) || (SendVar->ByteCount % 100 == 0))
		{
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
		case PROTO_YM:  // TBD
			((PYVar)ProtoVar)->YMode = Mode;
			((PYVar)ProtoVar)->YOpt = Yopt1K;  // TBD
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
	get_lang_msg("DLG_PROT_FIELNAME", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
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
		ts.XmodemOpt = HIWORD(Option);
		ts.XmodemBin = LOWORD(Option);
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
		Opt = ts.XmodemBin;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_Y,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else /* IdZReceive or IdZAuto */
		FileVar->OpId = OpYRcv;

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

	if (mode==IdZSend)
	{
		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,ts.FileDir,GMF_Z,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else /* IdZReceive or IdZAuto */
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
