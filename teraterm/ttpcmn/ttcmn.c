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

/* TTCMN.DLL, main */
#include <string.h>
#include <windows.h>
#include <assert.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "codeconv.h"
#include "compat_win.h"
#include "win32helper.h"
#include "makeoutputstring.h"

#include "ttcmn_shared_memory.h"
#include "ttcommon.h"
#include "ttcmn_i.h"

static PMap pm;

#define VTCLASSNAME "VTWin32"
#define TEKCLASSNAME "TEKWin32"

enum window_style {
	WIN_CASCADE,
	WIN_STACKED,
	WIN_SIDEBYSIDE,
};

void WINAPI SetCOMFlag(int Com)
{
	if (Com <= 0 || MAXCOMPORT <= Com) return;
	pm->ComFlag[(Com-1)/CHAR_BIT] |= 1 << ((Com-1)%CHAR_BIT);
}

void WINAPI ClearCOMFlag(int Com)
{
	if (Com <= 0 || MAXCOMPORT <= Com) return;
	pm->ComFlag[(Com-1)/CHAR_BIT] &= ~(1 << ((Com-1)%CHAR_BIT));
}

int WINAPI CheckCOMFlag(int Com)
{
	if (Com <= 0) return 0;
	if (Com > MAXCOMPORT) return 1;
	return ((pm->ComFlag[(Com-1)/CHAR_BIT] & 1 << (Com-1)%CHAR_BIT) > 0);
}

int WINAPI RegWin(HWND HWinVT, HWND HWinTEK)
{
	int i, j;

	if (pm->NWin>=MAXNWIN)
		return 0;
	if (HWinVT==NULL)
		return 0;
	if (HWinTEK!=NULL) {
		i = 0;
		while ((i<pm->NWin) && (pm->WinList[i]!=HWinVT))
			i++;
		if (i>=pm->NWin)
			return 0;
		for (j=pm->NWin-1 ; j>i ; j--)
			pm->WinList[j+1] = pm->WinList[j];
		pm->WinList[i+1] = HWinTEK;
		pm->NWin++;
		return 0;
	}
	pm->WinList[pm->NWin++] = HWinVT;
	memset(&pm->WinPrevRect[pm->NWin - 1], 0, sizeof(pm->WinPrevRect[pm->NWin - 1])); // RECT clear
	if (pm->NWin==1) {
		return 1;
	}
	else {
		return (int)(SendMessage(pm->WinList[pm->NWin-2],
		                         WM_USER_GETSERIALNO,0,0)+1);
	}
}

void WINAPI UnregWin(HWND HWin)
{
	int i, j;

	i = 0;
	while ((i<pm->NWin) && (pm->WinList[i]!=HWin)) {
		i++;
	}
	if (pm->WinList[i]!=HWin) {
		return;
	}
	for (j=i ; j<pm->NWin-1 ; j++) {
		pm->WinList[j] = pm->WinList[j+1];
		pm->WinPrevRect[j] = pm->WinPrevRect[j+1];  // RECT shift
	}
	if (pm->NWin>0) {
		pm->NWin--;
	}
}

static char GetWindowTypeChar(HWND Hw, HWND HWin)
{
#if 0
	if (HWin == Hw)
		return '*';
	else if (!IsWindowVisible(Hw))
#else
	if (!IsWindowVisible(Hw))
#endif
		return '#';
	else if (IsIconic(Hw))
		return '-';
	else if (IsZoomed(Hw))
		return '@';
	else
		return '+';
}

