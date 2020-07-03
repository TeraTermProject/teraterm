/*
 * Copyright (C) 2020 TeraTerm Project
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

// vtwinから分離

#include "teraterm_conf.h"
#include "teraterm.h"
#include "tttypes.h"
#include "ttcommon.h"
#include "ttwinman.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <windowsx.h>
#include <commctrl.h>
#include <wchar.h>	// for wmemcpy_s()

#include "ttsetup.h"
#include "keyboard.h"	// for ShiftKey() ControlKey()
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "layer_for_unicode.h"
#include "codeconv.h"
#include "sendmem.h"
//#include "clipboar.h"

#include "broadcast.h"


// WM_COPYDATAによるプロセス間通信の種別 (2005.1.22 yutaka)
#define IPC_BROADCAST_COMMAND 1		// 全端末へ送信
#define IPC_MULTICAST_COMMAND 2		// 任意の端末群へ送信

/*
 * COPYDATASTRUCT
 *
 * dwData
 *  IPC_BROADCAST_COMMAND
 * lpData
 *  +--------------+--+
 *  |string        |\0|
 *  +--------------+--+
 *  <-------------->
 * cbData
 *  strlen(string) + 1
 *	(wcslen(string) + 1) * sizeof(wchar_t)
 *  bufの直後には \0 は付かない
 *
 * dwData
 *  IPC_MULTICAST_COMMAND
 * lpData
 *  +------+--------------+--+
 *  |name\0|string        |\0|
 *  +------+--------------+--+
 *  <--------------------->
 * cbData
 *  strlen(string) + 1 + strlen(string)
 *	(wcslen(name) + 1 + wcslen(string)) * sizeof(wchar_t)
 *  bufの直後には \0 は付かない
 */

#define BROADCAST_LOGFILE "broadcast.log"

static void ApplyBroadCastCommandHisotry(HWND Dialog, char *historyfile)
{
	char EntName[13];
	char Command[HostNameMaxLength+1];
	int i = 1;

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_RESETCONTENT, 0, 0);
	do {
		_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Command%d", i);
		GetPrivateProfileString("BroadcastCommands",EntName,"",
		                        Command,sizeof(Command), historyfile);
		if (strlen(Command) > 0) {
			SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_ADDSTRING,
			                   0, (LPARAM)Command);
		}
		i++;
	} while ((i <= ts.MaxBroadcatHistory) && (strlen(Command)>0));

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, EM_LIMITTEXT,
	                   HostNameMaxLength-1, 0);

	SendDlgItemMessage(Dialog, IDC_COMMAND_EDIT, CB_SETCURSEL,0,0);
}

// ドロップダウンの中のエディットコントロールを
// サブクラス化するためのウインドウプロシージャ
static WNDPROC OrigBroadcastEditProc; // Original window procedure
static HWND BroadcastWindowList;
static LRESULT CALLBACK BroadcastEditProc(HWND dlg, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{
	char buf[1024];
	int len;

	switch (msg) {
		case WM_CREATE:
			break;

		case WM_DESTROY:
			break;

		case WM_LBUTTONUP:
			// すでにテキストが入力されている場合は、カーソルを末尾へ移動させる。
			len = GetWindowText(dlg, buf, sizeof(buf));
			SendMessage(dlg, EM_SETSEL, len, len);
			SetFocus(dlg);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			SetFocus(dlg);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			{
				int i;
				HWND hd;
				int count;

				if (wParam == 0x0d) {  // Enter key
					SetWindowText(dlg, "");
					SendMessage(dlg, EM_SETSEL, 0, 0);
				}

				count = (int)SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
				for (i = 0 ; i < count ; i++) {
					if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
						hd = GetNthWin(i);
						if (hd) {
							PostMessage(hd, msg, wParam, lParam);
						}
					}
				}
			}
			break;

		case WM_CHAR:
			// 入力した文字がIDC_COMMAND_EDITに残らないように捨てる
			return FALSE;

		default:
			return _CallWindowProcW(OrigBroadcastEditProc, dlg, msg, wParam, lParam);
	}

	return FALSE;
}

