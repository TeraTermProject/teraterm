
#include <windows.h>
#include "dlglib.h"

extern BOOL CallOnIdle(LONG lCount);

typedef struct {
	DLGPROC OrigProc;	// Dialog proc
	LONG_PTR OrigUser;	// DWLP_USER
	LPARAM ParamInit;
	int DlgResult;
} TTDialogData;

static TTDialogData *TTDialogTmpData;

static int TTDoModal(HWND hDlgWnd)
{
	LONG lIdleCount = 0;
	MSG Msg;
	TTDialogData *data = (TTDialogData *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	for (;;)
	{
		if (!IsWindow(hDlgWnd)) {
			// ウインドウが閉じられた
			return IDCANCEL;
		}
#if defined(_DEBUG)
		if (!IsWindowVisible(hDlgWnd)) {
			// 誤ってEndDialog()が使われた? -> TTEndDialog()を使うこと
			::ShowWindow(hDlgWnd, SW_SHOWNORMAL);
		}
#endif
		int DlgRet = data->DlgResult;
		if (DlgRet != 0) {
			// TTEndDialog()が呼ばれた
			return DlgRet;
		}

		if(!::PeekMessage(&Msg, NULL, NULL, NULL, PM_NOREMOVE))
		{
			// メッセージがない
			// OnIdel() を処理する
			if (!CallOnIdle(lIdleCount++)) {
				// Idle処理がなくなった
				lIdleCount = 0;
				Sleep(10);
			}
			continue;
		}
		else
		{
			// メッセージがある

			// pump message
			BOOL quit = !::GetMessage(&Msg, NULL, NULL, NULL);
			if (quit) {
				// QM_QUIT
				PostQuitMessage(0);
				return IDCANCEL;
			}

			if (!::IsDialogMessage(hDlgWnd, &Msg)) {
				// ダイアログ以外の処理
				::TranslateMessage(&Msg);
				::DispatchMessage(&Msg);
			}
		}
	}

	// ここには来ない
	return IDOK;
}

static INT_PTR CALLBACK TTDialogProc(
	HWND hDlgWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	TTDialogData *data = (TTDialogData *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	if (msg == WM_INITDIALOG) {
		TTDialogData *data = (TTDialogData *)lParam;
		LONG_PTR r = SetWindowLongPtr(hDlgWnd, DWLP_USER, 0);
		DWORD d = GetLastError();
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)lParam);
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)lParam);
		lParam = data->ParamInit;
	}

	if (data == NULL) {
		// WM_INITDIALOGよりも前は設定されていない
		data = TTDialogTmpData;
	} else {
		// TTEndDialog()が呼ばれたとき、DWLP_USER が参照できない
		TTDialogTmpData = data;
	}

	SetWindowLongPtr(hDlgWnd, DWLP_DLGPROC, (LONG_PTR)data->OrigProc);
	SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)data->OrigUser);
	LRESULT Result = data->OrigProc(hDlgWnd, msg, wParam, lParam);
	data->OrigProc = (DLGPROC)GetWindowLongPtr(hDlgWnd, DWLP_DLGPROC);
	data->OrigUser = GetWindowLongPtr(hDlgWnd, DWLP_USER);
	SetWindowLongPtr(hDlgWnd, DWLP_DLGPROC, (LONG_PTR)TTDialogProc);
	SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)data);

	if (msg == WM_NCDESTROY) {
		SetWindowLongPtr(hDlgWnd, DWLP_USER, 0);
		free(data);
	}

	return Result;
}

/**
 *	EndDialog() 互換関数
 */
BOOL TTEndDialog(HWND hDlgWnd, INT_PTR nResult)
{
	TTDialogData *data = TTDialogTmpData;
	data->DlgResult = nResult;
	return TRUE;
}

/**
 *	CreateDialogIndirectParam() 互換関数
 */
HWND TTCreateDialogIndirectParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,			// オーナーウィンドウのハンドル
	DLGPROC lpDialogFunc,		// ダイアログボックスプロシージャへのポインタ
	LPARAM lParamInit)			// 初期化値
{
	TTDialogData *data = (TTDialogData *)malloc(sizeof(TTDialogData));
	data->OrigProc = lpDialogFunc;
	data->OrigUser = 0;
	data->ParamInit = lParamInit;
	data->DlgResult = 0;
#if 0
	HRSRC hResource = ::FindResource(hInstance, lpTemplateName, RT_DIALOG);
	HANDLE hDlgTemplate = ::LoadResource(hInstance, hResource);
	DLGTEMPLATE *lpTemplate = (DLGTEMPLATE *)::LockResource(hDlgTemplate);
#else
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, lpTemplateName);
#endif
	TTDialogTmpData = data;
	HWND hDlgWnd = CreateDialogIndirectParam(
		hInstance, lpTemplate, hWndParent, TTDialogProc, (LPARAM)data);
	TTDialogTmpData = NULL;
	ShowWindow(hDlgWnd, SW_SHOW);
    UpdateWindow(hDlgWnd);
#if 1
	free(lpTemplate);
#endif
	return hDlgWnd;
}

/**
 *	DialogBoxParam() 互換関数
 *		EndDialog()ではなく、TTEndDialog()を使用すること
 */
int TTDialogBoxParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,			// オーナーウィンドウのハンドル
	DLGPROC lpDialogFunc,		// ダイアログボックスプロシージャへのポインタ
	LPARAM lParamInit)			// 初期化値
{
	HWND hDlgWnd = TTCreateDialogIndirectParam(
		hInstance, lpTemplateName,
		hWndParent, lpDialogFunc, lParamInit);
	EnableWindow(hWndParent, FALSE);
	int DlgResult = TTDoModal(hDlgWnd);
	::DestroyWindow(hDlgWnd);
	EnableWindow(hWndParent, TRUE);
	return DlgResult;
}

