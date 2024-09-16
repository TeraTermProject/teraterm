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

#include "teraterm.h"
#include "tttypes.h"
#include "ftdlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "ttlib_types.h"
#include "dlglib.h"
#include "vtterm.h"
#include "ftlib.h"
#include "codeconv.h"
#include "asprintf.h"
#include "win32helper.h"

#include "filesys_log_res.h"
#include "filesys_log.h"
#include "filesys.h"  // for ProtoGetProtoFlag()

#define TitLog      L"Log"

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
	LONG FileSize, ByteCount;

	DWORD StartTime;

	enum enumLineEnd eLineEnd;

	// log rotate
	int RotateMode;  //  enum rotate_mode RotateMode;
	LONG RotateSize;
	int RotateStep;

	HANDLE LogThread;
	DWORD LogThreadId;
	HANDLE LogThreadEvent;

	BOOL IsPause;

	PFileTransDlg FLogDlg;

	LogCode_t log_code;
	BOOL bom;

	BOOL FileLog;
	BOOL BinLog;
} TFileVar;
typedef TFileVar *PFileVar;

static PFileVar LogVar = NULL;

static PCHAR cv_LogBuf;
static int cv_LogPtr, cv_LStart, cv_LCount;
static PCHAR cv_BinBuf;
static int cv_BinPtr, cv_BStart, cv_BCount;
static int cv_BinSkip;

// �x���������ݗp�X���b�h�̃��b�Z�[�W
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void Log1Bin(BYTE b);
static void LogBinSkip(int add);
static BOOL CreateLogBuf(void);
static BOOL CreateBinBuf(void);
void LogPut1(BYTE b);
static void OutputStr(const wchar_t *str);
static void LogToFile(PFileVar fv);
static void FLogOutputBOM(PFileVar fv);

