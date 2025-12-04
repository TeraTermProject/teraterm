/*
 * (C) 2020- TeraTerm Project
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

/* TERATERM.EXE, log routines */
#include <stdio.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <process.h>
#include <windows.h>
#include <assert.h>

#include "tttypes.h"
#include "ftdlg.h"
#include "ttwinman.h"
#include "ttlib.h"
#include "ttlib_types.h"
#include "vtterm.h"
#include "codeconv.h"
#include "asprintf.h"
#include "makeoutputstring.h"

#include "filesys_log.h"
#include "filesys.h"  // for ProtoGetProtoFlag()

#define TitLog      L"Logging"

/*
   Line Head flag for timestamping
   2007.05.24 Gentaro
*/
enum enumLineEnd {
	Line_Other = 0,
	Line_LineHead = 1,
	Line_FileHead = 2,
};

typedef struct {
	wchar_t *FullName;

	HANDLE FileHandle;
	LONG FileSize;		// ? 使っていない
	LONG ByteCount;		// ファイルサイズ

	DWORD StartTime;

	enum enumLineEnd eLineEnd;

	// log rotate
	enum rotate_mode RotateMode;
	LONG RotateSize;
	int RotateStep;

	HANDLE LogThread;
	DWORD LogThreadId;
	HANDLE LogThreadEvent;

	BOOL IsPause;

	PFileTransDlg FLogDlg;

	LogCode_t log_code;
	BOOL bom;

	enum LogModeTag {
		NONE,
		BIN_MODE,
		TEXT_MODE,
	} LogMode;

	PCHAR cv_LogBuf;	// バッファの先頭
	int cv_LogPtr;		// 書き込み位置
	int cv_LStart;		// 読出し位置
	int cv_LCount;		// データ数
	int cv_BinSkip;
	int BuffSize;		// バッファサイズ (=InBuffSize)

	CRITICAL_SECTION filelog_lock;   /* ロック用変数 */
} TFileVar;
typedef TFileVar *PFileVar;

static PFileVar LogVar = NULL;

// 遅延書き込み用スレッドのメッセージ
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void OutputStr(PFileVar fv, const wchar_t *str);
static void LogToFile(PFileVar fv);
static void FLogOutputBOM(PFileVar fv);

static BOOL OpenFTDlg(PFileVar fv)
{
	PFileTransDlg FTDlg = new CFileTransDlg();
	if (FTDlg == NULL) {
		return FALSE;
	}

	wchar_t *DlgCaption;
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "FILEDLG_TRANS_TITLE_LOG", TitLog, ts.UILanguageFileW, &uimsg);
	aswprintf(&DlgCaption, L"Tera Term: %s", uimsg);
	free(uimsg);

	CFileTransDlg::Info info;
	info.UILanguageFileW = ts.UILanguageFileW;
	info.OpId = CFileTransDlg::OpLog;
	info.DlgCaption = DlgCaption;
	info.FileName = NULL;
	info.FullName = fv->FullName;
	info.HideDialog = ts.LogHideDialog ? TRUE : FALSE;
	info.HMainWin = HVTWin;
	FTDlg->Create(hInst, &info);
	FTDlg->RefreshNum(0, fv->FileSize, fv->ByteCount);

	fv->FLogDlg = FTDlg;

	free(DlgCaption);
	return TRUE;
}

/**
 *	ファイル名文字列の置き換え,ログ用
 *	次の文字を置き換える
 *		&h	ホスト名に置換
 *		&p	TCPポート番号に置換
 *		&u	ログオン中のユーザ名
 *
 *	@param	pcv
 *	@param	src	置き換える前の文字列(ファイル名)
 *	@return	置き換えられた文字列
 *			不要になったらfree()すること
 */