void WINAPI SetWinMenuW(HMENU menu, const wchar_t *langFile, int VTFlag)
{
	int i;
	char Temp[MAXPATHLEN];
	HWND Hw;

	// delete all items in Window menu
	i = GetMenuItemCount(menu);
	if (i>0)
		do {
			i--;
			RemoveMenu(menu,i,MF_BYPOSITION);
		} while (i>0);

	i = 0;
	while (i<pm->NWin) {
		Hw = pm->WinList[i]; // get window handle
		if ((GetClassNameA(Hw,Temp,sizeof(Temp))>0) &&
		    ((strcmp(Temp,VTCLASSNAME)==0) ||
		     (strcmp(Temp,TEKCLASSNAME)==0))) {
			Temp[0] = '&';
			Temp[1] = (char)(0x31 + i);
			Temp[2] = ' ';
			Temp[3] = GetWindowTypeChar(Hw, NULL);
			Temp[4] = ' ';
			GetWindowTextA(Hw,&Temp[5],sizeof(Temp)-6);
			AppendMenuA(menu,MF_ENABLED | MF_STRING,ID_WINDOW_1+i,Temp);
			i++;
			if (i>8) {
				i = pm->NWin;
			}
		}
		else {
			UnregWin(Hw);
		}
	}
	if (VTFlag == 1) {
		static const DlgTextInfo MenuTextInfo[] = {
			{ ID_WINDOW_WINDOW, "MENU_WINDOW_WINDOW" },
			{ ID_WINDOW_MINIMIZEALL, "MENU_WINDOW_MINIMIZEALL" },
			{ ID_WINDOW_RESTOREALL, "MENU_WINDOW_RESTOREALL" },
			{ ID_WINDOW_CASCADEALL, "MENU_WINDOW_CASCADE" },
			{ ID_WINDOW_STACKED, "MENU_WINDOW_STACKED" },
			{ ID_WINDOW_SIDEBYSIDE, "MENU_WINDOW_SIDEBYSIDE" },
		};

		AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_WINDOW, "&Window");
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_MINIMIZEALL, "&Minimize All");
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_RESTOREALL, "&Restore All");
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_CASCADEALL, "&Cascade");
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_STACKED, "&Stacked");
		AppendMenuA(menu, MF_ENABLED | MF_STRING, ID_WINDOW_SIDEBYSIDE, "Side &by Side");

		SetI18nMenuStrsW(menu, "Tera Term", MenuTextInfo, _countof(MenuTextInfo), langFile);

		if (pm->WinUndoFlag) {
			wchar_t *uimsg;
			if (pm->WinUndoStyle == WIN_CASCADE)
				GetI18nStrWW("Tera Term", "MENU_WINDOW_CASCADE_UNDO", L"&Undo - Cascade", langFile, &uimsg);
			else if (pm->WinUndoStyle == WIN_STACKED)
				GetI18nStrWW("Tera Term", "MENU_WINDOW_STACKED_UNDO", L"&Undo - Stacked", langFile, &uimsg);
			else
				GetI18nStrWW("Tera Term", "MENU_WINDOW_SIDEBYSIDE_UNDO", L"&Undo - Side by Side", langFile, &uimsg);
			AppendMenuW(menu, MF_ENABLED | MF_STRING, ID_WINDOW_UNDO, uimsg);
			free(uimsg);
		}

	}
	else {
		wchar_t *uimsg;
		GetI18nStrWW("Tera Term", "MENU_WINDOW_WINDOW", L"&Window", langFile, &uimsg);
		AppendMenuW(menu,MF_ENABLED | MF_STRING,ID_TEKWINDOW_WINDOW, uimsg);
		free(uimsg);
	}
}

void WINAPI SetWinList(HWND HWin, HWND HDlg, int IList)
{
	int i;
	char Temp[MAXPATHLEN];
	HWND Hw;

	for (i=0; i<pm->NWin; i++) {
		Hw = pm->WinList[i]; // get window handle
		if ((GetClassName(Hw,Temp,sizeof(Temp))>0) &&
		    ((strcmp(Temp,VTCLASSNAME)==0) ||
		     (strcmp(Temp,TEKCLASSNAME)==0))) {
			Temp[0] = GetWindowTypeChar(Hw, HWin);
			Temp[1] = ' ';
			GetWindowText(Hw,&Temp[2],sizeof(Temp)-3);
			SendDlgItemMessage(HDlg, IList, LB_ADDSTRING,
			                   0, (LPARAM)Temp);
			if (Hw==HWin) {
				SendDlgItemMessage(HDlg, IList, LB_SETCURSEL, i,0);
			}
		}
		else {
			UnregWin(Hw);
		}
	}
}