static void UpdateBroadcastWindowList(HWND hWnd)
{
	int i, count;
	HWND hd;
	TCHAR szWindowText[256];

	SendMessage(hWnd, LB_RESETCONTENT, 0, 0);

	count = GetRegisteredWindowCount();
	for (i = 0 ; i < count ; i++) {
		hd = GetNthWin(i);
		if (hd == NULL) {
			break;
		}

		GetWindowText(hd, szWindowText, 256);
		SendMessage(hWnd, LB_INSERTSTRING, -1, (LPARAM)szWindowText);
	}
}

static COPYDATASTRUCT *BuildBroadcastCDSW(const wchar_t *buf)
{
	COPYDATASTRUCT *cds = (COPYDATASTRUCT *)malloc(sizeof(COPYDATASTRUCT));
	size_t buflen = wcslen(buf);

	cds->dwData = IPC_BROADCAST_COMMAND;
	cds->cbData = (DWORD)(buflen * sizeof(wchar_t));	// '\0' は含まない
	cds->lpData = (void *)buf;

	return cds;
}

static COPYDATASTRUCT *BuildMulticastCDSW(const wchar_t *name, const wchar_t *buf)
{
	size_t buflen = wcslen(buf);
	size_t nlen = wcslen(name) + 1;
	size_t msglen = nlen + buflen;
	wchar_t *msg = (wchar_t *)malloc(msglen * sizeof(wchar_t));
	if (msg == NULL) {
		return NULL;
	}
	wcscpy_s(msg, msglen, name);
	wmemcpy_s(msg + nlen, msglen - nlen, buf, buflen);

	COPYDATASTRUCT *cds = (COPYDATASTRUCT *)malloc(sizeof(COPYDATASTRUCT));
	if (cds == NULL) {
		free(msg);
		return NULL;
	}
	cds->dwData = IPC_MULTICAST_COMMAND;
	cds->cbData = (DWORD)(msglen * sizeof(wchar_t));
	cds->lpData = msg;

	return cds;
}

/*
 * ダイアログで選択されたウィンドウのみ、もしくは親ウィンドウのみに送るブロードキャストモード。
 * リアルタイムモードが off の時に利用される。
 */
static void SendBroadcastMessageToSelected(HWND HVTWin, HWND hWnd, int parent_only, const wchar_t *buf)
{
	COPYDATASTRUCT *cds = BuildBroadcastCDSW(buf);

	if (parent_only) {
		// 親ウィンドウのみに WM_COPYDATA メッセージを送る
		SendMessage(GetParent(hWnd), WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)cds);
	}
	else {
		// ダイアログで選択されたウィンドウにメッセージを送る
		int count = (int)SendMessage(BroadcastWindowList, LB_GETCOUNT, 0, 0);
		for (int i = 0 ; i < count ; i++) {
			// リストボックスで選択されているか
			if (SendMessage(BroadcastWindowList, LB_GETSEL, i, 0)) {
				HWND hd = GetNthWin(i);
				if (hd != NULL) {
					// WM_COPYDATAを使って、プロセス間通信を行う。
					SendMessage(hd, WM_COPYDATA, (WPARAM)HVTWin, (LPARAM)cds);
				}
			}
		}
	}

	free(cds);
}

/**
 * 全 Tera Term へCOPYDATASTRUCTを送信する
 *	@param[in]	hWnd	送信元
 *	@param[in]	cds		COPYDATASTRUCT
 */
static void SendCDS(HWND hWnd, const COPYDATASTRUCT *cds)
{
	int count = GetRegisteredWindowCount();
	for (int i = 0 ; i < count ; i++) {
		HWND hd = GetNthWin(i);
		if (hd == NULL) {
			break;
		}
		// WM_COPYDATAを使って、プロセス間通信を行う。
		SendMessage(hd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)cds);
	}
}

/*
 * 全 Tera Term へメッセージを送信するブロードキャストモード。
 * "sendbroadcast"マクロコマンドからのみ利用される。
 */
void SendBroadcastMessage(HWND HVTWin, HWND hWnd, const wchar_t *buf)
{
	COPYDATASTRUCT *cds = BuildBroadcastCDSW(buf);
	SendCDS(HVTWin, cds);
	free(cds);
}