static wchar_t *ConvertLognameW(const TComVar *pcv, const wchar_t *src)
{
	const TTTSet *pts = pcv->ts;
	size_t dest_len = wcslen(src) + 1;
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * dest_len);

	const wchar_t *s = src;
	size_t i = 0;

	while(*s != '\0') {
		if (*s == '&' && *(s+1) != '\0') {
			wchar_t c = *(s+1);
			wchar_t *add_text = NULL;
			switch (c) {
			case 'h':
				s += 2;
				if (pcv->Open) {
					switch(pcv->PortType) {
					case IdTCPIP: {
						// ホスト名がIPv6アドレスだと、ファイル名に使用できない文字(:)が入るため置換
						wchar_t *host = ToWcharA(pts->HostName);
						wchar_t *host_fix = replaceInvalidFileNameCharW(host, '_');
						free(host);
						add_text = host_fix;
						break;
					}
					case IdSerial: {
						wchar_t *port;
						aswprintf(&port, L"COM%d", ts.ComPort);
						add_text = port;
						break;
					}
					default:
						;
					}
				}
				break;
			case 'p':
				s += 2;
				if (pcv->Open) {
					if (pcv->PortType == IdTCPIP) {
						wchar_t *port;
						aswprintf(&port, L"%d", ts.TCPPort);
						add_text = port;
					}
				}
				break;
			case 'u': {
				s += 2;
				wchar_t user[256 + 1];	// 256=UNLEN
				DWORD l = _countof(user);
				if (GetUserNameW(user, &l) != 0) {
					add_text = _wcsdup(user);
				}
				break;
			}
			default:
				// pass '&'
				s++;
				add_text = NULL;
				break;
			}

			if (add_text != NULL) {
				size_t l = wcslen(add_text);
				dest_len += l;
				dest = (wchar_t *)realloc(dest, sizeof(wchar_t) * dest_len);
				wcscpy(&dest[i], add_text);
				free(add_text);
				i += l;
			}
		}
		else {
			dest[i] = *s++;
			i++;
		}
	}
	dest[i] = 0;
	return dest;
}

static void FixLogOption(void)
{
	if (ts.LogBinary) {
		ts.LogTypePlainText = false;
		ts.LogTimestamp = false;
	}
}