void WINAPI SelectWin(int WinId)
{
	if ((WinId>=0) && (WinId<pm->NWin)) {
		ForegroundWin(pm->WinList[WinId]);
	}
}

void WINAPI ForegroundWin(HWND hWnd)
{
	/* �E�B���h�E���ő剻����эŏ�������Ă����ꍇ�A���̏�Ԃ��ێ��ł���悤�ɁA
	 * SW_SHOWNORMAL ���� SW_SHOW �֕ύX�����B
	 * (2009.11.8 yutaka)
	 * �E�B���h�E���ŏ�������Ă���Ƃ��͌��̃T�C�Y�ɖ߂�(SW_RESTORE)�悤�ɂ����B
	 * (2009.11.9 maya)
	 */
	if (IsIconic(hWnd)) {
		ShowWindow(hWnd, SW_RESTORE);
	}
	else {
		ShowWindow(hWnd, SW_SHOW);
	}
	SetForegroundWindow(hWnd);
}

void WINAPI SelectNextWin(HWND HWin, int Next, BOOL SkipIconic)
{
	int i;

	i = 0;
	while ((i < pm->NWin) && (pm->WinList[i]!=HWin)) {
		i++;
	}
	if (pm->WinList[i]!=HWin) {
		return;
	}

	do {
		i += Next;
		if (i >= pm->NWin) {
			i = 0;
		}
		else if (i < 0) {
			i = pm->NWin-1;
		}

		if (pm->WinList[i] == HWin) {
			break;
		}
	} while ((SkipIconic && IsIconic(pm->WinList[i])) || !IsWindowVisible(pm->WinList[i]));

	SelectWin(i);
}

void WINAPI ShowAllWin(int stat) {
	int i;

	for (i=0; i < pm->NWin; i++) {
		ShowWindow(pm->WinList[i], stat);
	}
}

void WINAPI UndoAllWin(void) {
	int i;
	WINDOWPLACEMENT rc0;
	RECT rc;
	int stat = SW_RESTORE;
	int multi_mon = 0;

	if (HasMultiMonitorSupport()) {
		multi_mon = 1;
	}

	// ��x�A����������t���O�͗��Ƃ��B
	pm->WinUndoFlag = FALSE;

	memset(&rc0, 0, sizeof(rc0));

	for (i=0; i < pm->NWin; i++) {
		// �����w��ŁA�O��̏�Ԃ��c���Ă���ꍇ�́A�E�B���h�E�̏�Ԃ����ɖ߂��B
		if (stat == SW_RESTORE && memcmp(&pm->WinPrevRect[i], &rc0, sizeof(rc0)) != 0) {
			rc = pm->WinPrevRect[i].rcNormalPosition;

			// NT4.0, 95 �̓}���`���j�^API�ɔ�Ή�
			if (multi_mon) {
				// �Ώۃ��j�^�̏����擾
				HMONITOR hMonitor;
				MONITORINFO mi;
				hMonitor = pMonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
				mi.cbSize = sizeof(MONITORINFO);
				pGetMonitorInfoA(hMonitor, &mi);

				// �ʒu�␳�i�����O��ŉ𑜓x���ς���Ă���ꍇ�ւ̑΍�j
				if (rc.right > mi.rcMonitor.right) {
					rc.left -= rc.right - mi.rcMonitor.right;
					rc.right = mi.rcMonitor.right;
				}
				if (rc.left < mi.rcMonitor.left) {
					rc.right += mi.rcMonitor.left - rc.left;
					rc.left = mi.rcMonitor.left;
				}
				if (rc.bottom > mi.rcMonitor.bottom) {
					rc.top -= rc.bottom - mi.rcMonitor.bottom;
					rc.bottom = mi.rcMonitor.bottom;
				}
				if (rc.top < mi.rcMonitor.top) {
					rc.bottom += mi.rcMonitor.top - rc.top;
					rc.top = mi.rcMonitor.top;
				}
			}

			// �E�B���h�E�ʒu����
			SetWindowPos(
				pm->WinList[i], NULL,
				rc.left,
				rc.top,
				rc.right - rc.left,
				rc.bottom - rc.top,
				SWP_NOZORDER);

			// �E�B���h�E�̏�ԕ���
			ShowWindow(pm->WinList[i], pm->WinPrevRect[i].showCmd);

		} else {
			ShowWindow(pm->WinList[i], stat);
		}
	}
}

