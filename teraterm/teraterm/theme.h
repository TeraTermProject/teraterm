/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2022- TeraTerm Project
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

#pragma once

#include <windows.h>

#include "vtdisp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _BG_TYPE { BG_COLOR = 0, BG_PICTURE, BG_WALLPAPER, BG_NONE } BG_TYPE;
typedef enum _BG_PATTERN {
	BG_STRETCH = 0,
	BG_TILE,
	BG_CENTER,
	BG_FIT_WIDTH,
	BG_FIT_HEIGHT,
	BG_AUTOFIT,
	BG_AUTOFILL
} BG_PATTERN;

typedef struct {
	BG_TYPE type;
	BG_PATTERN pattern;
	BOOL enable;
	BOOL antiAlias;
	COLORREF color;		// type=BG_PICTURE, BG_WALLPAPER時、画像のない部分塗りつぶし色
	int alpha;
	wchar_t file[MAX_PATH];
} TBGSrc;

typedef struct _BGTheme {
	TBGSrc BGDest;				// 背景画像
	TBGSrc BGSrc1;				// 壁紙(Windowsのデスクトップ背景)
	TBGSrc BGSrc2;				// fill color
	BYTE TextBackAlpha;			// 通常属性(SGR0),back部分のAlpha
	BYTE BGReverseTextAlpha;	// 反転属性(SGR7),back部分のAlpha
	BYTE BackAlpha;				// その他のback部分のAlpha
} BGTheme;

////////////////////
// color theme
////////////////////

// Character Attributes
//	Normal(SGR 0), BOLD(SGR 1),...
typedef struct {
	BOOL change;		// TRUE/FALSE = default色から変更する/変更しない
	BOOL enable;		// TRUE/FALSE = この文字属性(Bold attribute等)の独自色を有効/無効にする
	COLORREF fg;		// Fore color (文字属性の独自色が有効な場合)
	COLORREF bg;		// Back color (文字属性の独自色が有効な場合)
} TColorSetting;

// ANSI Color
typedef struct {
	BOOL change;			// TRUE/FALSE =  default色から変更する/変更しない
	//BOOL enable;			// 不要か?
	COLORREF color[16];		// ANSI color 256色 の最初の16色, (前半が暗い色,後半が原色の明るい色)
} TAnsiColorSetting;

// color theme
typedef struct {
	wchar_t name[50];
	TColorSetting vt;			// SGR 0
	TColorSetting bold;			// SGR 1
	TColorSetting underline;	// SGR 4
	TColorSetting blink;		// SGR 5
	TColorSetting reverse;		// SGR 7
	TColorSetting url;
	TAnsiColorSetting ansicolor;
} TColorTheme;

typedef struct {
	BG_PATTERN id;
	const wchar_t *str;
} BG_PATTERN_ST;

// setting / themefile.cpp
void ThemeEnable(BOOL enable);
BOOL ThemeIsEnabled(void);
const BG_PATTERN_ST *ThemeBGPatternList(int index);

// file / themefile.cpp
void ThemeLoadBG(const wchar_t *file, BGTheme *bg_theme);
void ThemeLoadColor(const wchar_t *fname, TColorTheme *color_theme);
void ThemeLoad(const wchar_t *fname, BGTheme *bg_theme, TColorTheme *color_theme);
void ThemeSaveBG(const BGTheme *bg_theme, const wchar_t *fname);
void ThemeSaveColor(TColorTheme *color_theme, const wchar_t *fname);

// setting / vtdisp.c
void ThemeGetBG(vtdraw_t *vt, BGTheme *bg_theme);
void ThemeSetBG(vtdraw_t *vt, const BGTheme *bg_theme);
void ThemeGetColor(vtdraw_t *vt, TColorTheme *data);
void ThemeSetColor(vtdraw_t *vt, const TColorTheme *data);
void ThemeGetColorDefault(TColorTheme *data);
void ThemeGetColorDefaultTS(const TTTSet *pts, TColorTheme *color_theme);
void ThemeGetBGDefault(BGTheme *bg_theme);

#ifdef __cplusplus
}
#endif
