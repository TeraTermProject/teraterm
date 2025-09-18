/*
 * Copyright (C) 2008- TeraTerm Project
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
#include "compat_win.h"

#include <windows.h>
#include <stdio.h>

#include "tipwin.h"

static TipWin *SizeTip;
static int tip_enabled = 0;

/*
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

		hm = pMonitorFromPoint(*point, MONITOR_DEFAULTTONULL);
		if (hm == NULL) {
			if (NearestMonitor) {
				// 最も近いモニタに表示する
				hm = pMonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			} else {
				// スクリーンからはみ出している場合はマウスのあるモニタに表示する
				GetCursorPos(point);
				hm = pMonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			}
		}

		mi.cbSize = sizeof(MONITORINFO);
		pGetMonitorInfoA(hm, &mi);
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

/* リサイズ用ツールチップを表示する
 *
 * 引数：
 *   src        ウィンドウハンドル
 *   cx, cy     ツールチップに表示する縦横サイズ
 *   fwSide     リサイズ時にどこのウィンドウを掴んだか
 *   newX, newY リサイズ後の左上の座標
 *
 * 注意:
 * Windows9xでは画面のプロパティで「ウィンドウの内容を表示したまま
 * ドラッグする」にチェックを入れないと、ツールチップが表示されません。
 */
void UpdateSizeTip(HWND src, int cx, int cy, UINT fwSide, int newX, int newY)
{
	char str[32];
	int tooltip_movable = 0;

	if (!tip_enabled)
		return;

	/* Generate the tip text */
	_snprintf_s(str, _countof(str), _TRUNCATE, "%dx%d", cx, cy);

	// ウィンドウの右、右下、下を掴んだ場合は、ツールチップを左上隅に配置する。
	// それら以外はリサイズ後の左上隅に配置する。
	if (!(fwSide == WMSZ_RIGHT || fwSide == WMSZ_BOTTOMRIGHT || fwSide == WMSZ_BOTTOM)) {
		tooltip_movable = 1;
	}

	if (SizeTip == NULL) {
		RECT wr;
		POINT point;
		int w, h;
		const int x_offset = 16;

		// ウィンドウの位置を取得
		GetWindowRect(src, &wr);

		// sizetipを出す位置は、ウィンドウ左上(X, Y)に対して、
		// (X, Y - sizetipの高さ) とする。
		SizeTip = TipWinCreate(NULL, src);
		TipWinSetTextA(SizeTip, str);
		TipWinGetWindowSize(SizeTip, &w, &h);
		point.x = wr.left;
		point.y = wr.top - h;
		FixPosFromFrame(&point, x_offset, FALSE);
		cx = point.x;
		cy = point.y;
		TipWinSetPos(SizeTip, cx, cy);
		TipWinSetVisible(SizeTip, 1);
	}
	else {
		/* Tip already exists, just set the text */
		TipWinSetTextA(SizeTip, str);

		// ウィンドウの左上が移動する場合
		if (tooltip_movable) {
			int frame_width;
			TipWinGetFrameSize(SizeTip, &frame_width);
			TipWinSetPos(SizeTip, newX + frame_width * 2, newY + frame_width * 2);
		}
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