// スレッドの終了とファイルのクローズ
static void CloseFileSync(PFileVar fv)
{
	BOOL ret;

	if (fv->FileHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	if (fv->LogThread != INVALID_HANDLE_VALUE) {
		// スレッドの終了待ち
		ret = PostThreadMessage(fv->LogThreadId, WM_QUIT, 0, 0);
		if (ret != 0) {
			// スレッドキューにエンキューできた場合のみ待ち合わせを行う。
			WaitForSingleObject(fv->LogThread, INFINITE);
		}
		else {
			//DWORD code = GetLastError();
		}
		CloseHandle(fv->LogThread);
		fv->LogThread = INVALID_HANDLE_VALUE;
	}
	CloseHandle(fv->FileHandle);
	fv->FileHandle = INVALID_HANDLE_VALUE;
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
				WriteFile(fv->FileHandle, buf, buflen, &wrote, NULL);
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

// 遅延書き込み用スレッドを起こす。
// (2013.4.19 yutaka)
// DeferredLogWriteThread スレッドが起床して、スレッドキューが作成されるより前に、
// ログファイルのクローズ(CloseFileSync)が行われると、エンキューが失敗し、デッドロック
// するという問題を修正した。
// スレッド間の同期を行うため、名前なしイベントオブジェクトを使って、スレッドキューの
// 作成まで待ち合わせするようにした。名前付きイベントオブジェクトを使う場合は、
// システム(Windows OS)上でユニークな名前にする必要がある。
// (2016.9.23 yutaka)
static void StartThread(PFileVar fv)
{
	unsigned tid;
	fv->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	fv->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, fv, 0, &tid);
	fv->LogThreadId = tid;
	if (fv->LogThreadEvent != NULL) {
		WaitForSingleObject(fv->LogThreadEvent, INFINITE);
		CloseHandle(fv->LogThreadEvent);
	}
}

static void OpenLogFile(PFileVar fv)
{
	// LogLockExclusive が有効な場合にまったく共有しないと、
	// 書き込み中のログファイルを他のエディタで開けないため
	int dwShareMode = FILE_SHARE_READ;
	if (!ts.LogLockExclusive) {
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	fv->FileHandle = CreateFileW(fv->FullName, GENERIC_WRITE, dwShareMode, NULL,
								 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static BOOL LogStart(PFileVar fv, const wchar_t *fname)
{
	fv->FullName = _wcsdup(fname);
	FixLogOption();

	fv->BuffSize = InBuffSize;
	fv->cv_LogBuf = (char *)malloc(fv->BuffSize);
	if (fv->cv_LogBuf == NULL) {
		return FALSE;
	}
	fv->cv_LogPtr = 0;
	fv->cv_LStart = 0;
	fv->cv_LCount = 0;
	if (ts.LogBinary > 0) {
		fv->LogMode = TFileVar::LogModeTag::BIN_MODE;
	} else {
		fv->LogMode = TFileVar::LogModeTag::TEXT_MODE;
	}

	OpenLogFile(fv);
	if (fv->FileHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	/* 2007.05.24 Gentaro */
	fv->eLineEnd = Line_LineHead;
	if (ts.Append > 0)
	{
		SetFilePointer(fv->FileHandle, 0, NULL, FILE_END);
		/* 2007.05.24 Gentaro
		   If log file already exists,
		   a newline is inserted before the first timestamp.
		*/
		fv->eLineEnd = Line_FileHead;
	}

	// BOM出力
	if (ts.Append == 0 && ts.LogBinary == 0 && fv->bom) {
		// 追記ではない(新規) && バイナリではない && BOM を出力時
		FLogOutputBOM(fv);
	}

	// Log rotate configuration
	fv->RotateMode = (enum rotate_mode)ts.LogRotate;
	fv->RotateSize = ts.LogRotateSize;
	fv->RotateStep = ts.LogRotateStep;

	if (fv->RotateMode != ROTATE_NONE) {
		// Log rotateが有効の場合、初期ファイルサイズを設定する。
		DWORD size = GetFileSize(fv->FileHandle, NULL);
		if (size == -1) {
			return FALSE;
		}
		fv->ByteCount = size;
	}
	else {
		fv->ByteCount = 0;
	}

	if (! OpenFTDlg(fv)) {
		return FALSE;
	}

	fv->IsPause = FALSE;
	fv->StartTime = GetTickCount();

	if (ts.DeferredLogWriteMode) {
		StartThread(fv);
	}

	if (fv->LogMode == TFileVar::LogModeTag::TEXT_MODE) {
		cv.Log1Byte = FLogPutANSI;
	}
	else {
		cv.Log1Bin = FLogPutBinary;
		cv.LogBinSkip = FLogBinSkip;
	}

	return TRUE;
}

/**
 * 現在バッファにあるデータをすべてログに書き出す
 *
 *	TODO
 *		1行の長さ
 */
void FLogOutputAllBuffer(void)
{
	if (LogVar == NULL) {
		return;
	}
	PFileVar fv = LogVar;
	DWORD ofs;
	int size;
	wchar_t buf[512];
	for (ofs = 0 ;  ; ofs++ ) {
		// 1つの行を取得する。文字だけなので、エスケープシーケンスは含まれない。
		size = BuffGetAnyLineDataW(ofs, buf, _countof(buf));
		if (size == -1)
			break;

		OutputStr(fv, buf);
		OutputStr(fv, L"\r\n");
		LogToFile(fv);
	}
}

/**
 * バッファへ1byte書き込み
 *		実際の書き込みは LogToFile() で行われる
 */
static void Put1(PFileVar fv, BYTE b)
{
	fv->cv_LogBuf[fv->cv_LogPtr] = b;
	fv->cv_LogPtr++;
	if (fv->cv_LogPtr >= fv->BuffSize) {
		fv->cv_LogPtr = fv->cv_LogPtr - fv->BuffSize;
	}
	if (fv->cv_LCount>=fv->BuffSize) {
		// バッファがいっぱいの時、古い1byteを捨てる
		fv->cv_LCount = fv->BuffSize;
		fv->cv_LStart = fv->cv_LogPtr;
	}
	else {
		fv->cv_LCount++;
	}
}

/**
 * バッファから1byte取り出し
 *
 *	@return	TRUE/FALSE	取り出せた/バッファが空
 */
static BOOL Get1(PFileVar fv, PBYTE b)
{
	if (fv->cv_LCount <= 0) {
		// 空
		return FALSE;
	}
	*b = fv->cv_LogBuf[fv->cv_LStart];
	fv->cv_LStart++;
	if (fv->cv_LStart >= fv->BuffSize) {
		fv->cv_LStart = fv->cv_LStart - fv->BuffSize;
	}
	fv->cv_LCount--;
	return TRUE;
}

static void logfile_lock(PFileVar fv)
{
	EnterCriticalSection(&fv->filelog_lock);
}

static void logfile_unlock(PFileVar fv)
{
	LeaveCriticalSection(&fv->filelog_lock);
}

// ログをローテートする。
// (2013.3.21 yutaka)
static void LogRotate(PFileVar fv)
{
	int loopmax = 10000;  // XXX
	int i, k;

	if (fv->RotateMode == ROTATE_NONE)
		return;

	if (fv->RotateMode == ROTATE_SIZE) {
		if (fv->ByteCount <= fv->RotateSize)
			return;
		//OutputDebugPrintf("%s: mode %d size %ld\n", __FUNCTION__, fv->RotateMode, fv->ByteCount);
	} else {
		return;
	}

	logfile_lock(fv);
	// ログサイズを再初期化する。
	fv->ByteCount = 0;

	// いったん今のファイルをクローズして、別名のファイルをオープンする。
	CloseFileSync(fv);

	// 世代ローテーションのステップ数の指定があるか
	if (fv->RotateStep > 0)
		loopmax = fv->RotateStep;

	for (i = 1 ; i <= loopmax ; i++) {
		wchar_t *filename;
		aswprintf(&filename, L"%s.%d", fv->FullName, i);
		DWORD attr = GetFileAttributesW(filename);
		free(filename);
		if (attr == INVALID_FILE_ATTRIBUTES)
			break;
	}
	if (i > loopmax) {
		// 世代がいっぱいになったら、最古のファイルから廃棄する。
		i = loopmax;
	}

	// 別ファイルにリネーム。
	for (k = i-1 ; k >= 0 ; k--) {
		wchar_t *oldfile;
		if (k == 0)
			oldfile = _wcsdup(fv->FullName);
		else
			aswprintf(&oldfile, L"%s.%d", fv->FullName, k);
		wchar_t *newfile;
		aswprintf(&newfile, L"%s.%d", fv->FullName, k+1);
		DeleteFileW(newfile);
		if (MoveFileW(oldfile, newfile) == 0) {
			OutputDebugPrintf("%s: rename %d\n", __FUNCTION__, errno);
		}
		free(oldfile);
		free(newfile);
	}

	// 再オープン
	OpenLogFile(fv);
	if (fv->bom) {
		FLogOutputBOM(fv);
	}
	if (ts.DeferredLogWriteMode) {
		StartThread(fv);
	}

	logfile_unlock(fv);
}

static wchar_t *TimeStampStr(PFileVar fv)
{
	char *strtime = NULL;
	switch (ts.LogTimestampType) {
	case TIMESTAMP_LOCAL:
	default:
		strtime = mctimelocal(ts.LogTimestampFormat, FALSE);
		break;
	case TIMESTAMP_UTC:
		strtime = mctimelocal(ts.LogTimestampFormat, TRUE);
		break;
	case TIMESTAMP_ELAPSED_LOGSTART:
		strtime = strelapsed(fv->StartTime);
		break;
	case TIMESTAMP_ELAPSED_CONNECTED:
		strtime = strelapsed(cv.ConnectedTime);
		break;
	}

	char tmp[128];
	tmp[0] = 0;
	strncat_s(tmp, sizeof(tmp), "[", _TRUNCATE);
	strncat_s(tmp, sizeof(tmp), strtime, _TRUNCATE);
	strncat_s(tmp, sizeof(tmp), "] ", _TRUNCATE);
	free(strtime);

	return ToWcharA(tmp);
}

/**
 * バッファ内のログをファイルへ書き込む
 */
static void LogToFile(PFileVar fv)
{
	BYTE b;

	if (fv->LogMode != TFileVar::LogModeTag::TEXT_MODE &&
		fv->LogMode != TFileVar::LogModeTag::BIN_MODE) {
		return;
	}

	if (fv->cv_LCount==0) {
		return;
	}

	// ロックを取る(2004.8.6 yutaka)
	logfile_lock(fv);

	// 書き込みデータを作成する
	DWORD WriteBufMax = fv->BuffSize;
	DWORD WriteBufLen = 0;
	PCHAR WriteBuf = (PCHAR)malloc(WriteBufMax);
	while (Get1(fv, &b)) {
		if (FLogIsPause() || ProtoGetProtoFlag()) {
			continue;
		}

		// fv->BuffSizeより大きくならないはず
		if (WriteBufLen >= (WriteBufMax*4/5)) {
			WriteBufMax *= 2;
			WriteBuf = (PCHAR)realloc(WriteBuf, WriteBufMax);
		}

		WriteBuf[WriteBufLen++] = b;

		fv->ByteCount++;
	}

	// 書き込み
	if (WriteBufLen > 0) {
		if (ts.DeferredLogWriteMode) {
			PostThreadMessage(fv->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)WriteBuf, WriteBufLen);
		}
		else {
			DWORD wrote;
			WriteFile(fv->FileHandle, WriteBuf, WriteBufLen, &wrote, NULL);
			free(WriteBuf);
		}
	}

	logfile_unlock(fv);

	if (FLogIsPause() || ProtoGetProtoFlag()) return;
	fv->FLogDlg->RefreshNum(fv->StartTime, fv->FileSize, fv->ByteCount);


	// ログ・ローテート
	LogRotate(fv);
}

static void FileTransEnd_(PFileVar fv)
{
	fv->LogMode = TFileVar::LogModeTag::NONE;
	cv.Log1Byte = NULL;
	cv.Log1Bin = NULL;
	cv.LogBinSkip = NULL;
	PFileTransDlg FLogDlg = fv->FLogDlg;
	if (FLogDlg != NULL) {
		FLogDlg->DestroyWindow();
		FLogDlg = NULL;
		fv->FLogDlg = NULL;
	}
	CloseFileSync(fv);
	free(fv->cv_LogBuf);
	fv->cv_LogBuf = NULL;
	fv->cv_LogPtr = 0;
	fv->cv_LStart = 0;
	fv->cv_LCount = 0;
	free(fv->FullName);
	fv->FullName = NULL;
	DeleteCriticalSection(&fv->filelog_lock);
	free(fv);
}

/**
 *	ログをポーズする
 */
void FLogPause(BOOL Pause)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	fv->IsPause = Pause;
	fv->FLogDlg->ChangeButton(Pause);
}

/**
 *	ログローテートの設定
 *	ログのサイズが<size>バイトを超えていれば、ローテーションするよう設定する
 */
void FLogRotateSize(size_t size)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	fv->RotateMode = ROTATE_SIZE;
	fv->RotateSize = (LONG)size;
}

/**
 *	ログローテートの設定
 *	ログファイルの世代を設定する
 */
void FLogRotateRotate(int step)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	fv->RotateStep = step;
}

/**
 *	ログローテートの設定
 *	ローテーションを停止
 */
void FLogRotateHalt(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	fv->RotateMode = ROTATE_NONE;
	fv->RotateSize = 0;
	fv->RotateStep = 0;
}

void FLogClose(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}

	FileTransEnd_(fv);
	LogVar = NULL;
}

