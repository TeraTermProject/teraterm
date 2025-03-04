/*
 * Copyright (C) 2022- TeraTerm Project
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

/*
 *	common contol �� TOOLTIPS_CLASS ���g�p�����c�[���`�b�v
 */
#include <windows.h>
#include <commctrl.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "ttlib.h"
#include "tipwin2.h"

typedef struct tagTipWinData {
	HWND hDlg;
	HWND hTip;
	int EdittextId = -1; // EDITTEXT�̃c�[���`�b�v�����p
} TipWin2;

TipWin2 *TipWin2Create(HINSTANCE hInstance, HWND hDlg)
{
	HINSTANCE hInst = hInstance;
	if (hInstance == NULL) {
		hInst = (HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE);
	}

	HWND hTip = CreateWindowExW(NULL, TOOLTIPS_CLASSW, NULL,
								WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
								CW_USEDEFAULT, CW_USEDEFAULT,
								CW_USEDEFAULT, CW_USEDEFAULT,
								hDlg, NULL,
								hInst, NULL);
	if (hTip == NULL) {
		return NULL;
	}
	//SendMessageW(hTip, TTM_SETMAXTIPWIDTH, 0, INT_MAX);	// OS�ɂ���ĈႤ?
	SendMessageW(hTip, TTM_SETMAXTIPWIDTH, 0, 200);

	TipWin2 *tWin = (TipWin2 *)calloc(1, sizeof(TipWin2));
	if (tWin == NULL) {
		return NULL;
	}
	tWin->hTip = hTip;
	tWin->hDlg = hDlg;

	return tWin;
}

void TipWin2Destroy(TipWin2 *tWin)
{
	if (tWin == NULL) {
		assert(FALSE);
		return;
	}
	if (tWin->EdittextId != -1) {
		KillTimer(tWin->hTip, tWin->EdittextId);
	}
	DestroyWindow(tWin->hTip);
	tWin->hTip = NULL;
	free(tWin);
}

/**
 * @brief	�c�[���`�b�v��o�^����
 * @param	tWin
 * @param	id �_�C�A���O�̃R���g���[��ID
 * @param	text �c�[���`�b�v
 *			NULL�̂Ƃ��o�^�폜
 */
void TipWin2SetTextW(TipWin2 *tWin, int id, const wchar_t *text)
{
	TOOLINFOW toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = tWin->hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;	// TTF_IDISHWND �������rect�͎Q�Ƃ���Ȃ�
	toolInfo.uId = (UINT_PTR)GetDlgItem(tWin->hDlg, id);
	toolInfo.lpszText = (LPWSTR)text;	// text �� SendMessage() ���ɑ��݂���Ηǂ�
	SendMessageW(tWin->hTip, TTM_ADDTOOLW, 0, (LPARAM)&toolInfo);
}

/**
 * @brief �c�[���`�b�v���폜����
 * @param tWin
 * @param id �_�C�A���O�̃R���g���[��ID
 *
 *	�폜�ł��Ȃ����Ƃ�����?
 *	TipWin2SetTextW() �� text = NULL ���g�p�����ق����悳����
 */
void TipWin2Delete(TipWin2 *tWin, int id)
{
	TOOLINFOW toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = tWin->hDlg;
	toolInfo.uFlags = TTF_IDISHWND;
	toolInfo.uId = (UINT_PTR)GetDlgItem(tWin->hDlg, id);
	PostMessageW(tWin->hTip, TTM_DELTOOLW, 0, (LPARAM)&toolInfo);
}

/**
 * @brief �c�[���`�b�v��L��/����������
 * @param tWin
 *
 *	����̃R���g���[���ɐݒ肷�邱�Ƃ͂ł��Ȃ�
 */
void TipWin2Activate(TipWin2 *tWin, BOOL active)
{
	PostMessageW(tWin->hTip, TTM_ACTIVATE ,active, 0);
}

/**
 * @brief	�c�[���`�b�v�̍��W���f�B�X�v���C����͂ݏo���Ȃ��悤���炷
 * @param	tWin
 * @param	point �c�[���`�b�v������̍��W (in, out)
 */
