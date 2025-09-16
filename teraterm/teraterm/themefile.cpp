/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

#include <string.h>
#include <stdio.h>

#include "tttypes.h"
#include "compat_win.h"
#include "asprintf.h"
#include "inifile_com.h"
#include "win32helper.h"
#include "codeconv.h"

#include "theme.h"

#define BG_SECTION_OLD L"BG"
#define BG_SECTION_NEW L"BG Theme"
#define COLOR_THEME_SECTION L"Color Theme"

#if !defined(offsetof)
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

/**
 *	ANSI256色時の先頭16色
 *	INIファイルから読み込むときのキーワードと色番号対応
 */
static const struct {
	int index;
	const wchar_t *key;
} ansi_list[] = {
	{ 7 + 8, L"Fore" },
	{ 0, L"Back" },
	{ 1 + 8, L"Red" },
	{ 2 + 8, L"Green" },
	{ 3 + 8, L"Yellow" },
	{ 4 + 8, L"Blue" },
	{ 5 + 8, L"Magenta" },
	{ 6 + 8, L"Cyan" },

	{ 7, L"DarkFore" },
	{ 0 + 8, L"DarkBack" },
	{ 1, L"DarkRed" },
	{ 2, L"DarkGreen" },
	{ 3, L"DarkYellow" },
	{ 4, L"DarkBlue" },
	{ 5, L"DarkMagenta" },
	{ 6, L"DarkCyan" },
};

/**
 *	INIファイルのキーワードとTColorTheme構造体のメンバーの対応表
 */
static const struct {
	const wchar_t *key;
	size_t offset;
} color_attr_list[] = {
	{ L"VTColor", offsetof(TColorTheme, vt) },
	{ L"BoldColor", offsetof(TColorTheme, bold) },
	{ L"BlinkColor", offsetof(TColorTheme, blink) },
	{ L"ReverseColor", offsetof(TColorTheme, reverse) },
	{ L"URLColor", offsetof(TColorTheme, url) },
	{ L"VTUnderlineColor", offsetof(TColorTheme, underline) },
};

const BG_PATTERN_ST *ThemeBGPatternList(int index)
{
	static const BG_PATTERN_ST bg_pattern_list[] = {
		{ BG_STRETCH, L"stretch" },
		{ BG_TILE, L"tile" },
		{ BG_CENTER, L"center" },
		{ BG_FIT_WIDTH, L"fit_width" },
		{ BG_FIT_HEIGHT, L"fit_height" },
		{ BG_AUTOFIT, L"autofit" },
		{ BG_AUTOFILL, L"autofill" },
	};

	if (index >= _countof(bg_pattern_list)) {
		return NULL;
	}
	return &bg_pattern_list[index];
}

static COLORREF LoadColorOneANSI(const wchar_t *section, const wchar_t *key, const wchar_t *file, COLORREF defcolor)
{
	int r;
	wchar_t *str;
	DWORD e = hGetPrivateProfileStringW(section, key, NULL, file, &str);
	if (e != 0 || *str == 0) {
		free(str);
		return defcolor;
	}
	if (*str == L'#') {
		// #RRGGBB 形式
		DWORD i32;
		r = swscanf_s(str, L"#%08x", &i32);
		if (r == 1) {
			free(str);
			return RGB((i32 & 0xff0000) >> 16, (i32 & 0x00ff00) >> 8, (i32 & 0x0000ff));
		}
	}
	// R, G, B 形式
	int red, green, blue;
	r = swscanf_s(str, L"%d , %d , %d", &red, &green, &blue);
	free(str);
	if (r == 3) {
		return RGB(red, green, blue);
	}
	return defcolor;
}

static COLORREF BGGetColor(const wchar_t *section, const wchar_t *key, COLORREF defcolor, const wchar_t *file)
{
	COLORREF color = LoadColorOneANSI(section, key, file, defcolor);
	return color;
}

/*
 *	color theme用ロード
 */
