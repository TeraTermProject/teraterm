/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2008-2017 TeraTerm Project
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

/* Routines for dialog boxes */
#include "teraterm.h"
#include <stdio.h>
#include <commctrl.h>

void EnableDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		EnableWindow(HControl,TRUE);
	}
}

void DisableDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		EnableWindow(HControl,FALSE);
	}
}

void ShowDlgItem(HWND HDlg, int FirstId, int LastId)
{
	int i;
	HWND HControl;

	for (i = FirstId ; i <= LastId ; i++) {
		HControl = GetDlgItem(HDlg, i);
		ShowWindow(HControl,SW_SHOW);
	}
}

void SetRB(HWND HDlg, int R, int FirstId, int LastId)
{
	HWND HControl;
	DWORD Style;

	if ( R<1 ) {
		return;
	}
	if ( FirstId+R-1 > LastId ) {
		return;
	}
	HControl = GetDlgItem(HDlg, FirstId + R - 1);
	SendMessage(HControl, BM_SETCHECK, 1, 0);
	Style = GetClassLong(HControl, GCL_STYLE);
	SetClassLong(HControl, GCL_STYLE, Style | WS_TABSTOP);
}

void GetRB(HWND HDlg, LPWORD R, int FirstId, int LastId)
{
	int i;

	*R = 0;
	for (i = FirstId ; i <= LastId ; i++) {
		if (SendDlgItemMessage(HDlg, i, BM_GETCHECK, 0, 0) != 0) {
			*R = i - FirstId + 1;
			return;
		}
	}
}

void SetDlgNum(HWND HDlg, int id_Item, LONG Num)
{
	char Temp[16];

	/* In Win16, SetDlgItemInt can not be used to display long integer. */
	_snprintf_s(Temp,sizeof(Temp),_TRUNCATE,"%d",Num);
	SetDlgItemText(HDlg,id_Item,Temp);
}

void InitDlgProgress(HWND HDlg, int id_Progress, int *CurProgStat) {
	HWND HProg;
	HProg = GetDlgItem(HDlg, id_Progress);

	*CurProgStat = 0;

	SendMessage(HProg, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100));
	SendMessage(HProg, PBM_SETSTEP, (WPARAM)1, 0);
	SendMessage(HProg, PBM_SETPOS, (WPARAM)0, 0);

	ShowWindow(HProg, SW_SHOW);
	return;
}

void SetDlgPercent(HWND HDlg, int id_Item, int id_Progress, LONG a, LONG b, int *p)
{
	// 20MB以上のファイルをアップロードしようとすると、buffer overflowで
	// 落ちる問題への対処。(2005.3.18 yutaka)
	// cf. http://sourceforge.jp/tracker/index.php?func=detail&aid=5713&group_id=1412&atid=5333
	double Num; 
	char NumStr[10]; 

	if (b==0) {
		Num = 100.0; 
	}
	else {
		Num = 100.0 * (double)a / (double)b; 
	}
	_snprintf_s(NumStr,sizeof(NumStr),_TRUNCATE,"%3.1f%%",Num); 
	SetDlgItemText(HDlg, id_Item, NumStr); 

	if (id_Progress != 0 && p != NULL && *p >= 0 && (double)*p < Num) {
		*p = (int)Num;
		SendMessage(GetDlgItem(HDlg, id_Progress), PBM_SETPOS, (WPARAM)*p, 0);
	}
}

void SetDlgTime(HWND HDlg, int id_Item, DWORD stime, int bytes)
{
	static int prev_elapsed;
	int elapsed, rate;
	char buff[64];

	elapsed = (GetTickCount() - stime) / 1000;

	if (elapsed == 0) {
		prev_elapsed = 0;
		SetDlgItemText(HDlg, id_Item, "0:00");
		return;
	}

	if (elapsed == prev_elapsed) {
		return;
	}
	prev_elapsed = elapsed;

	rate = bytes / elapsed;
	if (rate < 1200) {
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%d:%02d (%dBytes/s)", elapsed / 60, elapsed % 60, rate); 
	}
	else if (rate < 1200000) {
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%d:%02d (%d.%02dKB/s)", elapsed / 60, elapsed % 60, rate / 1000, rate / 10 % 100); 
	}
	else {
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%d:%02d (%d.%02dMB/s)", elapsed / 60, elapsed % 60, rate / (1000 * 1000), rate / 10000 % 100); 
	}

	SetDlgItemText(HDlg, id_Item, buff);
}

void SetDropDownList(HWND HDlg, int Id_Item, PCHAR far *List, int nsel)
{
	int i;

	i = 0;
	while (List[i] != NULL) {
		SendDlgItemMessage(HDlg, Id_Item, CB_ADDSTRING,
		                   0, (LPARAM)List[i]);
		i++;
	}
	SendDlgItemMessage(HDlg, Id_Item, CB_SETCURSEL,nsel-1,0);
}

LONG GetCurSel(HWND HDlg, int Id_Item)
{
	LONG n;

	n = SendDlgItemMessage(HDlg, Id_Item, CB_GETCURSEL, 0, 0);
	if (n==CB_ERR) {
		n = 0;
	}
	else {
		n++;
	}

	return n;
}