static COPYDATASTRUCT *BuildMulticastCopyData(const char *name, const char *buf)
{
	size_t buflen = strlen(buf);
	size_t nlen = strlen(name) + 1;
	size_t msglen = nlen + buflen;
	char *msg = (char *)malloc(msglen);
	if (msg == NULL) {
		return NULL;
	}
	strcpy_s(msg, msglen, name);
	memcpy_s(msg + nlen, msglen - nlen, buf, buflen);

	COPYDATASTRUCT *cds = (COPYDATASTRUCT *)malloc(sizeof(COPYDATASTRUCT));
	if (cds == NULL) {
		free(msg);
		return NULL;
	}
	cds->dwData = IPC_MULTICAST_COMMAND;
	cds->cbData = (DWORD)msglen;
	cds->lpData = msg;

	return cds;
}

/*
 * 任意の Tera Term 群へメッセージを送信するマルチキャストモード。厳密には、
 * ブロードキャスト送信を行い、受信側でメッセージを取捨選択する。
 * "sendmulticast"マクロコマンドからのみ利用される。
 */
void SendMulticastMessage(HWND HVTWin_, HWND hWnd, const wchar_t *name, const wchar_t *buf)
{
	COPYDATASTRUCT *cdsW = BuildMulticastCDSW(name, buf);
	SendCDS(HVTWin_, cdsW);
	free(cdsW->lpData);
	free(cdsW);
}

void SetMulticastName(const wchar_t *name)
{
	// TODO MulticastName を wchar_t 化
	char *nameA = ToCharW(name);
	strncpy_s(ts.MulticastName, sizeof(ts.MulticastName), nameA, _TRUNCATE);
	free(nameA);
}

static int CompareMulticastName(const wchar_t *name)
{
	// TODO MulticastName を wchar_t 化
	wchar_t *MulticastNameW = ToWcharA(ts.MulticastName);
	int result = wcscmp(MulticastNameW, name);
	free(MulticastNameW);
	return result;
}