static void ThemeLoadColorOld(const wchar_t *file, TColorTheme *theme)
{
	const wchar_t *section = BG_SECTION_OLD;
	theme->ansicolor.change = TRUE;

	theme->ansicolor.color[IdFore] = BGGetColor(section, L"Fore", theme->ansicolor.color[IdFore], file);
	theme->ansicolor.color[IdBack] = BGGetColor(section, L"Back", theme->ansicolor.color[IdBack], file);
	theme->ansicolor.color[IdRed] = BGGetColor(section, L"Red", theme->ansicolor.color[IdRed], file);
	theme->ansicolor.color[IdGreen] = BGGetColor(section, L"Green", theme->ansicolor.color[IdGreen], file);
	theme->ansicolor.color[IdYellow] = BGGetColor(section, L"Yellow", theme->ansicolor.color[IdYellow], file);
	theme->ansicolor.color[IdBlue] = BGGetColor(section, L"Blue", theme->ansicolor.color[IdBlue], file);
	theme->ansicolor.color[IdMagenta] = BGGetColor(section, L"Magenta", theme->ansicolor.color[IdMagenta], file);
	theme->ansicolor.color[IdCyan] = BGGetColor(section, L"Cyan", theme->ansicolor.color[IdCyan], file);

	theme->ansicolor.color[IdFore + 8] = BGGetColor(section, L"DarkFore", theme->ansicolor.color[IdFore + 8], file);
	theme->ansicolor.color[IdBack + 8] = BGGetColor(section, L"DarkBack", theme->ansicolor.color[IdBack + 8], file);
	theme->ansicolor.color[IdRed + 8] = BGGetColor(section, L"DarkRed", theme->ansicolor.color[IdRed + 8], file);
	theme->ansicolor.color[IdGreen + 8] = BGGetColor(section, L"DarkGreen", theme->ansicolor.color[IdGreen + 8], file);
	theme->ansicolor.color[IdYellow + 8] = BGGetColor(section, L"DarkYellow", theme->ansicolor.color[IdYellow + 8], file);
	theme->ansicolor.color[IdBlue + 8] = BGGetColor(section, L"DarkBlue", theme->ansicolor.color[IdBlue + 8], file);
	theme->ansicolor.color[IdMagenta + 8] = BGGetColor(section, L"DarkMagenta", theme->ansicolor.color[IdMagenta + 8], file);
	theme->ansicolor.color[IdCyan + 8] = BGGetColor(section, L"DarkCyan", theme->ansicolor.color[IdCyan + 8], file);

	theme->vt.fg = BGGetColor(section, L"VTFore", theme->vt.fg, file);
	theme->vt.bg = BGGetColor(section, L"VTBack", theme->vt.bg, file);
	theme->vt.change = TRUE;
	theme->vt.enable = TRUE;

	theme->blink.fg = BGGetColor(section, L"VTBlinkFore", theme->blink.fg, file);
	theme->blink.bg = BGGetColor(section, L"VTBlinkBack", theme->blink.bg, file);
	theme->blink.change = TRUE;
	theme->blink.enable = TRUE;

	theme->bold.fg = BGGetColor(section, L"VTBoldFore", theme->bold.fg, file);
	theme->bold.bg = BGGetColor(section, L"VTBoldBack", theme->bold.bg, file);
	theme->bold.change = TRUE;
	theme->bold.enable = TRUE;

	theme->underline.fg = BGGetColor(section, L"VTUnderlineFore", theme->underline.fg, file);
	theme->underline.bg = BGGetColor(section, L"VTUnderlineBack", theme->underline.bg, file);
	theme->underline.change = TRUE;
	theme->underline.enable = TRUE;

	theme->reverse.fg = BGGetColor(section, L"VTReverseFore", theme->reverse.fg, file);
	theme->reverse.bg = BGGetColor(section, L"VTReverseBack", theme->reverse.bg, file);
	theme->reverse.change = TRUE;
	theme->reverse.enable = TRUE;

	theme->url.fg = BGGetColor(section, L"URLFore", theme->url.fg, file);
	theme->url.bg = BGGetColor(section, L"URLBack", theme->url.bg, file);
	theme->url.change = TRUE;
	theme->url.enable = TRUE;
}

