/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Clipboard routines */
#include "teraterm.h"
#include "tttypes.h"
#include "vtdisp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ttwinman.h"
#include "ttcommon.h"

#include "clipboar.h"
#include "tt_res.h"

// for clipboard copy
static HGLOBAL CBCopyHandle = NULL;
static PCHAR CBCopyPtr = NULL;

// for clipboard paste
static HGLOBAL CBMemHandle = NULL;
static PCHAR CBMemPtr = NULL;
static LONG CBMemPtr2 = 0;
static BOOL CBAddCR = FALSE;
static BYTE CBByte;
static BOOL CBRetrySend;
static BOOL CBRetryEcho;
static BOOL CBSendCR;
static BOOL CBDDE;

static HFONT DlgClipboardFont;

PCHAR CBOpen(LONG MemSize)
{
  if (MemSize==0) return (NULL);
  if (CBCopyHandle!=NULL) return (NULL);
  CBCopyPtr = NULL;
  CBCopyHandle = GlobalAlloc(GMEM_MOVEABLE, MemSize);
  if (CBCopyHandle == NULL) 
    MessageBeep(0);
  else {
    CBCopyPtr = GlobalLock(CBCopyHandle);
    if (CBCopyPtr == NULL)
    {
      GlobalFree(CBCopyHandle);
      CBCopyHandle = NULL;
      MessageBeep(0);
    }
  }
  return (CBCopyPtr);
}

void CBClose()
{
  BOOL Empty;
  if (CBCopyHandle==NULL) return;

  Empty = FALSE;
  if (CBCopyPtr!=NULL)
    Empty = (CBCopyPtr[0]==0);

  GlobalUnlock(CBCopyHandle);
  CBCopyPtr = NULL;

  if (OpenClipboard(HVTWin))
  {
    EmptyClipboard();
    if (! Empty)
      SetClipboardData(CF_TEXT, CBCopyHandle);
    CloseClipboard();
  }
  CBCopyHandle = NULL;
}

void CBStartPaste(HWND HWin, BOOL AddCR,
		  int BuffSize, PCHAR DataPtr, int DataSize)
//
//  DataPtr and DataSize are used only for DDE
//	  For clipboard, BuffSize should be 0
//	  DataSize should be <= BuffSize
{
  UINT Cf;

  if (! cv.Ready) return;
  if (TalkStatus!=IdTalkKeyb) return;

  CBAddCR = AddCR;

  if (BuffSize==0) // for clipboard
  {
    if (IsClipboardFormatAvailable(CF_TEXT))
      Cf = CF_TEXT;
    else if (IsClipboardFormatAvailable(CF_OEMTEXT))
      Cf = CF_OEMTEXT;
    else return;
  }

  CBMemHandle = NULL;
  CBMemPtr = NULL;
  CBMemPtr2 = 0;
  CBDDE = FALSE;
  if (BuffSize==0) //clipboard
  {
    if (OpenClipboard(HWin))
      CBMemHandle = GetClipboardData(Cf);
    if (CBMemHandle!=NULL) TalkStatus=IdTalkCB;
  }
  else { // dde
    CBMemHandle = GlobalAlloc(GHND,BuffSize);
    if (CBMemHandle != NULL)
    {
      CBDDE = TRUE;
      CBMemPtr = GlobalLock(CBMemHandle);
      if (CBMemPtr != NULL)
      {
	memcpy(CBMemPtr,DataPtr,DataSize);
	GlobalUnlock(CBMemHandle);
	CBMemPtr=NULL;
	TalkStatus=IdTalkCB;
      }
    }
  }
  CBRetrySend = FALSE;
  CBRetryEcho = FALSE;
  CBSendCR = FALSE;
  if (TalkStatus != IdTalkCB) CBEndPaste();
}