//
// すべてのターミナルへ同一コマンドを送信するモードレスダイアログの表示
// (2005.1.22 yutaka)
//
static INT_PTR CALLBACK BroadcastCommandDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_BROADCAST_TITLE" },
		{ IDC_HISTORY_CHECK, "DLG_BROADCAST_HISTORY" },
		{ IDC_ENTERKEY_CHECK, "DLG_BROADCAST_ENTER" },
		{ IDC_PARENT_ONLY, "DLG_BROADCAST_PARENTONLY" },
		{ IDC_REALTIME_CHECK, "DLG_BROADCAST_REALTIME" },
		{ IDOK, "DLG_BROADCAST_SUBMIT" },
		{ IDCANCEL, "BTN_CLOSE" },
	};
	LRESULT checked;
	LRESULT history;
	char historyfile[MAX_PATH];
	static HWND hwndBroadcast     = NULL; // Broadcast dropdown
	static HWND hwndBroadcastEdit = NULL; // Edit control on Broadcast dropdown
	// for resize
	RECT rc_dlg, rc, rc_ok;
	POINT p;
	static int ok2right, cancel2right, cmdlist2ok, list2bottom, list2right;
	// for update list
	const int list_timer_id = 100;
	const int list_timer_tick = 1000; // msec
	static int prev_instances = 0;
	// for status bar
	static HWND hStatus = NULL;
	static int init_width, init_height;

	switch (msg) {
		case WM_SHOWWINDOW:
			if (wp) {  // show
				// Tera Term window list
				UpdateBroadcastWindowList(GetDlgItem(hWnd, IDC_LIST));
				return TRUE;
			}
			break;

		case WM_INITDIALOG:
			// ラジオボタンのデフォルトは CR にする。
			SendMessage(GetDlgItem(hWnd, IDC_RADIO_CR), BM_SETCHECK, BST_CHECKED, 0);
			// デフォルトでチェックボックスを checked 状態にする。
			SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			// history を反映する (2007.3.3 maya)
			if (ts.BroadcastCommandHistory) {
				SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CHECK), BM_SETCHECK, BST_CHECKED, 0);
			}
			GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
			ApplyBroadCastCommandHisotry(hWnd, historyfile);

			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hWnd, IDC_COMMAND_EDIT));

			// サブクラス化させてリアルタイムモードにする (2008.1.21 yutaka)
			hwndBroadcast = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
			hwndBroadcastEdit = GetWindow(hwndBroadcast, GW_CHILD);
			OrigBroadcastEditProc = (WNDPROC)_SetWindowLongPtrW(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)BroadcastEditProc);
			// デフォルトはon。残りはdisable。
			SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_SETCHECK, BST_CHECKED, 0);  // default on
			EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), FALSE);

			// Tera Term window list
			BroadcastWindowList = GetDlgItem(hWnd, IDC_LIST);
			UpdateBroadcastWindowList(BroadcastWindowList);

			// I18N
			SetDlgTexts(hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

			// ダイアログの初期サイズを保存
			GetWindowRect(hWnd, &rc_dlg);
			init_width = rc_dlg.right - rc_dlg.left;
			init_height = rc_dlg.bottom - rc_dlg.top;

			// 現在サイズから必要な値を計算
			GetClientRect(hWnd, &rc_dlg);
			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hWnd, &p);

			GetWindowRect(GetDlgItem(hWnd, IDOK), &rc_ok);
			ok2right = p.x - rc_ok.left;

			GetWindowRect(GetDlgItem(hWnd, IDCANCEL), &rc);
			cancel2right = p.x - rc.left;

			GetWindowRect(GetDlgItem(hWnd, IDC_COMMAND_EDIT), &rc);
			cmdlist2ok = rc_ok.left - rc.right;

			GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &rc);
			list2bottom = p.y - rc.bottom;
			list2right = p.x - rc.right;

			// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
			InitCommonControls();
			hStatus = CreateStatusWindow(
				WS_CHILD | WS_VISIBLE |
				CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hWnd, 1);

			// リスト更新タイマーの開始
			SetTimer(hWnd, list_timer_id, list_timer_tick, NULL);

			return FALSE;

		case WM_COMMAND:
			switch (wp) {
			case IDC_ENTERKEY_CHECK | (BN_CLICKED << 16):
				// チェックの有無により、ラジオボタンの有効・無効を決める。
				checked = SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) { // 改行コードあり
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), TRUE);

				} else {
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
				}
				return TRUE;

			case IDC_REALTIME_CHECK | (BN_CLICKED << 16):
				checked = SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) { // checkあり
					// new handler
					hwndBroadcast = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
					hwndBroadcastEdit = GetWindow(hwndBroadcast, GW_CHILD);
					OrigBroadcastEditProc = (WNDPROC)_SetWindowLongPtrW(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)BroadcastEditProc);

					EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_LIST), TRUE);  // true
				} else {
					// restore old handler
					_SetWindowLongPtrW(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)OrigBroadcastEditProc);

					EnableWindow(GetDlgItem(hWnd, IDC_HISTORY_CHECK), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CRLF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_CR), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_RADIO_LF), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_PARENT_ONLY), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_LIST), TRUE);  // true
				}
				return TRUE;
			}

			switch (LOWORD(wp)) {
				case IDOK:
					{
						wchar_t buf[256 + 3];
						//memset(buf, 0, sizeof(buf));

						// realtime modeの場合、Enter keyのみ送る。
						// cf. http://logmett.com/forum/viewtopic.php?f=8&t=1601
						// (2011.3.14 hirata)
						checked = SendMessage(GetDlgItem(hWnd, IDC_REALTIME_CHECK), BM_GETCHECK, 0, 0);
						if (checked & BST_CHECKED) { // checkあり
							wcsncpy_s(buf, _countof(buf), L"\n", _TRUNCATE);
							SetDlgItemTextA(hWnd, IDC_COMMAND_EDIT, "");
						}
						else {
							UINT ret = _GetDlgItemTextW(hWnd, IDC_COMMAND_EDIT, buf, 256 - 1);
							if (ret == 0) { // error
								memset(buf, 0, sizeof(buf));
							}

							// ブロードキャストコマンドの履歴を保存 (2007.3.3 maya)
							history = SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CHECK), BM_GETCHECK, 0, 0);
							if (history) {
								GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
								if (LoadTTSET()) {
									char *bufA = ToCharW(buf);	// TODO wchar_t 対応
									(*AddValueToList)(historyfile, bufA, "BroadcastCommands", "Command",
													  ts.MaxBroadcatHistory);
									free(bufA);
									FreeTTSET();
								}
								ApplyBroadCastCommandHisotry(hWnd, historyfile);
								ts.BroadcastCommandHistory = TRUE;
							}
							else {
								ts.BroadcastCommandHistory = FALSE;
							}
							checked = SendMessage(GetDlgItem(hWnd, IDC_ENTERKEY_CHECK), BM_GETCHECK, 0, 0);
							if (checked & BST_CHECKED) { // 改行コードあり
								if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_CRLF), BM_GETCHECK, 0, 0) & BST_CHECKED) {
									wcsncat_s(buf, _countof(buf), L"\r\n", _TRUNCATE);

								} else if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_CR), BM_GETCHECK, 0, 0) & BST_CHECKED) {
									wcsncat_s(buf, _countof(buf), L"\r", _TRUNCATE);

								} else if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_LF), BM_GETCHECK, 0, 0) & BST_CHECKED) {
									wcsncat_s(buf, _countof(buf), L"\n", _TRUNCATE);

								} else {
									wcsncat_s(buf, _countof(buf), L"\r", _TRUNCATE);

								}
							}
						}

						// 337: 2007/03/20 チェックされていたら親ウィンドウにのみ送信
						checked = SendMessage(GetDlgItem(hWnd, IDC_PARENT_ONLY), BM_GETCHECK, 0, 0);

						SendBroadcastMessageToSelected(HVTWin, hWnd, (int)checked, buf);
					}

					// モードレスダイアログは一度生成されると、アプリケーションが終了するまで
					// 破棄されないので、以下の「ウィンドウプロシージャ戻し」は不要と思われる。(yutaka)