/**
 *	save color one attribute
 */
static void BGSaveColorOne(const wchar_t *section, const wchar_t *key, const TColorSetting *color, const wchar_t *fn)
{
	wchar_t *buf;
	COLORREF fg = color->fg;
	COLORREF bg = color->bg;
	int sp_len = 20 - (int)wcslen(key);
	aswprintf(&buf, L"%*.*s %d,%d,  %3hhu,%3hhu,%3hhu,  %3hhu,%3hhu,%3hhu      ; #%02x%02x%02x, #%02x%02x%02x",
			  sp_len, sp_len, L"                    ",
			  color->change, color->enable,
			  GetRValue(fg), GetGValue(fg), GetBValue(fg),
			  GetRValue(bg), GetGValue(bg), GetBValue(bg),
			  GetRValue(fg), GetGValue(fg), GetBValue(fg),
			  GetRValue(bg), GetGValue(bg), GetBValue(bg));
	WritePrivateProfileStringW(section, key, buf, fn);
	free(buf);
}

#if 1
static void BGSaveColorANSI(const wchar_t *section, const TAnsiColorSetting *color, const wchar_t *fn)
{
	int i;
	wchar_t *buff = NULL;
	awcscat(&buff, L"1, 1, ");

	for (i = 0; i < 16; i++) {
		wchar_t color_str[32];
		const COLORREF c = color->color[i];
		swprintf(color_str, _countof(color_str), L"%hhu,%hhu,%hhu, ", GetRValue(c), GetGValue(c), GetBValue(c));
		awcscat(&buff, color_str);
	}

	WritePrivateProfileStringW(section, L"ANSIColor", buff, fn);
	free(buff);
}
#endif

static void SaveColorOneANSI(const wchar_t *section, const wchar_t *key, const wchar_t *file, COLORREF color, int index)
{
	const BYTE r = GetRValue(color);
	const BYTE g = GetGValue(color);
	const BYTE b = GetBValue(color);
	int sp_len = 20 - (int)wcslen(key);
	wchar_t *str;
	aswprintf(&str, L"%*.*s %3hhu, %3hhu, %3hhu  ; #%02hhx%02hhx%02hhx ; ANSIColor[%2d]",
			  sp_len, sp_len, L"                    ",
			  r, g, b, r, g, b, index);
	WritePrivateProfileStringW(section, key, str, file);
	free(str);
}

static void SaveColorANSINew(const wchar_t *section, const TAnsiColorSetting *color, const wchar_t *fname)
{
	wchar_t *str;
	aswprintf(&str, L"%d", color->change);
	WritePrivateProfileStringW(section, L"ANSIColor", str, fname);
	free(str);

	for (int i = 0; i < _countof(ansi_list); i++) {
		const int index = ansi_list[i].index;
		const wchar_t *key = ansi_list[i].key;
		SaveColorOneANSI(section, key, fname, color->color[index], index);
	}
}

/**
 *	カラーテーマの保存
 *		TODO 削除
 */
#if 1
void ThemeSaveColorOld(TColorTheme *color_theme, const wchar_t *fn)
{
	const wchar_t *section = COLOR_THEME_SECTION;

	WritePrivateProfileStringW(section, L"Theme", color_theme->name, fn);

	for (int i = 0; i < _countof(color_attr_list); i++) {
		const wchar_t *key = color_attr_list[i].key;
		const TColorSetting *color = (TColorSetting *)((UINT_PTR)color_theme + color_attr_list[i].offset);
		BGSaveColorOne(section, key, color, fn);
	}

	BGSaveColorANSI(section, &(color_theme->ansicolor), fn);
}
#endif

/**
 *	カラーテーマの保存
 */