// この関数はクリップボードおよびDDEデータを端末へ送り込む。
//
// CBMemHandleハンドルはグローバル変数なので、この関数が終了するまでは、
// 次のクリップボードおよびDDEデータを処理することはできない（破棄される可能性あり）。
// また、データ列で null-terminate されていることを前提としているため、後続のデータ列は
// 無視される。
// (2006.11.6 yutaka)
void CBSend()
{
  int c;
  BOOL EndFlag;
  int SendBytes = 0;

  if (CBMemHandle==NULL) return;

  if (CBRetrySend)
  {
    CBRetryEcho = (ts.LocalEcho>0);
    c = CommTextOut(&cv,(PCHAR)&CBByte,1);
    CBRetrySend = (c==0);
    if (CBRetrySend) return;
  }

  if (CBRetryEcho)
  {
    c = CommTextEcho(&cv,(PCHAR)&CBByte,1);
    CBRetryEcho = (c==0);
    if (CBRetryEcho) return;
  }

  CBMemPtr = GlobalLock(CBMemHandle);
  if (CBMemPtr==NULL) return;
  
  do {
    if (CBSendCR && (CBMemPtr[CBMemPtr2]==0x0a))
      CBMemPtr2++;

    EndFlag = (CBMemPtr[CBMemPtr2]==0);
    if (! EndFlag)
    {
      CBByte = CBMemPtr[CBMemPtr2];
      CBMemPtr2++;
// Decoding characters which are encoded by MACRO
//   to support NUL character sending
//
//  [encoded character] --> [decoded character]
//         01 01        -->     00
//         01 02        -->     01
      if (CBByte==0x01) /* 0x01 from MACRO */
      {
        CBByte = CBMemPtr[CBMemPtr2];
        CBMemPtr2++;
        CBByte = CBByte - 1; // character just after 0x01
      }
    }
    else if (CBAddCR)
    {
	  EndFlag = FALSE;
      CBAddCR = FALSE;
      CBByte = 0x0d;
    }
    else {
      CBEndPaste();
      return;
    }

    if (! EndFlag)
    {
      c = CommTextOut(&cv,(PCHAR)&CBByte,1);
      CBSendCR = (CBByte==0x0D);
      CBRetrySend = (c==0);
      if ((! CBRetrySend) &&
	  (ts.LocalEcho>0))
      {
	c = CommTextEcho(&cv,(PCHAR)&CBByte,1);
	CBRetryEcho = (c==0);
      }

      // SSH2 で大きいデータを貼り付けたときにエコーバックが欠けることが
      // あるので、500 バイト送信するごとに 10ms(この値に根拠はない) 待つ
      // workaround (2008.8.22 maya)
      SendBytes++;
      if (SendBytes > 500) {
        SendBytes = 0;
        Sleep(10);
      }
    }
    else
      c=0;
  }
  while (c>0);

  if (CBMemPtr!=NULL)
  {
    GlobalUnlock(CBMemHandle);
    CBMemPtr=NULL;
  }
}

void CBEndPaste()
{
  TalkStatus = IdTalkKeyb;

  if (CBMemHandle!=NULL)
  {
    if (CBMemPtr!=NULL)
      GlobalUnlock(CBMemHandle);
    if (CBDDE)
      GlobalFree(CBMemHandle);
  }
  if (!CBDDE) CloseClipboard();

  CBDDE = FALSE;
  CBMemHandle = NULL;
  CBMemPtr = NULL;
  CBMemPtr2 = 0;
  CBAddCR = FALSE;
}


static char *ClipboardPtr = NULL;
static int PasteCanceled = 0;