/**
 *	ログをオープンする
 *	@param[in]	fname	ログファイル名, CreateFile()に渡される
 *
 *	ログファイル名はstrftimeの展開などは行われない。
 *	FLogGetLogFilename() や FLogOpenDialog() で
 *	ファイル名を取得できる。
 */
BOOL FLogOpen(const wchar_t *fname, LogCode_t code, BOOL bom)
{
	if (LogVar != NULL) {
		return FALSE;
	}

	//
	PFileVar fv = (PFileVar)malloc(sizeof(*fv));
	if (fv == NULL) {
		return FALSE;
	}
	memset(fv, 0, sizeof(TFileVar));
	fv->FileHandle = INVALID_HANDLE_VALUE;
	fv->LogThread = INVALID_HANDLE_VALUE;
	fv->eLineEnd = Line_LineHead;

	fv->log_code = code;
	fv->bom = bom;

	InitializeCriticalSection(&fv->filelog_lock);

	BOOL ret = LogStart(fv, fname);
	if (ret == FALSE) {
		FileTransEnd_(fv);
		LogVar = NULL;
		return FALSE;
	}
	LogVar = fv;
	return TRUE;
}

BOOL FLogIsOpend(void)
{
	return LogVar != NULL;
}

BOOL FLogIsOpendText(void)
{
	PFileVar fv = LogVar;
	return fv != NULL && (fv->LogMode == TFileVar::LogModeTag::TEXT_MODE);
}