void TipWin2MovePointToDisplay(const TipWin2 *tWin, POINT *point)
{
	RECT desktop, rect;
	int win_x, win_y, win_width, win_height;

	GetWindowRect(tWin->hTip, &rect);
	win_x = point->x;
	win_y = point->y;
	win_height = rect.bottom - rect.top;
	win_width  = rect.right - rect.left;

	GetDesktopRect(tWin->hDlg, &desktop);
	if (win_y < desktop.top) {
		win_y = desktop.top;
	}
	else if (win_y + win_height > desktop.bottom) {
		win_y = desktop.bottom - win_height;
	}
	if (win_x < desktop.left) {
		win_x = desktop.left;
	}
	else if (win_x + win_width > desktop.right) {
		win_x = desktop.right - win_width;
	}

	point->x = win_x;
	point->y = win_y;
}

/**
 * @brief	EDITTEXT�̓��͐����p�̃c�[���`�b�v�̏����p�^�C�}�[
 * @param	hTip �c�[���`�b�v�̃E�B���h�E�n���h��
 * @param	id EDITTEXT�̃R���g���[��ID
 */
static void CALLBACK TipWin2HideEdittextErrMsgProc(const HWND hTip, const UINT /*uMsg*/, const UINT_PTR id, const DWORD /*dwTime*/) // uMsg,dwTime is unused
{
	KillTimer(hTip, id);
	TOOLINFOW toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = GetParent(hTip);
	toolInfo.uId = id;
	SendMessageW(hTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
}

/**
 * @brief	EDITTEXT�ɓ��͐����p�̃c�[���`�b�v��\������(ES_NUMBER����)
 * @param	tWin
 * @param	hEdit EDITTEXT�̃E�B���h�E�n���h��
 * @param	id EDITTEXT�̃R���g���[��ID
 * @param	icon �c�[���`�b�v�̃A�C�R��
 * @param	title �c�[���`�b�v�̃^�C�g��
 * @param	text �c�[���`�b�v�̃��b�Z�[�W
 */
void TipWin2ShowEdittextErrMsgW(TipWin2 *tWin, const HWND hEdit, const int id, const int icon, const wchar_t *title, const wchar_t *text)
{
	// �c�[���`�b�v�̃A�C�R���A�^�C�g���A���b�Z�[�W��ݒ�
	SendMessage(tWin->hTip, TTM_SETTITLEW, icon, (LPARAM)title);
	TOOLINFOW toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = tWin->hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	toolInfo.uId = (UINT_PTR)id;
	toolInfo.lpszText = (LPWSTR)text;
	SendMessageW(tWin->hTip, TTM_ADDTOOLW, 0, (LPARAM)&toolInfo);

	// �c�[���`�b�v�̈ʒu���v�Z
	RECT rect;
	POINT point;
	SendMessage(hEdit, EM_GETRECT, 0, (LPARAM)(&rect));
	point.y = rect.bottom - rect.top;
	DWORD startPos, endPos;
	SendMessage(hEdit, EM_GETSEL, (WPARAM)(&startPos), (LPARAM)(&endPos));
	int nDim = SendMessage(hEdit, EM_POSFROMCHAR, startPos, 0);
	if (nDim == -1) {
		point.x = rect.right - rect.left;
	} else {
		point.x = LOWORD(nDim);
	}
	ClientToScreen(hEdit, &point);

	// �c�[���`�b�v��L����
	SendMessageW(tWin->hTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);

	// �c�[���`�b�v���f�B�X�v���C����͂ݏo���Ȃ����W���擾
	TipWin2MovePointToDisplay(tWin, &point);

	// �c�[���`�b�v��\��
	SendMessageW(tWin->hTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x, point.y));

	// �c�[���`�b�v�����p�̃^�C�}�[��ݒ�
	if (tWin->EdittextId != -1) {
		KillTimer(tWin->hTip, tWin->EdittextId);
	}
	tWin->EdittextId = id;
	SetTimer(tWin->hTip, id, 1500, TipWin2HideEdittextErrMsgProc);
}