static LRESULT CALLBACK OnClipboardDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LOGFONT logfont;
	HFONT font;
	char uimsg[MAX_UIMSG];
	//char *p;
	POINT p;
	RECT rc_dsk, rc_dlg;
	int dlg_height, dlg_width;
	OSVERSIONINFO osvi;
	static int ok2right, info2bottom, edit2ok, edit2info;
	RECT rc_edit, rc_ok, rc_cancel, rc_info;

	switch (msg) {
		case WM_INITDIALOG:
#if 0
			for (p = ClipboardPtr; *p ; p++) {
				char buf[20];
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%02x ", *p);
				OutputDebugString(buf);
			}
#endif

			font = (HFONT)SendMessage(hDlgWnd, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_TAHOMA_FONT", hDlgWnd, &logfont, &DlgClipboardFont, ts.UILanguageFile)) {
				SendDlgItemMessage(hDlgWnd, IDC_EDIT, WM_SETFONT, (WPARAM)DlgClipboardFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hDlgWnd, IDC_CLIPBOARD_INFO, WM_SETFONT, (WPARAM)DlgClipboardFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hDlgWnd, IDOK, WM_SETFONT, (WPARAM)DlgClipboardFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hDlgWnd, IDCANCEL, WM_SETFONT, (WPARAM)DlgClipboardFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgClipboardFont = NULL;
			}

			GetWindowText(hDlgWnd, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_CLIPBOARD_TITLE", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetWindowText(hDlgWnd, ts.UIMsg);
			GetDlgItemText(hDlgWnd, IDC_CLIPBOARD_INFO, uimsg, sizeof(uimsg));
			get_lang_msg("DLG_CLIPBOARD_INFO", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hDlgWnd, IDC_CLIPBOARD_INFO, ts.UIMsg);
			GetDlgItemText(hDlgWnd, IDCANCEL, uimsg, sizeof(uimsg));
			get_lang_msg("BTN_CANCEL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hDlgWnd, IDCANCEL, ts.UIMsg);
			GetDlgItemText(hDlgWnd, IDOK, uimsg, sizeof(uimsg));
			get_lang_msg("BTN_OK", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
			SetDlgItemText(hDlgWnd, IDOK, ts.UIMsg);

			SendMessage(GetDlgItem(hDlgWnd, IDC_EDIT), WM_SETTEXT, 0, (LPARAM)ClipboardPtr);

			DispConvScreenToWin(CursorX, CursorY, &p.x, &p.y);
			ClientToScreen(HVTWin, &p);

			// キャレットが画面からはみ出しているときに貼り付けをすると
			// 確認ウインドウが見えるところに表示されないことがある。
			// ウインドウからはみ出した場合に調節する (2008.4.24 maya)
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 4) {
				// NT4.0 はマルチモニタAPIに非対応
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_dsk, 0);
			}
			else {
				HMONITOR hm;
				POINT pt;
				MONITORINFO mi;

				pt.x = p.x;
				pt.y = p.y;
				hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

				mi.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hm, &mi);
				rc_dsk = mi.rcWork;
			}
			GetWindowRect(hDlgWnd, &rc_dlg);
			dlg_height = rc_dlg.bottom-rc_dlg.top;
			dlg_width  = rc_dlg.right-rc_dlg.left;
			if (p.y < rc_dsk.top) {
				p.y = rc_dsk.top;
			}
			else if (p.y + dlg_height > rc_dsk.bottom) {
				p.y = rc_dsk.bottom - dlg_height;
			}
			if (p.x < rc_dsk.left) {
				p.x = rc_dsk.left;
			}
			else if (p.x + dlg_width > rc_dsk.right) {
				p.x = rc_dsk.right - dlg_width;
			}

			SetWindowPos(hDlgWnd, NULL, p.x, p.y,
			             0, 0, SWP_NOSIZE | SWP_NOZORDER);

			// 現在サイズから必要な値を計算
			GetClientRect(hDlgWnd,                                 &rc_dlg);
			GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
			GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);
			GetWindowRect(GetDlgItem(hDlgWnd, IDC_CLIPBOARD_INFO), &rc_info);

			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hDlgWnd, &p);
			ok2right = p.x - rc_ok.left;
			info2bottom = p.y - rc_info.top;
			edit2ok = rc_ok.left - rc_edit.right;
			edit2info = rc_info.top - rc_edit.bottom;

			// サイズを復元
			SetWindowPos(hDlgWnd, NULL, 0, 0,
			             ts.PasteDialogSize.cx, ts.PasteDialogSize.cy,
			             SWP_NOZORDER | SWP_NOMOVE);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
				{
					int len = SendMessage(GetDlgItem(hDlgWnd, IDC_EDIT), WM_GETTEXTLENGTH, 0, 0);
					HGLOBAL hMem;
					char *buf;

					hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
					buf = GlobalLock(hMem);
					SendMessage(GetDlgItem(hDlgWnd, IDC_EDIT), WM_GETTEXT, len + 1, (LPARAM)buf);
					GlobalUnlock(hMem);

					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);

					// hMemはクリップボードが保持しているので、破棄してはいけない。

					if (DlgClipboardFont != NULL) {
						DeleteObject(DlgClipboardFont);
					}

					EndDialog(hDlgWnd, IDOK);
				}
					break;

				case IDCANCEL:
					PasteCanceled = 1;

					if (DlgClipboardFont != NULL) {
						DeleteObject(DlgClipboardFont);
					}

					EndDialog(hDlgWnd, IDCANCEL);
					break;

				default:
					return FALSE;
			}