#if 0
					_SetWindowLongPtrW(hwndBroadcastEdit, GWLP_WNDPROC, (LONG_PTR)OrigBroadcastEditProc);
#endif

					//EndDialog(hDlgWnd, IDOK);
					return TRUE;

				case IDCANCEL:
					EndDialog(hWnd, 0);
					//DestroyWindow(hWnd);

					return TRUE;

				case IDC_COMMAND_EDIT:
					if (HIWORD(wp) == CBN_DROPDOWN) {
						GetDefaultFName(ts.HomeDir, BROADCAST_LOGFILE, historyfile, sizeof(historyfile));
						ApplyBroadCastCommandHisotry(hWnd, historyfile);
					}
					return FALSE;

				case IDC_LIST:
					// 一般的なアプリケーションと同じ操作感を持たせるため、
					// 「SHIFT+クリック」による連続的な選択をサポートする。
					// (2009.9.28 yutaka)
					if (HIWORD(wp) == LBN_SELCHANGE && ShiftKey()) {
						int i, cur, prev;

						cur = ListBox_GetCurSel(BroadcastWindowList);
						prev = -1;
						for (i = cur - 1 ; i >= 0 ; i--) {
							if (ListBox_GetSel(BroadcastWindowList, i)) {
								prev = i;
								break;
							}
						}
						if (prev != -1) {
							// すでに選択済みの箇所があれば、そこから連続選択する。
							for (i = prev ; i < cur ; i++) {
								ListBox_SetSel(BroadcastWindowList, TRUE, i);
							}
						}
					}

					return FALSE;

				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			//DestroyWindow(hWnd);
			EndDialog(hWnd, 0);
			return TRUE;

		case WM_SIZE:
			{
				// 再配置
				int dlg_w, dlg_h;
				RECT rc_dlg;
				RECT rc;
				POINT p;

				// 新しいダイアログのサイズを得る
				GetClientRect(hWnd, &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				// OK button
				GetWindowRect(GetDlgItem(hWnd, IDOK), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDOK), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// Cancel button
				GetWindowRect(GetDlgItem(hWnd, IDCANCEL), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDCANCEL), 0,
				             dlg_w - cancel2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// Command Edit box
				GetWindowRect(GetDlgItem(hWnd, IDC_COMMAND_EDIT), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDC_COMMAND_EDIT), 0,
				             0, 0, dlg_w - p.x - ok2right - cmdlist2ok, p.y,
				             SWP_NOMOVE | SWP_NOZORDER);

				// List Edit box
				GetWindowRect(GetDlgItem(hWnd, IDC_LIST), &rc);
				p.x = rc.left;
				p.y = rc.top;
				ScreenToClient(hWnd, &p);
				SetWindowPos(GetDlgItem(hWnd, IDC_LIST), 0,
				             0, 0, dlg_w - p.x - list2right , dlg_h - p.y - list2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// status bar
				SendMessage(hStatus , msg , wp , lp);
			}
			return TRUE;

		case WM_GETMINMAXINFO:
			{
				// ダイアログの初期サイズより小さくできないようにする
				LPMINMAXINFO lpmmi;
				lpmmi = (LPMINMAXINFO)lp;
				lpmmi->ptMinTrackSize.x = init_width;
				lpmmi->ptMinTrackSize.y = init_height;
			}
			return FALSE;

		case WM_TIMER:
			{
				int n;

				if (wp != list_timer_id)
					break;

				n = GetRegisteredWindowCount();
				if (n != prev_instances) {
					prev_instances = n;
					UpdateBroadcastWindowList(BroadcastWindowList);
				}
			}
			return TRUE;

		case WM_VKEYTOITEM:
			// リストボックスでキー押下(CTRL+A)されたら、全選択。
			if ((HWND)lp == BroadcastWindowList) {
				if (ControlKey() && LOWORD(wp) == 'A') {
					int i, n;

					//OutputDebugPrintf("msg %x wp %x lp %x\n", msg, wp, lp);
					n = GetRegisteredWindowCount();
					for (i = 0 ; i < n ; i++) {
						ListBox_SetSel(BroadcastWindowList, TRUE, i);
					}
				}
			}
			return TRUE;

		default:
			//OutputDebugPrintf("msg %x wp %x lp %x\n", msg, wp, lp);
			return FALSE;
	}
	return TRUE;
}