void ThemeSaveColor(TColorTheme *color_theme, const wchar_t *fn)
{
	const wchar_t *section = COLOR_THEME_SECTION;
	WritePrivateProfileStringW(section, L"Theme", color_theme->name, fn);

	for (int i = 0; i < _countof(color_attr_list); i++) {
		const wchar_t *key = color_attr_list[i].key;
		const TColorSetting *color = (TColorSetting *)((UINT_PTR)color_theme + color_attr_list[i].offset);
		BGSaveColorOne(section, key, color, fn);
	}

	SaveColorANSINew(section, &(color_theme->ansicolor), fn);
}

static void WriteInt3(const wchar_t *Sect, const wchar_t *Key, const wchar_t *FName,
					  int i1, int i2, int i3)
{
	wchar_t Temp[96];
	_snwprintf_s(Temp, _countof(Temp), _TRUNCATE, L"%d,%d,%d",
	            i1, i2,i3);
	WritePrivateProfileStringW(Sect, Key, Temp, FName);
}

static void WriteCOLORREF(const wchar_t *Sect, const wchar_t *Key, const wchar_t *FName, COLORREF color)
{
	int red = color & 0xff;
	int green = (color >> 8) & 0xff;
	int blue = (color >> 16) & 0xff;

	WriteInt3(Sect, Key, FName, red, green, blue);
}

static const wchar_t *GetBGPatternStr(BG_PATTERN id)
{
	int index;
	for (index = 0;; index++) {
		const BG_PATTERN_ST *st = ThemeBGPatternList(index);
		if (st == NULL) {
			// 見つからない
			st = ThemeBGPatternList(0);
			return st->str;
		}
		if (st->id == id) {
			return st->str;
		}
	}
}

static BOOL GetBGPatternID(const wchar_t *str, BG_PATTERN *pattern)
{
	int index;
	for (index = 0;; index++) {
		const BG_PATTERN_ST *st = ThemeBGPatternList(index);
		if (st == NULL) {
			// 見つからない
			st = ThemeBGPatternList(0);
			*pattern = st->id;
			return FALSE;
		}
		if (_wcsicmp(st->str, str) == 0) {
			*pattern = st->id;
			return TRUE;
		}
	}
}

void ThemeSaveBG(const BGTheme *bg_theme, const wchar_t *file)
{
	const wchar_t *section = BG_SECTION_NEW;

	WritePrivateProfileIntW(section, L"BGDestEnable", bg_theme->BGDest.enable, file);
	WritePrivateProfileStringW(section, L"BGDestFile", bg_theme->BGDest.file, file);
#if 0
	WritePrivateProfileStringW(section, L"BGDestType",
									bg_theme->BGDest.type == BG_PICTURE ? "picture" : "color", file);
#endif
	WriteCOLORREF(section, L"BGDestColor", file, bg_theme->BGDest.color);
	WritePrivateProfileStringW(section, L"BGDestPattern", GetBGPatternStr(bg_theme->BGDest.pattern), file);
	WritePrivateProfileIntW(section, L"BGDestAlpha", bg_theme->BGDest.alpha, file);

	WritePrivateProfileIntW(section, L"BGSrc1Enable", bg_theme->BGSrc1.enable, file);
	WritePrivateProfileIntW(section, L"BGSrc1Alpha", bg_theme->BGSrc1.alpha, file);

	WritePrivateProfileIntW(section, L"BGSrc2Enable", bg_theme->BGSrc2.enable, file);
	WritePrivateProfileIntW(section, L"BGSrc2Alpha", bg_theme->BGSrc2.alpha, file);
	WriteCOLORREF(section, L"BGSrc2Color", file, bg_theme->BGSrc2.color);

	WritePrivateProfileIntW(section, L"BGReverseTextAlpha", bg_theme->BGReverseTextAlpha, file);
	WritePrivateProfileIntW(section, L"BGTextBackAlpha", bg_theme->TextBackAlpha, file);
	WritePrivateProfileIntW(section, L"BGBackAlpha", bg_theme->BackAlpha, file);
}

