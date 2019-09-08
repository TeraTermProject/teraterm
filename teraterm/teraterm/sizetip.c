/*
 * Copyright (C) 2008-2019 TeraTerm Project
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
/* original idea from PuTTY 0.60 windows/sizetip.c */

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "ttwinman.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "TipWin.h"

static TipWin *SizeTip;
static int tip_enabled = 0;

/**
 *	point を
 *	スクリーンからはみ出している場合、入るように補正する
 *	NearestMonitor が TRUE のとき、最も近いモニタ
 *	FALSEのとき、マウスのあるモニタに移動させる
 *	ディスプレイの端から FrameWidth(pixel) より離れるようにする
 */
static void FixPosFromFrame(POINT *point, int FrameWidth, BOOL NearestMonitor)
{
	if (HasMultiMonitorSupport()) {
		// マルチモニタがサポートされている場合
		HMONITOR hm;
		MONITORINFO mi;
		int ix, iy;

		// 元の座標を保存しておく
		ix = point->x;
		iy = point->y;

		hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONULL);
		if (hm == NULL) {
			if (NearestMonitor) {
				// 最も近いモニタに表示する
				hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			} else {
				// スクリーンからはみ出している場合はマウスのあるモニタに表示する
				GetCursorPos(point);
				hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			}
		}

		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hm, &mi);
		if (ix < mi.rcMonitor.left + FrameWidth) {
			ix = mi.rcMonitor.left + FrameWidth;
		}
		if (iy < mi.rcMonitor.top + FrameWidth) {
			iy = mi.rcMonitor.top + FrameWidth;
		}
		
		point->x = ix;
		point->y = iy;
	}
	else
	{
		// マルチモニタがサポートされていない場合
		if (point->x < FrameWidth) {
			point->x = FrameWidth;
		}
		if (point->y < FrameWidth) {
			point->y = FrameWidth;
		}
	}
}

void UpdateSizeTip(HWND src, int cx, int cy)
{
	TCHAR str[32];

	if (!tip_enabled)
		return;

	/* Generate the tip text */

	_stprintf_s(str, _countof(str), _T("%dx%d"), cx, cy);

	if (SizeTip == NULL) {
		RECT wr;
		POINT point;
		int w, h;

		// 文字列の縦横サイズを取得する
		TipWinGetTextWidthHeight(src, str, &w, &h);

		// ウィンドウの位置を取得
		GetWindowRect(src, &wr);
		// sizetipを出す位置は、ウィンドウ左上(X, Y)に対して、
		// (X, Y - 文字列の高さ - FRAME_WIDTH * 2) とする。
		point.x = wr.left;
		point.y = wr.top - (h + FRAME_WIDTH * 2);
		FixPosFromFrame(&point, 16, FALSE);
		cx = point.x;
		cy = point.y;

		SizeTip = TipWinCreate(src, cx, cy, str);
	} else {
		/* Tip already exists, just set the text */
		TipWinSetText(SizeTip, str);
		//SetWindowText(tip_wnd, str);
	}
}

void EnableSizeTip(int bEnable)
{
	if (SizeTip && !bEnable) {
		TipWinDestroy(SizeTip);
		SizeTip = NULL;
	}

	tip_enabled = bEnable;
}