HWND WINAPI GetNthWin(int n)
{
	if (n<pm->NWin) {
		return pm->WinList[n];
	}
	else {
		return NULL;
	}
}

int WINAPI GetRegisteredWindowCount(void)
{
	return (pm->NWin);
}

// �L���ȃE�B���h�E��T���A���݈ʒu���L�������Ă����B
static void get_valid_window_and_memorize_rect(HWND myhwnd, HWND hwnd[], int *num, int style)
{
	int i, n;
	WINDOWPLACEMENT wndPlace;

	// ���ɖ߂�(Undo)���j���[�͈�x�����\��������B
	if (pm->WinUndoFlag == FALSE) {
		pm->WinUndoFlag = TRUE;
	} else {
		// ���łɃ��j���[���\������Ă��āA���ȑO�Ɠ����X�^�C�����I�����ꂽ��A
		// ���j���[�������B
		// Windows8�ł́A����ɘA�����ē����X�^�C����I�����Ă����j���[���������܂܂����A
		// Tera Term�ł̓��j���[���\������邽�߁A���삪�قȂ�B
		if (pm->WinUndoStyle == style)
			pm->WinUndoFlag = FALSE;
	}
	pm->WinUndoStyle = style;

	n = 0;
	for (i = 0 ; i < pm->NWin ; i++) {
		// ���݈ʒu���o���Ă����B
		wndPlace.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(pm->WinList[i], &wndPlace);
		pm->WinPrevRect[i] = wndPlace;

		// �������g�͐擪�ɂ���B
		if (pm->WinList[i] == myhwnd) {
			hwnd[n] = hwnd[0];
			hwnd[0] = myhwnd;
		} else {
			hwnd[n] = pm->WinList[i];
		}
		n++;
	}
	*num = n;
}

// �E�B���h�E�����E�ɕ��ׂĕ\������(Show Windows Side by Side)
void WINAPI ShowAllWinSidebySide(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_SIDEBYSIDE);
	TileWindows(NULL, MDITILE_VERTICAL, NULL, n, hwnd);
}

// �E�B���h�E���㉺�ɕ��ׂĕ\������(Show Windows Stacked)
void WINAPI ShowAllWinStacked(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_STACKED);
	TileWindows(NULL, MDITILE_HORIZONTAL, NULL, n, hwnd);
}

// �E�B���h�E���d�˂ĕ\������(Cascade)
void WINAPI ShowAllWinCascade(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_CASCADE);
	CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, n, hwnd);
}

// �STera Term�ɏI���w�����o���B
void WINAPI BroadcastClosingMessage(HWND myhwnd)
{
	int i, max;
	HWND hwnd[MAXNWIN];

	// Tera Term���I��������ƁA���L���������ω����邽�߁A
	// ��������o�b�t�@�ɃR�s�[���Ă����B
	max = pm->NWin;
	for (i = 0 ; i < pm->NWin ; i++) {
		hwnd[i] = pm->WinList[i];
	}

	for (i = 0 ; i < max ; i++) {
		// �������g�͍Ō�ɂ���B
		if (hwnd[i] == myhwnd)
			continue;

		PostMessage(hwnd[i], WM_USER_NONCONFIRM_CLOSE, 0, 0);
	}
	PostMessage(myhwnd, WM_USER_NONCONFIRM_CLOSE, 0, 0);
}


int WINAPI CommReadRawByte(PComVar cv, LPBYTE b)
{
	if ( ! cv->Ready ) {
		return 0;
	}

	if ( cv->InBuffCount>0 ) {
		*b = cv->InBuff[cv->InPtr];
		cv->InPtr++;
		cv->InBuffCount--;
		if ( cv->InBuffCount==0 ) {
			cv->InPtr = 0;
		}
		return 1;
	}
	else {
		cv->InPtr = 0;
		return 0;
	}
}