#if 0
		case WM_CLOSE:
			PasteCanceled = 1;
			EndDialog(hDlgWnd, 0);
			return TRUE;
#endif

		case WM_SIZE:
			{
				// 再配置
				POINT p;
				int dlg_w, dlg_h;

				GetClientRect(hDlgWnd,                                 &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
				GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);
				GetWindowRect(GetDlgItem(hDlgWnd, IDCANCEL),           &rc_cancel);
				GetWindowRect(GetDlgItem(hDlgWnd, IDC_CLIPBOARD_INFO), &rc_info);

				// OK
				p.x = rc_ok.left;
				p.y = rc_ok.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDOK), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// CANCEL
				p.x = rc_cancel.left;
				p.y = rc_cancel.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDCANCEL), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// INFO
				p.x = rc_info.left;
				p.y = rc_info.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_CLIPBOARD_INFO), 0,
				             p.x, dlg_h - info2bottom, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// EDIT
				p.x = rc_edit.left;
				p.y = rc_edit.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT), 0,
				             0, 0, dlg_w - p.x - edit2ok - ok2right, dlg_h - p.y - edit2info - info2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// サイズを保存
				GetWindowRect(hDlgWnd, &rc_dlg);
				ts.PasteDialogSize.cx = rc_dlg.right - rc_dlg.left;
				ts.PasteDialogSize.cy = rc_dlg.bottom - rc_dlg.top;
			}
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

//
// クリップボードに改行コードが含まれていたら、確認ダイアログを表示する。
// クリップボードの変更も可能。
//
// return 0: Cancel
//        1: Paste OK
//
// (2008.2.3 yutaka)
//
int CBStartPasteConfirmChange(HWND HWin)
{
	UINT Cf;
	HANDLE hText;
	char *pText;
	int pos;
	int ret = 0;

	if (ts.ConfirmChangePaste == 0)
		return 1;

	if (! cv.Ready) 
		goto error;
	if (TalkStatus!=IdTalkKeyb)
		goto error;

	if (IsClipboardFormatAvailable(CF_TEXT))
		Cf = CF_TEXT;
	else if (IsClipboardFormatAvailable(CF_OEMTEXT))
		Cf = CF_OEMTEXT;
	else 
		goto error;

	if (!OpenClipboard(HWin)) 
		goto error;

	hText = GetClipboardData(Cf);

	if (hText != NULL) {
		pText = (char *)GlobalLock(hText);
		pos = strcspn(pText, "\r\n");  // 改行が含まれていたら
		if (pText[pos] != '\0') {
			ClipboardPtr = pText;
			PasteCanceled = 0;
			ret = DialogBox(hInst, MAKEINTRESOURCE(IDD_CLIPBOARD_DIALOG),
							HVTWin, (DLGPROC)OnClipboardDlgProc);
			if (ret == 0 || ret == -1) {
				ret = GetLastError();
			}

			if (PasteCanceled) {
				ret = 0;
				GlobalUnlock(hText);
				CloseClipboard();
				goto error;
			}

		}

		ret = 1;

		GlobalUnlock(hText);
	}

	CloseClipboard();

error:
	return (ret);
}
