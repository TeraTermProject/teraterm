/*
 * Copyright (C) 2024- TeraTerm Project
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

#include "color_sample.h"

typedef struct ColorSampleTag {
	HWND hWnd;
	WNDPROC OrigProc;
	int dummy;
	COLORREF fore_color;
	COLORREF back_color;
	HFONT SampleFont;
} ColorSampleWork;

static LRESULT CALLBACK CSProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ColorSampleWork *work = (ColorSampleWork *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	LRESULT Result;

	switch (msg) {
	case WM_PAINT: {
		int i, x, y;
		int DX[3];
		TEXTMETRICA Metrics;
		RECT TestRect;
		int FW, FH;

		PAINTSTRUCT ps;
		HDC hDC;
		hDC = BeginPaint(hWnd, &ps);

		SetTextColor(hDC, work->fore_color);
		SetBkColor(hDC, work->back_color);
		SelectObject(hDC, work->SampleFont);
		GetTextMetricsA(hDC, &Metrics);
		FW = Metrics.tmAveCharWidth;
		FH = Metrics.tmHeight;
		for (i = 0; i <= 2; i++) {
			DX[i] = FW;
		}
		GetClientRect(hWnd, &TestRect);
		x = (int)((TestRect.left + TestRect.right) / 2 - FW * 1.5);
		y = (TestRect.top + TestRect.bottom - FH) / 2;
		ExtTextOutA(hDC, x, y, ETO_CLIPPED | ETO_OPAQUE, &TestRect, "ABC", 3, &(DX[0]));
		break;
	}
	default:
		break;
	}

	Result = CallWindowProcW(work->OrigProc, hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_NCDESTROY: {
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
		free(work);
		break;
	}
	default:
		break;
	}

	return Result;
}

/**
 * 	@brief カラーサンプル初期化
 *
 *	@param hWnd		ダイアログのstatic text
 *	@param hFont	フォント
 *					(フォントの破棄は呼び出し側で行うこと)
 */
ColorSample *ColorSampleInit(HWND hWnd, HFONT hFont)
{
	ColorSampleWork *work = (ColorSampleWork *)calloc(sizeof(*work), 1);
	work->hWnd = hWnd;
	work->SampleFont = hFont;
	work->OrigProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)CSProc);
	SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)work);
	InvalidateRect(hWnd, NULL, TRUE);
	return work;
}

/**
 * 	@brief カラーサンプル色設定
 */
void ColorSampleSetColor(ColorSample *work, COLORREF fore_color, COLORREF back_color)
{
	work->fore_color = fore_color;
	work->back_color = back_color;
	InvalidateRect(work->hWnd, NULL, TRUE);
}