static void LogBinSkip(PComVar cv, int add)
{
	if (cv->LogBinSkip != NULL) {
		cv->LogBinSkip(add);
	}
}

void WINAPI CommInsert1Byte(PComVar cv, BYTE b)
{
	if ( ! cv->Ready ) {
		return;
	}

	if (cv->InPtr == 0) {
		memmove(&(cv->InBuff[1]),&(cv->InBuff[0]),cv->InBuffCount);
	}
	else {
		cv->InPtr--;
	}
	cv->InBuff[cv->InPtr] = b;
	cv->InBuffCount++;

	LogBinSkip(cv, 1);
}

static void Log1Bin(PComVar cv, BYTE b)
{
	if (cv->Log1Bin != NULL) {
		cv->Log1Bin(b);
	}
}

int WINAPI CommRead1Byte(PComVar cv, LPBYTE b)
{
	int c;

	if ( ! cv->Ready ) {
		return 0;
	}

	if ( cv->TelMode ) {
		c = 0;
	}
	else {
		c = CommReadRawByte(cv,b);
	}

	if ((c==1) && cv->TelCRFlag) {
		cv->TelCRFlag = FALSE;
		if (*b==0) {
			c = 0;
		}
	}

	if ( c==1 ) {
		if ( cv->IACFlag ) {
			cv->IACFlag = FALSE;
			if ( *b != 0xFF ) {
				cv->TelMode = TRUE;
				CommInsert1Byte(cv,*b);
				LogBinSkip(cv, -1);
				c = 0;
			}
		}
		else if ((cv->PortType==IdTCPIP) && (*b==0xFF)) {
			if (!cv->TelFlag && cv->TelAutoDetect) { /* TTPLUG */
				cv->TelFlag = TRUE;
			}
			if (cv->TelFlag) {
				cv->IACFlag = TRUE;
				c = 0;
			}
		}
		else if (cv->TelFlag && ! cv->TelBinRecv && (*b==0x0D)) {
			cv->TelCRFlag = TRUE;
		}
	}

	if (c == 1) {
		Log1Bin(cv, *b);
	}

	return c;
}

int WINAPI CommRawOut(PComVar cv, /*const*/ PCHAR B, int C)
{
	int a;

	if ( ! cv->Ready ) {
		return C;
	}

	if (C > OutBuffSize - cv->OutBuffCount) {
		a = OutBuffSize - cv->OutBuffCount;
	}
	else {
		a = C;
	}
	if ( cv->OutPtr > 0 ) {
		memmove(&(cv->OutBuff[0]),&(cv->OutBuff[cv->OutPtr]),cv->OutBuffCount);
		cv->OutPtr = 0;
	}
	memcpy(&(cv->OutBuff[cv->OutBuffCount]),B,a);
	cv->OutBuffCount = cv->OutBuffCount + a;
	return a;
}

int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready ) {
		return C;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') && ! cv->TelBinSend ) {
			d[Len++] = '\x00';
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if ( OutBuffSize - cv->OutBuffCount - Len >= 0 ) {
			CommRawOut(cv, d, Len);
			a = 1;
		}
		else {
			a = 0;
		}

		i += a;
	}
	return i;
}

/**
 *	�f�[�^(������)���o�̓o�b�t�@�֏�������
 *
 *	�w��f�[�^�����ׂď������߂Ȃ��ꍇ�͏������܂Ȃ�
 *	CommRawOut() �͏������߂镪������������
 *
 *	@retval	TRUE	�o�͂ł���
 *	@retval	FALSE	�o�͂ł��Ȃ�����(buffer full)
 */