static BOOL OpenFTDlg_(PFileVar fv)
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
 *	�t�@�C����������̒u������,���O�p
 *	���̕�����u��������
 *		&h	�z�X�g���ɒu��
 *		&p	TCP�|�[�g�ԍ��ɒu��
 *		&u	���O�I�����̃��[�U��
 *
 *	@param	pcv
 *	@param	src	�u��������O�̕�����(�t�@�C����)
 *	@return	�u��������ꂽ������
 *			�s�v�ɂȂ�����free()���邱��
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
						// �z�X�g����IPv6�A�h���X���ƁA�t�@�C�����Ɏg�p�ł��Ȃ�����(:)�����邽�ߒu��
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


// �X���b�h�̏I���ƃt�@�C���̃N���[�Y
static void CloseFileSync(PFileVar fv)
{
	BOOL ret;

	if (fv->FileHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	if (fv->LogThread != INVALID_HANDLE_VALUE) {
		// �X���b�h�̏I���҂�
		ret = PostThreadMessage(fv->LogThreadId, WM_QUIT, 0, 0);
		if (ret != 0) {
			// �X���b�h�L���[�ɃG���L���[�ł����ꍇ�̂ݑ҂����킹���s���B
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

// �x���������ݗp�X���b�h
static unsigned _stdcall DeferredLogWriteThread(void *arg)
{
	MSG msg;
	PFileVar fv = (PFileVar)arg;
	PCHAR buf;
	DWORD buflen;
	DWORD wrote;

	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// �X���b�h�L���[�̍쐬���I��������Ƃ��X���b�h�������֒ʒm����B
	if (fv->LogThreadEvent != NULL) {
		SetEvent(fv->LogThreadEvent);
	}

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {
			case WM_DPC_LOGTHREAD_SEND:
				buf = (PCHAR)msg.wParam;
				buflen = (DWORD)msg.lParam;
				WriteFile(fv->FileHandle, buf, buflen, &wrote, NULL);
				free(buf);   // �����Ń��������
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

// �x���������ݗp�X���b�h���N�����B
// (2013.4.19 yutaka)
// DeferredLogWriteThread �X���b�h���N�����āA�X���b�h�L���[���쐬�������O�ɁA
// ���O�t�@�C���̃N���[�Y(CloseFileSync)���s����ƁA�G���L���[�����s���A�f�b�h���b�N
// ����Ƃ��������C�������B
// �X���b�h�Ԃ̓������s�����߁A���O�Ȃ��C�x���g�I�u�W�F�N�g���g���āA�X���b�h�L���[��
// �쐬�܂ő҂����킹����悤�ɂ����B���O�t���C�x���g�I�u�W�F�N�g���g���ꍇ�́A
// �V�X�e��(Windows OS)��Ń��j�[�N�Ȗ��O�ɂ���K�v������B
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
	// LogLockExclusive ���L���ȏꍇ�ɂ܂��������L���Ȃ��ƁA
	// �������ݒ��̃��O�t�@�C���𑼂̃G�f�B�^�ŊJ���Ȃ�����
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

	if (ts.LogBinary > 0)
	{
		fv->BinLog = TRUE;
		fv->FileLog = FALSE;
		if (! CreateBinBuf())
		{
			return FALSE;
		}
	}
	else {
		fv->BinLog = FALSE;
		fv->FileLog = TRUE;
		if (! CreateLogBuf())
		{
			return FALSE;
		}
	}
	cv_LStart = cv_LogPtr;
	cv_LCount = 0;

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

	// BOM�o��
	if (ts.Append == 0 && ts.LogBinary == 0 && fv->bom) {
		// �ǋL�ł͂Ȃ�(�V�K) && �o�C�i���ł͂Ȃ� && BOM ���o�͎�
		FLogOutputBOM(fv);
	}

	// Log rotate configuration
	fv->RotateMode = ts.LogRotate;
	fv->RotateSize = ts.LogRotateSize;
	fv->RotateStep = ts.LogRotateStep;

	// Log rotate���L���̏ꍇ�A�����t�@�C���T�C�Y��ݒ肷��B
	// �ŏ��̃t�@�C�����ݒ肵���T�C�Y�Ń��[�e�[�g���Ȃ����̏C���B
	// (2016.4.9 yutaka)
	if (fv->RotateMode != ROTATE_NONE) {
		DWORD size = GetFileSize(fv->FileHandle, NULL);
		if (size == -1) {
			return FALSE;
		}
		fv->ByteCount = size;
	}
	else {
		fv->ByteCount = 0;
	}

	if (! OpenFTDlg_(fv)) {
		return FALSE;
	}

	fv->IsPause = FALSE;
	fv->StartTime = GetTickCount();

	if (ts.DeferredLogWriteMode) {
		StartThread(fv);
	}

	if (fv->FileLog) {
		cv.Log1Byte = LogPut1;
	}
	if (fv->BinLog) {
		cv.Log1Bin = Log1Bin;
		cv.LogBinSkip = LogBinSkip;
	}

	return TRUE;
}

/**
 * ���݃o�b�t�@�ɂ���f�[�^�����ׂă��O�ɏ����o��
 * (2013.9.29 yutaka)
 *
 *	TODO
 *		1�s�̒���
 */
void FLogOutputAllBuffer(void)
{
	PFileVar fv = LogVar;
	DWORD ofs;
	int size;
	wchar_t buf[512];
	for (ofs = 0 ;  ; ofs++ ) {
		// 1�̍s���擾����B���������Ȃ̂ŁA�G�X�P�[�v�V�[�P���X�͊܂܂�Ȃ��B
		size = BuffGetAnyLineDataW(ofs, buf, _countof(buf));
		if (size == -1)
			break;

		OutputStr(buf);
		OutputStr(L"\r\n");
		LogToFile(fv);
	}
}

/**
 * ���O��1byte��������
 *		�o�b�t�@�֏������܂��
 *		���ۂ̏������݂� LogToFile() �ōs����
 */
void LogPut1(BYTE b)
{
	PFileVar fv = LogVar;

	cv_LogBuf[cv_LogPtr] = b;
	cv_LogPtr++;
	if (cv_LogPtr>=InBuffSize)
		cv_LogPtr = cv_LogPtr-InBuffSize;

	if (fv->FileLog)
	{
		if (cv_LCount>=InBuffSize)
		{
			cv_LCount = InBuffSize;
			cv_LStart = cv_LogPtr;
		}
		else
			cv_LCount++;
	}
	else
		cv_LCount = 0;
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



static CRITICAL_SECTION g_filelog_lock;   /* ���b�N�p�ϐ� */

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

// ���O�����[�e�[�g����B
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

	logfile_lock();
	// ���O�T�C�Y���ď���������B
	fv->ByteCount = 0;

	// �������񍡂̃t�@�C�����N���[�Y���āA�ʖ��̃t�@�C�����I�[�v������B
	CloseFileSync(fv);

	// ���ネ�[�e�[�V�����̃X�e�b�v���̎w�肪���邩
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
		// ���オ�����ς��ɂȂ�����A�ŌẪt�@�C������p������B
		i = loopmax;
	}

	// �ʃt�@�C���Ƀ��l�[���B
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

	// �ăI�[�v��
	OpenLogFile(fv);
	if (fv->bom) {
		FLogOutputBOM(fv);
	}
	if (ts.DeferredLogWriteMode) {
		StartThread(fv);
	}

	logfile_unlock();
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
 * �o�b�t�@���̃��O���t�@�C���֏�������
 */
static void LogToFile(PFileVar fv)
{
	PCHAR Buf;
	int Start, Count;
	BYTE b;

	if (fv->FileLog)
	{
		Buf = cv_LogBuf;
		Start = cv_LStart;
		Count = cv_LCount;
	}
	else if (fv->BinLog)
	{
		Buf = cv_BinBuf;
		Start = cv_BStart;
		Count = cv_BCount;
	}
	else
		return;

	if (Buf==NULL) return;
	if (Count==0) return;

	// ���b�N�����(2004.8.6 yutaka)
	logfile_lock();

	// �������݃f�[�^���쐬����
	DWORD WriteBufMax = 8192;
	DWORD WriteBufLen = 0;
	PCHAR WriteBuf = (PCHAR)malloc(WriteBufMax);
	while (Get1(Buf,&Start,&Count,&b)) {
		if (FLogIsPause() || ProtoGetProtoFlag()) {
			continue;
		}

		if (WriteBufLen >= (WriteBufMax*4/5)) {
			WriteBufMax *= 2;
			WriteBuf = (PCHAR)realloc(WriteBuf, WriteBufMax);
		}

		WriteBuf[WriteBufLen++] = b;

		(fv->ByteCount)++;
	}

	// ��������
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

	logfile_unlock();

	if (fv->FileLog)
	{
		cv_LStart = Start;
		cv_LCount = Count;
	}
	else {
		cv_BStart = Start;
		cv_BCount = Count;
	}
	if (FLogIsPause() || ProtoGetProtoFlag()) return;
	fv->FLogDlg->RefreshNum(fv->StartTime, fv->FileSize, fv->ByteCount);


	// ���O�E���[�e�[�g
	LogRotate(fv);
}

static BOOL CreateLogBuf(void)
{
	if (cv_LogBuf==NULL)
	{
		cv_LogBuf = (char *)malloc(InBuffSize);
		cv_LogPtr = 0;
		cv_LStart = 0;
		cv_LCount = 0;
	}
	return (cv_LogBuf!=NULL);
}

static void FreeLogBuf(void)
{
	free(cv_LogBuf);
	cv_LogBuf = NULL;
	cv_LogPtr = 0;
	cv_LStart = 0;
	cv_LCount = 0;
}

static BOOL CreateBinBuf(void)
{
	if (cv_BinBuf==NULL)
	{
		cv_BinBuf = (PCHAR)malloc(InBuffSize);
		cv_BinPtr = 0;
		cv_BStart = 0;
		cv_BCount = 0;
	}
	return (cv_BinBuf!=NULL);
}

static void FreeBinBuf(void)
{
	free(cv_BinBuf);
	cv_BinBuf = NULL;
	cv_BinPtr = 0;
	cv_BStart = 0;
	cv_BCount = 0;
}

static void FileTransEnd_(PFileVar fv)
{
	fv->FileLog = FALSE;
	fv->BinLog = FALSE;
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
	FreeLogBuf();
	FreeBinBuf();
	free(fv->FullName);
	fv->FullName = NULL;
	free(fv);

	LogVar = NULL;
}

/**
 *	���O���|�[�Y����
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
 *	���O���[�e�[�g�̐ݒ�
 *	���O�̃T�C�Y��<size>�o�C�g�𒴂��Ă���΁A���[�e�[�V��������悤�ݒ肷��
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
 *	���O���[�e�[�g�̐ݒ�
 *	���O�t�@�C���̐����ݒ肷��
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
 *	���O���[�e�[�g�̐ݒ�
 *	���[�e�[�V�������~
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

static INT_PTR CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_COMMENT_TITLE" },
		{ IDOK, "BTN_OK" }
	};

	switch (msg) {
		case WM_INITDIALOG:
			// �G�f�B�b�g�R���g���[���Ƀt�H�[�J�X�����Ă�
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));
			SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK: {
					size_t len = SendDlgItemMessageW(hDlgWnd, IDC_EDIT_COMMENT, WM_GETTEXTLENGTH, 0, 0);
					len += 1;
					wchar_t *buf = (wchar_t *)malloc(len * sizeof(wchar_t));
					GetDlgItemTextW(hDlgWnd, IDC_EDIT_COMMENT, buf, (int)len);
					FLogWriteStr(buf);
					FLogWriteStr(L"\n");		// TODO ���s�R�[�h
					free(buf);
					TTEndDialog(hDlgWnd, IDOK);
					break;
				}
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			TTEndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

/**
 * ���O�t�@�C���փR�����g��ǉ����� (2004.8.6 yutaka)
 */
void FLogAddCommentDlg(HINSTANCE hInst, HWND hWnd)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	TTDialogBox(hInst, MAKEINTRESOURCEW(IDD_COMMENT_DIALOG),
				hWnd, OnCommentDlgProc);
}

void FLogClose(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}

	FileTransEnd_(fv);
}

/**
 *	���O���I�[�v������
 *	@param[in]	fname	���O�t�@�C����, CreateFile()�ɓn�����
 *
 *	���O�t�@�C������strftime�̓W�J�Ȃǂ͍s���Ȃ��B
 *	FLogGetLogFilename() �� FLogOpenDialog() ��
 *	�t�@�C�������擾�ł���B
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
	LogVar = fv;
	memset(fv, 0, sizeof(TFileVar));
	fv->FileHandle = INVALID_HANDLE_VALUE;
	fv->LogThread = INVALID_HANDLE_VALUE;
	fv->eLineEnd = Line_LineHead;

	fv->log_code = code;
	fv->bom = bom;
	BOOL ret = LogStart(fv, fname);
	if (ret == FALSE) {
		FileTransEnd_(fv);
	}

	return ret;
}

BOOL FLogIsOpend(void)
{
	return LogVar != NULL;
}

BOOL FLogIsOpendText(void)
{
	return LogVar != NULL && LogVar->FileLog;
}

BOOL FLogIsOpendBin(void)
{
	return LogVar != NULL && LogVar->BinLog;
}

void FLogWriteStr(const wchar_t *str)
{
	if (LogVar != NULL) {
		OutputStr(str);
	}
}

/**
 *	���O�̏����擾����
 *	�}�N���p
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
 *	���݂̃��O�t�@�C�������擾
 */
const wchar_t *FLogGetFilename(void)
{
	if (LogVar == NULL) {
		return NULL;
	}
	return LogVar->FullName;
}

/**
 *	���O�t�@�C�����p�̏C�����s��,�t�@�C���������̂�
 *	- strftime() �Ɠ������t�W�J
 *	- �ݒ肳�ꂽ���O�t�@�C���t�H���_��ǉ�
 *	- �z�X�g��,�|�[�g�ԍ��W�J
 *
 *	@param 	filename	�t�@�C����(�p�X�͊܂܂Ȃ�)
 *	@return	�C���ς݃t�@�C����
 */
wchar_t *FLogGetLogFilenameBase(const wchar_t *filename)
{
	// �t�@�C���������𒊏o
	const wchar_t *last_path_sep = wcsrchr(filename, L'\\');
	wchar_t *format;
	if (last_path_sep == NULL) {
		format = _wcsdup(filename);
	}
	else {
		format = _wcsdup(last_path_sep + 1);
	}

	// strftime �Ɏg�p�ł��Ȃ��������폜
	deleteInvalidStrftimeCharW(format);

	// �����񒷂�0�ɂȂ���?
	if (format[0] == 0) {
		free(format);
		return _wcsdup(L"");
	}

	// ���ݎ������擾
	time_t time_local;
	time(&time_local);
	struct tm tm_local;
	localtime_s(&tm_local, &time_local);

	// strftime()�ŕϊ�
	// �����̈�͎����g��
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
			// �t�H�[�}�b�g�ł���
			break;
		}
		len *= 2;
	}
	free(format);

	// �z�X�g���Ȃ�
	wchar_t *host = ConvertLognameW(&cv, formated);
	free(formated);

	// �t�@�C�����Ɏg�p�ł��Ȃ�������u��
	//wchar_t *replaced = replaceInvalidFileNameCharW(host, 0);	// �폜
	wchar_t *replaced = replaceInvalidFileNameCharW(host, L'_');
	free(host);

	return replaced;
}

/**
 *	���O�t�@�C�������擾
 *	���O�t�@�C�����p�̏C�����s��
 *	- strftime() �Ɠ������t�W�J
 *	- �ݒ肳�ꂽ���O�t�@�C���t�H���_��ǉ�
 *	- �z�X�g��,�|�[�g�ԍ��W�J
 *
 *	@param[in]	log_filename	�t�@�C����(����/��΂ǂ���ł�ok)
 *								NULL�̏ꍇ�f�t�H���g�t�@�C�����ƂȂ�
 *								strftime�`��ok
 *	@return						�t���p�X�t�@�C����
 *								�s�v�ɂȂ����� free() ���邱��
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
		// ��΃p�X�����͂��ꂽ
		dir = ExtractDirNameW(log_filename);
		fname = ExtractFileNameW(log_filename);
	}
	else {
		dir = GetTermLogDir(&ts);
		fname = _wcsdup(log_filename);
	}

	wchar_t *formated = FLogGetLogFilenameBase(fname);
	free(fname);

	// �A��,���K��
	wchar_t *logfull = GetFullPathW(dir, formated);
	free(formated);
	free(dir);

	return logfull;
}

BOOL FLogIsPause()
{
	if (LogVar == NULL) {
		return FALSE;
	}
	return LogVar->IsPause;
}

void FLogWindow(int nCmdShow)
{
	if (LogVar == NULL) {
		return;
	}

	HWND HWndLog = LogVar->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, nCmdShow);
	if (nCmdShow == SW_RESTORE) {
		// �g���X�^�C�� WS_EX_NOACTIVATE ��Ԃ���������
		SetForegroundWindow(HWndLog);
	}
}

void FLogShowDlg(void)
{
	if (LogVar == NULL) {
		return;
	}
	HWND HWndLog = LogVar->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, SW_SHOWNORMAL);
	SetForegroundWindow(HWndLog);
}

/**
 * ���O��1byte��������
 *		LogPut1() �ƈႤ?
 */
//void Log1Bin(PComVar cv, BYTE b)
static void Log1Bin(BYTE b)
{
	if (LogVar->IsPause || ProtoGetProtoFlag()) {
		return;
	}
	if (cv_BinSkip > 0) {
		cv_BinSkip--;
		return;
	}
	cv_BinBuf[cv_BinPtr] = b;
	cv_BinPtr++;
	if (cv_BinPtr>=InBuffSize) {
		cv_BinPtr = cv_BinPtr-InBuffSize;
	}
	if (cv_BCount>=InBuffSize) {
		cv_BCount = InBuffSize;
		cv_BStart = cv_BinPtr;
	}
	else {
		cv_BCount++;
	}
}

static void LogBinSkip(int add)
{
	if (cv_BinBuf != NULL) {
		cv_BinSkip += add;
	}
}

/**
 *	���O�o�b�t�@�ɗ��܂��Ă���f�[�^�̃o�C�g����Ԃ�
 */
int FLogGetCount(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return 0;
	}
	if (fv->FileLog) {
		return cv_LCount;
	}
	if (fv->BinLog) {
		return cv_BCount;
	}
	return 0;
}

/**
 *	���O�o�b�t�@�̋󂫃o�C�g����Ԃ�
 */
int FLogGetFreeCount(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return 0;
	}
	if (fv->FileLog) {
		return InBuffSize - cv_LCount;
	}
	if (fv->BinLog) {
		return InBuffSize - cv_BCount;
	}
	return 0;
}

/**
 * �o�b�t�@���̃��O���t�@�C���֏�������
 */
void FLogWriteFile(void)
{
	PFileVar fv = LogVar;
	if (fv == NULL) {
		return;
	}
	if (cv_LogBuf!=NULL)
	{
		if (fv->FileLog) {
			LogToFile(fv);
		}
	}

	if (cv_BinBuf!=NULL)
	{
		if (fv->BinLog) {
			LogToFile(fv);
		}
	}
}

void FLogPutUTF32(unsigned int u32)
{
	PFileVar fv = LogVar;
	BOOL log_available = (cv_LogBuf != 0);

	if (!log_available) {
		// ���O�ɂ͏o�͂��Ȃ�
		return;
	}

	// �s����?(���s���o�͂�������)
	if (ts.LogTimestamp && fv->eLineEnd) {
		// �^�C���X�^���v���o��
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
			LogPut1(b);
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
				LogPut1(u16[i] & 0xff);
				LogPut1((u16[i] >> 8) & 0xff);
			}
			else {
				// UTF-16BE
				LogPut1((u16[i] >> 8) & 0xff);
				LogPut1(u16[i] & 0xff);
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

static void OutputStr(const wchar_t *str)
{
	size_t len;

	assert(str != NULL);

	len = wcslen(str);
	while(*str != 0) {
		unsigned int u32;
		size_t u16_len = UTF16ToUTF32(str, len, &u32);
		switch (u16_len) {
		case 0:
		default:
			// �ϊ��ł��Ȃ�
			str++;
			len--;
			break;
		case 1:
		case 2: {
			FLogPutUTF32(u32);
			str += u16_len;
			len -= u16_len;
			break;
		}
		}
	}
}