BOOL FLogIsOpendBin(void)
{
	PFileVar fv = LogVar;
	return fv != NULL && (fv->LogMode == TFileVar::LogModeTag::BIN_MODE);
}

/**
 *	ログに文字列を書き込む
 */
void FLogWriteStr(const wchar_t *str)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	if (str == NULL) {
		return;
	}
	OutputStr(fv, str);

	// すぐに書き込む
	LogToFile(fv);
}

/**
 *	ログの情報を取得する
 *	マクロ用
 */
void FLogInfo(char *param_ptr, size_t param_len)
{
	PFileVar fv = LogVar;
	if (fv) {
		param_ptr[0] = '0'
			+ (ts.LogBinary != 0)
			+ ((ts.Append != 0) << 1)
			+ ((ts.LogTypePlainText != 0) << 2)
			+ ((ts.LogTimestamp != 0) << 3)
			+ ((ts.LogHideDialog != 0) << 4);
		char *filenameU8 = ToU8W(fv->FullName);
		strncpy_s(param_ptr + 1, param_len - 1, filenameU8, _TRUNCATE);
		free(filenameU8);
	}
	else {
		param_ptr[0] = '0' - 1;
		param_ptr[1] = 0;
	}
}

/**
 *	現在のログファイル名を取得
 */