static BOOL WriteOutBuff(PComVar cv, const char *TempStr, int TempLen)
{
	BOOL output;

	if (TempLen == 0) {
		// ����0�ŏ������݂ɗ���ꍇ����
		return TRUE;
	}

	output = FALSE;
	if (cv->TelLineMode) {
		const BOOL Full = OutBuffSize - cv->LineModeBuffCount - TempLen < 0;
		if (!Full) {
			output = TRUE;
			memcpy(&(cv->LineModeBuff[cv->LineModeBuffCount]), TempStr, TempLen);
			cv->LineModeBuffCount += TempLen;
			if (cv->Flush) {
				cv->FlushLen = cv->LineModeBuffCount;
			}
		}
		if (cv->FlushLen > 0) {
			const int OutLen = CommRawOut(cv, cv->LineModeBuff, cv->FlushLen);
			cv->FlushLen -= OutLen;
			cv->LineModeBuffCount -= OutLen;
			memmove(cv->LineModeBuff, &(cv->LineModeBuff[OutLen]), cv->LineModeBuffCount);
		}
		cv->Flush = FALSE;
	}
	else {
		const BOOL Full = OutBuffSize-cv->OutBuffCount-TempLen < 0;
		if (! Full) {
			output = TRUE;
			CommRawOut(cv, (char *)TempStr, TempLen);
		}
	}
	return output;
}

/**
 *	�f�[�^(������)����̓o�b�t�@�֏�������
 *	���̓o�b�t�@�֓���� -> �G�R�[�����
 *
 *	@retval	TRUE	�o�͂ł���
 *	@retval	FALSE	�o�͂ł��Ȃ�����(buffer full)
 */
static BOOL WriteInBuff(PComVar cv, const char *TempStr, int TempLen)
{
	BOOL Full;

	if (TempLen == 0) {
		return TRUE;
	}

	Full = InBuffSize-cv->InBuffCount-TempLen < 0;
	if (! Full) {
		memcpy(&(cv->InBuff[cv->InBuffCount]),TempStr,TempLen);
		cv->InBuffCount = cv->InBuffCount + TempLen;
		return TRUE;
	}
	return FALSE;
}

/**
 *	���̓o�b�t�@�̐擪�ɋ󂫂���������l�߂�
 */
static void PackInBuff(PComVar cv)
{
	if ( (cv->InPtr>0) && (cv->InBuffCount>0) ) {
		memmove(cv->InBuff,&(cv->InBuff[cv->InPtr]),cv->InBuffCount);
		cv->InPtr = 0;
	}
}

int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready ) {
		return C;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if (B[i] == CR) {
			if ( cv->TelFlag && ! cv->TelBinSend ) {
				d[Len++] = '\x00';
			}
			if (cv->TelLineMode) {
				cv->Flush = TRUE;
			}
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if (WriteOutBuff(cv, d, Len)) {
			a = 1;
			i++;
		} else {
			a = 0;
		}
	}
	return i;
}

/**
 *	�o�͗p
 *	@param	cv
 *	@param	u32			���͕���
 *	@param	check_only	TRUE�ŏ����͍s�킸�A
 *	@param	TempStr		�o�͕�����
 *	@param	StrLen		TempStr�ւ̏o�͕�����
 *	@retval	�������s����
 */
