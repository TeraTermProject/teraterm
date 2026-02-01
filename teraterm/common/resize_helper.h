/*
 * Copyright (C) 2023- TeraTerm Project
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

/**
 *	ダイアログリサイズ時に登録したダイアログ上のコントロールを再配置する
 *	ウィンドウのどの部分を基準(アンカー)にするかを指定する
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReiseDlgHelper_st ReiseDlgHelper_t;

/**
 *	アンカー定数
 */
typedef enum {
	RESIZE_HELPER_ANCHOR_NONE_H = 0x01,	// 水平方向アンカーなし
	RESIZE_HELPER_ANCHOR_NONE_V = 0x02,	// 垂直方向アンカーなし
	RESIZE_HELPER_ANCHOR_NONE = RESIZE_HELPER_ANCHOR_NONE_H | RESIZE_HELPER_ANCHOR_NONE_V,
	RESIZE_HELPER_ANCHOR_LEFT = 0x04,	// ウィンドウ左端(未指定と同じ)
	RESIZE_HELPER_ANCHOR_RIGHT = 0x08,	// ウィンドウ右端
	RESIZE_HELPER_ANCHOR_TOP = 0x10,	// ウィンドウ上端(未指定と同じ)
	RESIZE_HELPER_ANCHOR_BOTTOM = 0x20,	// ウィンドウ下端
	RESIZE_HELPER_ANCHOR_L = RESIZE_HELPER_ANCHOR_LEFT,
	RESIZE_HELPER_ANCHOR_R = RESIZE_HELPER_ANCHOR_RIGHT,
	RESIZE_HELPER_ANCHOR_T = RESIZE_HELPER_ANCHOR_TOP,
	RESIZE_HELPER_ANCHOR_B = RESIZE_HELPER_ANCHOR_BOTTOM,
	RESIZE_HELPER_ANCHOR_LR = RESIZE_HELPER_ANCHOR_LEFT | RESIZE_HELPER_ANCHOR_RIGHT,
	RESIZE_HELPER_ANCHOR_TB = RESIZE_HELPER_ANCHOR_T | RESIZE_HELPER_ANCHOR_B,
	RESIZE_HELPER_ANCHOR_RT = RESIZE_HELPER_ANCHOR_R | RESIZE_HELPER_ANCHOR_T,
	RESIZE_HELPER_ANCHOR_RB = RESIZE_HELPER_ANCHOR_R | RESIZE_HELPER_ANCHOR_B,
	RESIZE_HELPER_ANCHOR_LRT = RESIZE_HELPER_ANCHOR_LR | RESIZE_HELPER_ANCHOR_T,
	RESIZE_HELPER_ANCHOR_LRB = RESIZE_HELPER_ANCHOR_LR | RESIZE_HELPER_ANCHOR_B,
	RESIZE_HELPER_ANCHOR_LRTB = RESIZE_HELPER_ANCHOR_LRT | RESIZE_HELPER_ANCHOR_B,
} ResizeHelperAnchor;

typedef struct {
	const int id;
	const ResizeHelperAnchor anchor;
} ResizeHelperInfo;

/**
 *	初期化
 *
 *	@param[in]	dlg			ダイアログハンドル
 *	@param[in]	size_box	TRUE/FALSE	サイズボックスを表示する/しない
 */
ReiseDlgHelper_t *ReiseDlgHelperCreate(HWND dlg, BOOL size_box);

/**
 *	終了
 */
void ReiseDlgHelperDelete(ReiseDlgHelper_t *h);

/**
 *	コントロールをウィンドウのどこにアンカーするか指定
 */
void ReiseDlgHelperAdd(ReiseDlgHelper_t *h, UINT id, ResizeHelperAnchor anchor);

/**
 *	初期化 + 連続登録
 *
 *	@param[in]	dlg			ダイアログハンドル
 *	@param[in]	size_box	TRUE/FALSE	サイズボックスを表示する/しない
 *	@param[in]	infos		リサイズ情報リスト
 *	@param[in]	info_count	リサイズ情報数
 */
ReiseDlgHelper_t *ReiseHelperInit(HWND dlg, BOOL size_box, const ResizeHelperInfo *infos, size_t info_count);

/**
 *	ウィンドウのメッセージが発生したら呼び出し
 */
void ReiseDlgHelper_WM_SIZE(ReiseDlgHelper_t *h);
INT_PTR ReiseDlgHelper_WM_GETMINMAXINFO(ReiseDlgHelper_t *h, LPARAM lp);

#ifdef __cplusplus
}
#endif