const wchar_t *FLogGetFilename(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return NULL;
	}
	return fv->FullName;
}

/**
 *	ログファイル名用の修飾を行う,ファイル名部分のみ
 *	- strftime() と同じ日付展開
 *	- 設定されたログファイルフォルダを追加
 *	- ホスト名,ポート番号展開
 *
 *	@param 	filename	ファイル名(パスは含まない)
 *	@return	修飾済みファイル名
 */
wchar_t *FLogGetLogFilenameBase(const wchar_t *filename)
{
	// ファイル名部分を抽出
	const wchar_t *last_path_sep = wcsrchr(filename, L'\\');
	wchar_t *format;
	if (last_path_sep == NULL) {
		format = _wcsdup(filename);
	}
	else {
		format = _wcsdup(last_path_sep + 1);
	}

	// strftime に使用できない文字を削除
	deleteInvalidStrftimeCharW(format);

	// 文字列長が0になった?
	if (format[0] == 0) {
		free(format);
		return _wcsdup(L"");
	}

	// 現在時刻を取得
	time_t time_local;
	time(&time_local);
	struct tm tm_local;
	localtime_s(&tm_local, &time_local);

	// strftime()で変換
	// 文字領域は自動拡張
	size_t len = 32;
	wchar_t *formated = NULL;
	while (1) {
		wchar_t *formated_realloc = (wchar_t *)realloc(formated, sizeof(wchar_t) * len);
		if (formated_realloc == NULL) {
			free(format);
			free(formated);
			return _wcsdup(L"");
		}
		formated = formated_realloc;
		size_t r = wcsftime(formated, len, format, &tm_local);
		if (r != 0) {
			// フォーマットできた
			break;
		}
		len *= 2;
	}
	free(format);

	// ホスト名など
	wchar_t *host = ConvertLognameW(&cv, formated);
	free(formated);

	// ファイル名に使用できない文字を置換
	//wchar_t *replaced = replaceInvalidFileNameCharW(host, 0);	// 削除
	wchar_t *replaced = replaceInvalidFileNameCharW(host, L'_');
	free(host);

	return replaced;
}