static int BGGetStrIndex(const wchar_t *section, const wchar_t *name, int def, const wchar_t *file, const wchar_t * const *strList, int nList)
{
	wchar_t defstr[64];
	wchar_t str[64];
	int i;

	def %= nList;

	wcsncpy_s(defstr, _countof(defstr), strList[def], _TRUNCATE);
	GetPrivateProfileStringW(section, name, defstr, str, _countof(str), file);

	for (i = 0; i < nList; i++)
		if (!_wcsicmp(str, strList[i]))
			return i;

	return 0;
}

static BOOL BGGetOnOff(const wchar_t *section, const wchar_t *name, BOOL def, const wchar_t *file)
{
	static const wchar_t * const strList[2] = {L"Off", L"On"};

	return (BOOL)BGGetStrIndex(section, name, def, file, strList, 2);
}

static BG_PATTERN BGGetPattern(const wchar_t *section, const wchar_t *name, BG_PATTERN def, const wchar_t *file)
{
	BG_PATTERN retval;
	wchar_t str[64];
	GetPrivateProfileStringW(section, name, L"", str, _countof(str), file);
	if (str[0] == 0) {
		return def;
	}
	if (GetBGPatternID(str, &retval) == FALSE) {
		retval = def;
	}
	return retval;
}

static BG_TYPE BGGetType(const wchar_t *section, const wchar_t *name, BG_TYPE def, const wchar_t *file)
{
	static const wchar_t *strList[3] = {L"color", L"picture", L"wallpaper"};

	return (BG_TYPE)BGGetStrIndex(section, name, def, file, strList, 3);
}

/**
 *	BGをロード
 */
