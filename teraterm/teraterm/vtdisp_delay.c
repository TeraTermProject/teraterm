/* Tera Term 高速化テスト */

#include <windows.h>
#include <time.h>
#include <stdio.h>

#include "tttypes.h"
#include "ttwinman.h"
#include "ttlib.h"
#include "vtdisp.h"
#include "vtdisp_delay.h"

// buffer.c
extern int StrChangeStart, StrChangeCount;
extern int dScroll;

#define DISPLAY_INTERVAL	(1000 / 30)		// (ms)
//#define DISPLAY_INTERVAL	(1000)	// (ms)

#define OutputDebugPrintf(...)

void BuffUpdateRect2(int XStart, int YStart, int XEnd, int YEnd);

////////////////////////////////////////

typedef struct {
	enum {
		NONE,				// 描画なし
		ONE_LINE,			// 1行描画
		WHOLE_TERM,			// 全画面描画
	} Type;

	// ONE_LINE時のデータ
	int StrChangeStart;		// cursorのX
	int CursorY;			// cursorのY
	int StrChangeCount;		// 文字数

	// 表示開始位置
	int NewOrgX;
	int NewOrgY;

	DWORD UpdateTick;

	// scroll bar情報
	int ScrollBarVPos;
} UpdateInfo_t;

static UpdateInfo_t UpdateInfo;

void UpdateStr()
{
	if (UpdateInfo.Type == NONE) {
		if (StrChangeCount != 0) {
			UpdateInfo.Type = ONE_LINE;
			UpdateInfo.CursorY = CursorY;
			UpdateInfo.StrChangeStart = StrChangeStart;
			UpdateInfo.StrChangeCount = StrChangeCount;
		}
	} else if (UpdateInfo.Type == ONE_LINE) {
		if (StrChangeCount == 0) {
			;
		} else if (UpdateInfo.CursorY != CursorY) {
			UpdateInfo.Type = WHOLE_TERM;
		} else if (UpdateInfo.StrChangeStart + UpdateInfo.StrChangeCount == StrChangeStart ) {
			UpdateInfo.StrChangeCount += StrChangeCount;
		} else {
			UpdateInfo.StrChangeStart = 0;
			UpdateInfo.StrChangeCount = NumOfColumns;
		}
	}

	OutputDebugPrintf("%d (%d,%d)%d , (%d,%d)%d\n",
					  UpdateInfo.Type,
					  StrChangeStart, CursorY, StrChangeCount,
					  UpdateInfo.StrChangeStart, UpdateInfo.CursorY, UpdateInfo.StrChangeCount);

	StrChangeCount = 0;
}

void BuffUpdateRect(int XStart, int YStart, int XEnd, int YEnd)
{
	OutputDebugPrintf("BuffUpdateRect(%d,%d,%d,%d)\n",
					  XStart, YStart, XEnd, YEnd);
	UpdateInfo.Type = WHOLE_TERM;
}

void DispUpdateScroll()
{
	OutputDebugPrintf("DispUpdateScroll\n");
	UpdateInfo.Type = WHOLE_TERM;
	UpdateInfo.NewOrgX += NewOrgX;
	UpdateInfo.NewOrgY += NewOrgY;
	dScroll = 0;
}

void UpdateTerm()
{
	DWORD now = timeGetTime();
	int NewOrgX;
	int NewOrgY;
	int ScrollBarUpdated = 0;
	OutputDebugPrintf("UpdateTerm %d\n", UpdateInfo.Type);

	if (UpdateInfo.Type == NONE) {
		return;
	}

	NewOrgX = UpdateInfo.NewOrgX;
	NewOrgY = UpdateInfo.NewOrgY;

	/* Update normal scroll */
	if (NewOrgX < 0) NewOrgX = 0;
	if (NewOrgX > NumOfColumns - WinWidth)
		NewOrgX = NumOfColumns - WinWidth;
	if (NewOrgY < -PageStart)
		NewOrgY = -PageStart;
	if (NewOrgY > BuffEnd - WinHeight - PageStart)
		NewOrgY = BuffEnd - WinHeight - PageStart;

	if (NewOrgX != WinOrgX) {
		SetScrollPos(HVTWin, SB_HORZ, NewOrgX, TRUE);
		ScrollBarUpdated = 1;
	}

	if (((ts.AutoScrollOnlyInBottomLine != 0 || NewOrgY!=WinOrgY)) &&
		(UpdateInfo.ScrollBarVPos != NewOrgY + PageStart))
	{
		SCROLLINFO scroll_info;
		scroll_info.cbSize = sizeof(scroll_info);
		scroll_info.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
		scroll_info.nPage = WinHeight;
		scroll_info.nMin = 0;
		scroll_info.nMax = BuffEnd-WinHeight;

		if ((BuffEnd==WinHeight) && (ts.EnableScrollBuff>0)) {
			scroll_info.nMax = BuffEnd;
			scroll_info.nPos = 0;
		} else {
			int max = BuffEnd - WinHeight;
			scroll_info.nMax = BuffEnd;
			scroll_info.nPos = NewOrgY+PageStart;
		}

		SetScrollInfo(HVTWin, SB_VERT, &scroll_info, TRUE);
		UpdateInfo.ScrollBarVPos = scroll_info.nPos;
		ScrollBarUpdated = 1;
		OutputDebugPrintf("sb_vert %d-%d-%d,%d\n",
						  scroll_info.nMin,
						  scroll_info.nPos,
						  scroll_info.nMax,
						  scroll_info.nPage);
	}

#if 0
	if (ScrollBarUpdated) {
		// スクロールバーが更新された場合はすぐに描画する
		InvalidateRect(HVTWin, NULL, FALSE);
	} else
#endif
	if (UpdateInfo.Type == WHOLE_TERM) {
		if ((now - UpdateInfo.UpdateTick) < DISPLAY_INTERVAL) {
			return;
		}
		InvalidateRect(HVTWin, NULL, FALSE);
	} else if (UpdateInfo.Type == ONE_LINE) {
		BuffUpdateRect2(UpdateInfo.StrChangeStart, UpdateInfo.CursorY,
						UpdateInfo.StrChangeStart + UpdateInfo.StrChangeCount, UpdateInfo.CursorY);
	}

	WinOrgX = NewOrgX;
	WinOrgY = NewOrgY;
	UpdateInfo.UpdateTick = now;
	UpdateInfo.Type = NONE;

}

int IsUpdateTerm()
{
	if (UpdateInfo.Type == NONE) {
		return 0;
	}
	return 1;
}