/**
 *	ログファイル名を取得
 *	ログファイル名用の修飾を行う
 *	- strftime() と同じ日付展開
 *	- 設定されたログファイルフォルダを追加
 *	- ホスト名,ポート番号展開
 *
 *	@param[in]	log_filename	ファイル名(相対/絶対どちらでもok)
 *								NULLの場合デフォルトファイル名となる
 *								strftime形式ok
 *	@return						フルパスファイル名
 *								不要になったら free() すること
 */
wchar_t *FLogGetLogFilename(const wchar_t *log_filename)
{
	wchar_t *dir;
	wchar_t *fname;
	if (log_filename == NULL) {
		dir = GetTermLogDir(&ts);
		fname = _wcsdup(ts.LogDefaultNameW);
	}
	else if (!IsRelativePathW(log_filename)) {
		// 絶対パスが入力された
		dir = ExtractDirNameW(log_filename);
		if (dir == NULL) {
			return NULL;
		}
		fname = ExtractFileNameW(log_filename);
	}
	else {
		dir = GetTermLogDir(&ts);
		fname = _wcsdup(log_filename);
	}

	wchar_t *formated = FLogGetLogFilenameBase(fname);
	free(fname);

	// 連結,正規化
	wchar_t *logfull = GetFullPathW(dir, formated);
	free(formated);
	free(dir);

	return logfull;
}

BOOL FLogIsPause()
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return FALSE;
	}
	return fv->IsPause;
}

void FLogWindow(int nCmdShow)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}

	HWND HWndLog = fv->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, nCmdShow);
	if (nCmdShow == SW_RESTORE) {
		// 拡張スタイル WS_EX_NOACTIVATE 状態を解除する
		SetForegroundWindow(HWndLog);
	}
}

void FLogShowDlg(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	HWND HWndLog = fv->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, SW_SHOWNORMAL);
	SetForegroundWindow(HWndLog);
}

/**
 * ログへ1byte書き込み
 */
static void Log1Bin(PFileVar fv, BYTE b)
{
	if (fv->IsPause || ProtoGetProtoFlag()) {
		return;
	}
	if (fv->LogMode == TFileVar::LogModeTag::BIN_MODE) {
		if (fv->cv_BinSkip > 0) {
			fv->cv_BinSkip--;
			return;
		}
	}
	Put1(fv, b);
}

static void LogBinSkip(PFileVar fv, int add)
{
	if (fv->LogMode == TFileVar::LogModeTag::BIN_MODE) {
		fv->cv_BinSkip += add;
	}
}

/**
 *	ログバッファに溜まっているデータのバイト数を返す
 */
int FLogGetCount(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return 0;
	}
	return fv->cv_LCount;
}

/**
 *	ログバッファの空きバイト数を返す
 */
int FLogGetFreeCount(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return 0;
	}
	return fv->BuffSize - fv->cv_LCount;
}

/**
 * バッファ内のログをファイルへ書き込む
 */
void FLogWriteFile(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	LogToFile(fv);
}

/**
 *	ログに1文字書きこむ(text, unicode)
 */