void ThemeLoadBGSection(const wchar_t *section, const wchar_t *file, BGTheme *bg_theme)
{
	wchar_t pathW[MAX_PATH];
	wchar_t *p;

	// Dest の読み出し
	bg_theme->BGDest.enable = GetPrivateProfileIntW(section, L"BGDestEnable", 0, file);
	bg_theme->BGDest.type = BGGetType(section, L"BGDestType", bg_theme->BGDest.type, file);
	bg_theme->BGDest.pattern = BGGetPattern(section, L"BGPicturePattern", bg_theme->BGDest.pattern, file);
	bg_theme->BGDest.pattern = BGGetPattern(section, L"BGDestPattern", bg_theme->BGDest.pattern, file);
	bg_theme->BGDest.antiAlias = BGGetOnOff(section, L"BGDestAntiAlias", bg_theme->BGDest.antiAlias, file);
	bg_theme->BGDest.alpha = GetPrivateProfileIntW(section, L"BGDestAlpha", bg_theme->BGDest.alpha, file);
	bg_theme->BGDest.color = BGGetColor(section, L"BGPictureBaseColor", bg_theme->BGDest.color, file);
	bg_theme->BGDest.color = BGGetColor(section, L"BGDestColor", bg_theme->BGDest.color, file);
	GetPrivateProfileStringW(section, L"BGPictureFile", bg_theme->BGDest.file, pathW, _countof(pathW), file);
	GetPrivateProfileStringW(section, L"BGDestFile", pathW, pathW, _countof(pathW), file);
	p = RandomFileW(pathW);
	if (p != NULL) {
		wcscpy_s(bg_theme->BGDest.file, _countof(bg_theme->BGDest.file), p);
		free(p);
	}
	else {
		bg_theme->BGDest.file[0] = 0;
	}

	// Src1 の読み出し
	bg_theme->BGSrc1.enable = GetPrivateProfileIntW(section, L"BGSrc1Enable", 0, file);
	bg_theme->BGSrc1.type = BGGetType(section, L"BGSrc1Type", bg_theme->BGSrc1.type, file);
	bg_theme->BGSrc1.pattern = BGGetPattern(section, L"BGSrc1Pattern", bg_theme->BGSrc1.pattern, file);
	bg_theme->BGSrc1.antiAlias = BGGetOnOff(section, L"BGSrc1AntiAlias", bg_theme->BGSrc1.antiAlias, file);
	bg_theme->BGSrc1.alpha = 255 - GetPrivateProfileIntW(section, L"BGPictureTone", 255 - bg_theme->BGSrc1.alpha, file);
	bg_theme->BGSrc1.alpha = GetPrivateProfileIntW(section, L"BGSrc1Alpha", bg_theme->BGSrc1.alpha, file);
	bg_theme->BGSrc1.color = BGGetColor(section, L"BGSrc1Color", bg_theme->BGSrc1.color, file);
	GetPrivateProfileStringW(section, L"BGSrc1File", bg_theme->BGSrc1.file, pathW, _countof(pathW), file);
	p = RandomFileW(pathW);
	if (p != NULL) {
		wcscpy_s(bg_theme->BGSrc1.file, _countof(bg_theme->BGSrc1.file), p);
		free(p);
	}
	else {
		bg_theme->BGSrc1.file[0] = 0;
	}

	// Src2 の読み出し
	bg_theme->BGSrc2.enable = GetPrivateProfileIntW(section, L"BGSrc2Enable", 0, file);
	bg_theme->BGSrc2.type = BGGetType(section, L"BGSrc2Type", bg_theme->BGSrc2.type, file);
	bg_theme->BGSrc2.pattern = BGGetPattern(section, L"BGSrc2Pattern", bg_theme->BGSrc2.pattern, file);
	bg_theme->BGSrc2.antiAlias = BGGetOnOff(section, L"BGSrc2AntiAlias", bg_theme->BGSrc2.antiAlias, file);
	bg_theme->BGSrc2.alpha = 255 - GetPrivateProfileIntW(section, L"BGFadeTone", 255 - bg_theme->BGSrc2.alpha, file);
	bg_theme->BGSrc2.alpha = GetPrivateProfileIntW(section, L"BGSrc2Alpha", bg_theme->BGSrc2.alpha, file);
	bg_theme->BGSrc2.color = BGGetColor(section, L"BGFadeColor", bg_theme->BGSrc2.color, file);
	bg_theme->BGSrc2.color = BGGetColor(section, L"BGSrc2Color", bg_theme->BGSrc2.color, file);
	GetPrivateProfileStringW(section, L"BGSrc2File", bg_theme->BGSrc2.file, pathW, _countof(pathW), file);
	p = RandomFileW(pathW);
	if (p != NULL) {
		wcscpy_s(bg_theme->BGSrc2.file, _countof(bg_theme->BGSrc2.file), p);
		free(p);
	}
	else {
		bg_theme->BGSrc2.file[0] = 0;
	}

	//その他読み出し
	bg_theme->BGReverseTextAlpha = GetPrivateProfileIntW(section, L"BGReverseTextTone", bg_theme->BGReverseTextAlpha, file);
	bg_theme->BGReverseTextAlpha = GetPrivateProfileIntW(section, L"BGReverseTextAlpha", bg_theme->BGReverseTextAlpha, file);
	bg_theme->TextBackAlpha = GetPrivateProfileIntW(section, L"BGTextBackAlpha", bg_theme->TextBackAlpha, file);
	bg_theme->BackAlpha = GetPrivateProfileIntW(section, L"BGBackAlpha", bg_theme->BackAlpha, file);
}

/**
 *	BGをロード
 */
void ThemeLoadBG(const wchar_t *file, BGTheme *bg_theme)
{
	ThemeLoadBGSection(BG_SECTION_OLD, file, bg_theme);
	ThemeLoadBGSection(BG_SECTION_NEW, file, bg_theme);
}

static void ReadANSIColorSetting(const wchar_t *section, TAnsiColorSetting *color, const wchar_t *fn)
{
	wchar_t BuffW[512];
	char Buff[512];
	int c, r, g, b;
	// ANSIColor16は、明るい/暗いグループが入れ替わっている
	const static int index256[] = {
		0,
		1, 2, 3, 4, 5, 6, 7,
		8,
		9, 10, 11, 12, 13, 14, 15,
	};

	GetPrivateProfileStringW(section, L"ANSIColor", L"0", BuffW, _countof(BuffW), fn);
	WideCharToACP_t(BuffW, Buff, _countof(Buff));

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	//color->enable = c;

	for (c=0; c<16; c++) {
		int idx = index256[c];
		GetNthNum(Buff, c * 3 + 3, &r);
		GetNthNum(Buff, c * 3 + 4, &g);
		GetNthNum(Buff, c * 3 + 5, &b);
		color->color[idx] = RGB(r, g, b);
	}
}