static BOOL OutControl(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data)
{
	PComVar cv = data;
	const wchar_t d = u32;
	size_t TempLen = 0;
	BOOL retval = FALSE;
	if (check_only == TRUE) {
		/* �`�F�b�N�̂� */
		if (d == CR || d == BS || d == 0x15/*ctrl-u*/) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	if (d==CR) {
		TempStr[TempLen++] = 0x0d;
		if (cv->CRSend==IdCRLF) {
			TempStr[TempLen++] = 0x0a;
		}
		else if ((cv->CRSend ==IdCR) &&
				 cv->TelFlag && ! cv->TelBinSend) {
			TempStr[TempLen++] = 0;
		}
		else if (cv->CRSend == IdLF) {
			TempStr[TempLen-1] = 0x0a;
		}
		if (cv->TelLineMode) {
			cv->Flush = TRUE;
		}
		retval = TRUE;
	}
	else if (d== BS) {
		if (cv->TelLineMode) {
			if (cv->FlushLen < cv->LineModeBuffCount) {
				cv->LineModeBuffCount--;
			}
		}
		else {
			TempStr[TempLen++] = BS;
		}
		retval = TRUE;
	}
	else if (d==0x15) { // ctrl-u
		if (cv->TelLineMode) {
			cv->LineModeBuffCount = cv->FlushLen;
		}
		else {
			TempStr[TempLen++] = 0x15;
		}
		retval = TRUE;
	}
	*StrLen = TempLen;
	return retval;
}
static BOOL ControlEcho(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data)
{
	PComVar cv = data;
	const wchar_t d = u32;
	size_t TempLen = 0;
	BOOL retval = FALSE;
	if (check_only == TRUE) {
		/* �`�F�b�N�̂� */
		if (d == CR || (d == 0x15/*ctrl-u*/ && cv->TelLineMode)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	if (d==CR) {
		TempStr[TempLen++] = 0x0d;
		if (cv->CRSend==IdCRLF) {
			TempStr[TempLen++] = 0x0a;
		}
		else if ((cv->CRSend ==IdCR) && cv->TelFlag && ! cv->TelBinSend) {
			TempStr[TempLen++] = 0;
		}
		else if (cv->CRSend == IdLF) {
			TempStr[TempLen-1] = 0x0a;
		}
		retval = TRUE;
	}
	else if (d==0x15/*ctrl-u*/ && cv->TelLineMode) {
		// Move to top of line (CHA "\033[G") and erase line (EL "\033[K")
		memcpy(TempStr, "\033[G\033[K", 6);
		TempLen += 6;
		retval = TRUE;
	}
	*StrLen = TempLen;
	return retval;
}

/**
 * CommTextOut() �� wchar_t ��
 *
 *	@retval		�o�͕�����(wchar_t�P��)
 */
int WINAPI CommTextOutW(PComVar cv, const wchar_t *B, int C)
{
	char TempStr[12];
	BOOL Full = FALSE;
	int i = 0;
	while (! Full && (i < C)) {
		// �o�͗p�f�[�^���쐬
		size_t TempLen = 0;
		size_t output_char_count;	// �����������
		output_char_count = MakeOutputString(cv->StateSend, &B[i], C - i, TempStr, &TempLen, OutControl, cv);

		// �f�[�^���o�̓o�b�t�@��
		if (WriteOutBuff(cv, TempStr, TempLen)) {
			i += output_char_count;		// output_char_count ������ ��������
		} else {
			Full = TRUE;
		}
	} // end of "while {}"
	_CrtCheckMemory();
	return i;
}

/**
 * CommTextEcho() �� wchar_t ��
 *
 *	@retval		�o�͕�����(wchar_t�P��)
 */
int WINAPI CommTextEchoW(PComVar cv, const wchar_t *B, int C)
{
	char TempStr[12];
	BOOL Full = FALSE;
	int i = 0;
	while (! Full && (i < C)) {
		// �o�͗p�f�[�^���쐬
		size_t TempLen = 0;
		size_t output_char_count;	// �����������
		output_char_count = MakeOutputString(cv->StateEcho, &B[i], C - i, TempStr, &TempLen, ControlEcho, cv);

		// �f�[�^���o�̓o�b�t�@��
		if (WriteInBuff(cv, TempStr, TempLen)) {
			i += output_char_count;		// output_char_count ������ ��������
		} else {
			Full = TRUE;
		}
	} // end of "while {}"
	_CrtCheckMemory();
	return i;
}

int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready )
		return C;

	PackInBuff(cv);

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') &&
		     ! cv->TelBinSend ) {
			d[Len] = 0x00;
			Len++;
		}

		if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len] = '\xff';
			Len++;
		}

		if (WriteInBuff(cv, d, Len)) {
			a = 1;
			i++;
		} else {
			a = 0;
		}
	}
	return i;
}

/**
 *	���L�������ւ̃|�C���^���Z�b�g
 */
DllExport void WINAPI SetPMPtr(PMap pm_)
{
	pm = pm_;
}

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
#ifdef _DEBUG
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
			WinCompatInit();
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			// TODO ttermpro.exe�ōs��
//			CloseSharedMemory(pm, HMap);
			break;
	}
	return TRUE;
}