static void FLogPutUTF32_(PFileVar fv, unsigned int u32)
{
	if (fv->LogMode != TFileVar::LogModeTag::TEXT_MODE) {
		// ログには出力しない
		return;
	}

	// 行頭か?(改行を出力した直後)
	if (ts.LogTimestamp && fv->eLineEnd) {
		// タイムスタンプを出力
		fv->eLineEnd = Line_Other; /* clear endmark*/
		wchar_t* strtime = TimeStampStr(fv);
		FLogWriteStr(strtime);
		free(strtime);
	}

	switch(fv->log_code) {
	case LOG_UTF8: {
		// UTF-8
		char u8_buf[4];
		size_t u8_len = UTF32ToUTF8(u32, u8_buf, _countof(u8_buf));
		for (size_t i = 0; i < u8_len; i++) {
			BYTE b = u8_buf[i];
			Put1(fv, b);
		}
		break;
	}
	case LOG_UTF16LE:
	case LOG_UTF16BE: {
		// UTF-16
		wchar_t u16[2];
		size_t u16_len = UTF32ToUTF16(u32, u16, _countof(u16));
		for (size_t i = 0; i < u16_len; i++) {
			if (fv->log_code == LOG_UTF16LE) {
				// UTF-16LE
				Put1(fv, u16[i] & 0xff);
				Put1(fv, (u16[i] >> 8) & 0xff);
			}
			else {
				// UTF-16BE
				Put1(fv, (u16[i] >> 8) & 0xff);
				Put1(fv, u16[i] & 0xff);
			}
		}
	}
	}

	if (u32 == 0x0a) {
		fv->eLineEnd = Line_LineHead; /* set endmark*/
	}
}

static void FLogOutputBOM(PFileVar fv)
{
	DWORD wrote;

	switch(fv->log_code) {
	case 0: {
		// UTF-8
		const char *bom = "\xef\xbb\xbf";
		WriteFile(fv->FileHandle, bom, 3, &wrote, NULL);
		fv->ByteCount += 3;
		break;
	}
	case 1: {
		// UTF-16LE
		const char *bom = "\xff\xfe";
		WriteFile(fv->FileHandle, bom, 2, &wrote, NULL);
		fv->ByteCount += 2;
		break;
	}
	case 2: {
		// UTF-16BE
		const char *bom = "\xfe\xff";
		WriteFile(fv->FileHandle, bom, 2, &wrote, NULL);
		fv->ByteCount += 2;
		break;
	}
	default:
		break;
	}
}

static void OutputStr(PFileVar fv, const wchar_t *strW)
{
	size_t len;

	assert(strW != NULL);

	if (fv->LogMode == TFileVar::LogModeTag::TEXT_MODE) {
		len = wcslen(strW);
		while(*strW != 0) {
			unsigned int u32;
			size_t u16_len = UTF16ToUTF32(strW, len, &u32);
			switch (u16_len) {
			case 0:
			default:
				// 変換できない
				strW++;
				len--;
				break;
			case 1:
			case 2: {
				FLogPutUTF32_(fv, u32);
				strW += u16_len;
				len -= u16_len;
				break;
			}
			}
		}
	} else if (fv->LogMode == TFileVar::LogModeTag::BIN_MODE) {
		char *str = MakeOutputStringConvW(strW, ts.KanjiCode, ts.KanjiIn, ts.KanjiOut, ts.JIS7KatakanaSend, NULL);
		if (str != NULL) {
			for (size_t i = 0; str[i] != 0; i++) {
				Put1(fv, (BYTE)str[i]);
			}
			free(str);
		}
	}
}

/**
 *	ログに1文字書きこむ(text, unicode)
 */
void FLogPutUTF32(unsigned int u32)
{
	PFileVar fv = LogVar;
	FLogPutUTF32_(fv, u32);
}

/**
 *	1文字(ANSI)出力
 *
 */
void FLogPutANSI(BYTE b)
{
	PFileVar fv = LogVar;
	if (fv->LogMode != TFileVar::LogModeTag::TEXT_MODE) {
		return;
	}
	Put1(fv, b);
}

/**
 *	1文字(Binary)出力
 *
 */
void FLogPutBinary(BYTE b)
{
	PFileVar fv = LogVar;
	Log1Bin(fv, b);
}

/**
 *	指定文字スキップ
 *		Binary出力時
 */
void FLogBinSkip(int add)
{
	PFileVar fv = LogVar;
	LogBinSkip(fv, add);
}