static void ReadColorSetting(const wchar_t *section, TColorSetting *color, const wchar_t *key, const wchar_t *fn)
{
	wchar_t BuffW[512];
	char Buff[512];
	int c, r, g, b;

	GetPrivateProfileStringW(section, key, L"0", BuffW, _countof(BuffW), fn);
	WideCharToACP_t(BuffW, Buff, _countof(Buff));

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	color->enable = c;

	if (color->change && color->enable) {
		GetNthNum(Buff, 3, &r);
		GetNthNum(Buff, 4, &g);
		GetNthNum(Buff, 5, &b);
		color->fg = RGB(r, g, b);

		GetNthNum(Buff, 6, &r);
		GetNthNum(Buff, 7, &g);
		GetNthNum(Buff, 8, &b);
		color->bg = RGB(r, g, b);
	}
}

/**
 *	カラーテーマプラグイン版 ini ファイル読み込み
 */
static void LoadColorPlugin(const wchar_t *fn, TColorTheme *color_theme)
{
	const wchar_t *section = COLOR_THEME_SECTION;
	wchar_t *name;
	hGetPrivateProfileStringW(section, L"Theme", NULL, fn, &name);
	wcscpy_s(color_theme->name, _countof(color_theme->name), name);
	free(name);

	for (int i = 0; i < _countof(color_attr_list); i++) {
		const wchar_t *key = color_attr_list[i].key;
		TColorSetting *color = (TColorSetting *)((UINT_PTR)color_theme + color_attr_list[i].offset);
		ReadColorSetting(section, color, key, fn);
	}

	ReadANSIColorSetting(section, &(color_theme->ansicolor), fn);
}

/**
 *	カラーテーマファイル読み込み,1アトリビュート分
 */
static void LoadColorAttr(const wchar_t *section, const wchar_t *key, const wchar_t *file, TColorSetting *attr)
{
	wchar_t *str;
	DWORD e = hGetPrivateProfileStringW(section, key, NULL, file, &str);
	if (e != 0 || *str == 0) {
		free(str);
		return;
	}

	BOOL change = FALSE;
	BOOL enable = FALSE;
	int fields;

	DWORD fore_rgb;
	DWORD back_rgb;
	fields = swscanf_s(str, L"%d, %d, #%06x, #%06x", &change, &enable, &fore_rgb, &back_rgb);
	if (fields == 4) {
		free(str);
		attr->change = change;
		attr->enable = enable;
		attr->fg = RGB((fore_rgb & 0xff0000) >> 16, (fore_rgb & 0x00ff00) >> 8, (fore_rgb & 0x0000ff));
		attr->bg = RGB((back_rgb & 0xff0000) >> 16, (back_rgb & 0x00ff00) >> 8, (back_rgb & 0x0000ff));
		return;
	}

	unsigned int fg_red, fg_green, fg_blue;
	unsigned int bg_red, bg_green, bg_blue;
	fields = swscanf_s(str, L"%d, %d, %u, %u, %u, %u, %u, %u", &change, &enable, &fg_red, &fg_green,
					   &fg_blue, &bg_red, &bg_green, &bg_blue);
	if (fields == 8) {
		free(str);
		attr->change = change;
		attr->enable = enable;
		attr->fg = RGB(fg_red, fg_green, fg_blue);
		attr->bg = RGB(bg_red, bg_green, bg_blue);
		return;
	}
	fields = swscanf_s(str, L"%d, %d", &change, &enable);
	if (fields == 2) {
		free(str);
		attr->change = change;
		attr->enable = FALSE;	 // 色指定が読めなかったので、文字属性の独自色は無効
		return;
	}
	fields = swscanf_s(str, L"%d", &change);
	free(str);
	if (fields == 1) {
		attr->change = FALSE;	 // 色変更はしない
		return;
	}
}