static HWND hDlgWnd = NULL;

void BroadCastShowDialog(HINSTANCE hInst, HWND hWnd)
{
	RECT prc, rc;
	LONG x, y;

	if (hDlgWnd != NULL) {
		goto activate;
	}

	SetDialogFont(ts.DialogFontName, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");

	// CreateDialogW() で生成したダイアログは、
	// エディットボックスにIMEからの入力が化けることがある (20/05/27,Windows10 64bit)
	//   ペーストはok
	hDlgWnd = TTCreateDialog(hInst, MAKEINTRESOURCE(IDD_BROADCAST_DIALOG),
							 hWnd, BroadcastCommandDlgProc);

	if (hDlgWnd == NULL) {
		return;
	}

	// ダイアログをウィンドウの真上に配置する (2008.1.25 yutaka)
	::GetWindowRect(hWnd, &prc);
	::GetWindowRect(hDlgWnd, &rc);
	x = prc.left;
	y = prc.top - (rc.bottom - rc.top);
	if (y < 0) {
		y = 0;
	}
	::SetWindowPos(hDlgWnd, NULL, x, y,  0, 0, SWP_NOSIZE | SWP_NOZORDER);

activate:;
	::ShowWindow(hDlgWnd, SW_SHOW);
}

BOOL BroadCastReceive(const COPYDATASTRUCT *cds)
{
	wchar_t *strW_ptr;
	size_t strW_len = 0;

	switch (cds->dwData) {
	case IPC_BROADCAST_COMMAND: {
		strW_len = cds->cbData / sizeof(wchar_t);
		strW_ptr = (wchar_t *)malloc((strW_len + 1) * sizeof(wchar_t));
		wmemcpy_s(strW_ptr, strW_len, (wchar_t *)cds->lpData, strW_len);
		strW_ptr[strW_len] = 0;		// 念の為
		break;
	}
	case IPC_MULTICAST_COMMAND: {
		wchar_t *name = (wchar_t *)cds->lpData;

		// マルチキャスト名をチェックする
		if (CompareMulticastName(name) != 0) {
			// 名前が異なっているので何もしない
			return TRUE;
		}

		// マルチキャスト名の次の文字列を取得
		size_t nlen = wcslen(name);
		strW_len = cds->cbData  / sizeof(wchar_t) - nlen - 1;	// -1 = name の '\0'
		strW_ptr = (wchar_t *)malloc((strW_len + 1) * sizeof(wchar_t));
		wmemcpy_s(strW_ptr, strW_len, (wchar_t *)cds->lpData + nlen + 1, strW_len);
		strW_ptr[strW_len] = 0;		// 念の為
		break;
	}

	default:
		// 知らないメッセージの場合
		return TRUE;
	}

	// 端末へ文字列を送り込む
	SendMem *sm = SendMemTextW(strW_ptr, strW_len);
	if (sm != NULL) {
		SendMemInitEcho(sm, FALSE);
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_PER_LINE, 10, 0);
		SendMemStart(sm);
	}

	return TRUE;
}