/*
 *	color theme用ロード
 */
static void ThemeLoadColorDraft(const wchar_t *file, TColorTheme *theme)
{
	const wchar_t *section = COLOR_THEME_SECTION;

	wchar_t *name;
	hGetPrivateProfileStringW(section, L"Theme", NULL, file, &name);
	wcscpy_s(theme->name, _countof(theme->name), name);
	free(name);

	for (int i = 0; i < _countof(color_attr_list); i++) {
		const wchar_t *key = color_attr_list[i].key;
		TColorSetting *color = (TColorSetting *)((UINT_PTR)theme + color_attr_list[i].offset);
		LoadColorAttr(section, key, file, color);
	}

	theme->ansicolor.change = (BOOL)GetPrivateProfileIntW(section, L"ANSIColor", 1, file);
	for (int i = 0; i < _countof(ansi_list); i++) {
		const int index = ansi_list[i].index;
		const wchar_t *key = ansi_list[i].key;
		theme->ansicolor.color[index] = LoadColorOneANSI(section, key, file, theme->ansicolor.color[index]);
	}
}

/*
 *	カラーテーマiniファイルをロードする
 */
void ThemeLoadColor(const wchar_t *fn, TColorTheme *color_theme)
{
	ThemeGetColorDefault(color_theme);
	LoadColorPlugin(fn, color_theme);
	ThemeLoadColorOld(fn, color_theme);
	ThemeLoadColorDraft(fn, color_theme);
}

/**
 *	テーマファイルを読み込む
 *
 * @param file				ファイル名
 *							NULLの時は構造体にデフォルト値が設定される
 * @param bg_theme
 * @param color_theme
*/
void ThemeLoad(const wchar_t *file, BGTheme *bg_theme, TColorTheme *color_theme)
{
	BOOL bg = FALSE;
	BOOL color = FALSE;
	wchar_t *prevDir;

	// カレントディレクトリを保存
	hGetCurrentDirectoryW(&prevDir);

	// テーマファイルのあるディレクトリに一時的に移動
	//		テーマファイル相対パスで読み込めるよう
	if (file != NULL) {
		wchar_t *dir = ExtractDirNameW(file);
		SetCurrentDirectoryW(dir);
		free(dir);
	}

	{
		wchar_t sections[128];
		size_t i;
		GetPrivateProfileSectionNamesW(sections, _countof(sections), file);
		for(i = 0; i < _countof(sections); /**/ ) {
			const wchar_t *p = &sections[i];
			size_t len = wcslen(p);
			if (len == 0) {
				break;
			}
			if (_wcsicmp(p, BG_SECTION_NEW) == 0 || _wcsicmp(p, BG_SECTION_OLD) == 0) {
				bg = TRUE;
			}
			else if(_wcsicmp(p, COLOR_THEME_SECTION) == 0) {
				color = TRUE;
			}
			i += len + 1;
		}
	}

	ThemeGetBGDefault(bg_theme);
	ThemeGetColorDefault(color_theme);

	// BG + カラーテーマ iniファイル
	if (bg && color) {
		ThemeLoadBG(file, bg_theme);
		ThemeLoadColor(file, color_theme);
	}
	// BGテーマ iniファイル
	// TODO この時カラーは読み込まないようにしたい
	else if (bg) {
		ThemeLoadBG(file, bg_theme);
		ThemeLoadColorOld(file, color_theme);
	}
	// カラーテーマ iniファイル
	else if (color) {
		ThemeLoadColor(file, color_theme);
	}
	else {
#if 0
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_TT_ERROR", L"Tera Term: ERROR",
			NULL, L"unknown ini file?",
			MB_OK|MB_ICONEXCLAMATION
		};
		TTMessageBoxW(HVTWin, &info, ts.UILanguageFileW);
#endif
	}

	// カレントフォルダを元に戻す
	SetCurrentDirectoryW(prevDir);
	free(prevDir);
}
