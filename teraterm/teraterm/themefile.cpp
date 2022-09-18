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

#include "theme.h"

// Eterm look-feel
#define BG_SECTION "BG"
#define BG_SECTIONW L"BG"
#define BG_DESTFILE "BGDestFile"
#define BG_DESTFILEW L"BGDestFile"
//#define BG_THEME_IMAGEFILE "theme\\ImageFile.INI"
//#define BG_THEME_IMAGEFILE_DEFAULT "theme\\*.INI"
#define BG_THEME_IMAGE_BRIGHTNESS_DEFAULT 64
#define BG_THEME_IMAGE_BRIGHTNESS1 "BGSrc1Alpha"
#define BG_THEME_IMAGE_BRIGHTNESS1W L"BGSrc1Alpha"
#define BG_THEME_IMAGE_BRIGHTNESS2 "BGSrc2Alpha"
//#define BG_THEME_IMAGEFILE_NAME "ImageFile.INI"
//#define BG_THEME_IMAGEFILE_NAMEW L"ImageFile.INI"
//#define BG_THEME_THEMEFILE_SCALE "Scale.INI"
//#define BG_THEME_THEMEFILE_TILE "Tile.INI"

const BG_PATTERN_ST *ThemeBGPatternList(int index)
{
	static const BG_PATTERN_ST bg_pattern_list[] = {
		{ BG_STRETCH, "stretch" },
		{ BG_TILE, "tile" },
		{ BG_CENTER, "center" },
		{ BG_FIT_WIDTH, "fit_width" },
		{ BG_FIT_HEIGHT, "fit_height" },
		{ BG_AUTOFIT, "autofit" },
		{ BG_AUTOFILL, "autofill" },
	};

	if (index >= _countof(bg_pattern_list)) {
		return NULL;
	}
	return &bg_pattern_list[index];
}

static COLORREF BGGetColor(const char *name, COLORREF defcolor, const wchar_t *file)
{
	unsigned int r, g, b;
	char colorstr[256], defstr[256];

	_snprintf_s(defstr, sizeof(defstr), _TRUNCATE, "%d,%d,%d", GetRValue(defcolor), GetGValue(defcolor),
				GetBValue(defcolor));

	GetPrivateProfileStringAFileW(BG_SECTION, name, defstr, colorstr, 255, file);

	r = g = b = 0;

	sscanf(colorstr, "%d , %d , %d", &r, &g, &b);

	return RGB(r, g, b);
}

/*
 *	color theme用ロード
 */
void ThemeLoadColorOld(const wchar_t *file, TColorTheme *theme)
{
	theme->ansicolor.color[IdFore] = BGGetColor("Fore", theme->ansicolor.color[IdFore], file);
	theme->ansicolor.color[IdBack] = BGGetColor("Back", theme->ansicolor.color[IdBack], file);
	theme->ansicolor.color[IdRed] = BGGetColor("Red", theme->ansicolor.color[IdRed], file);
	theme->ansicolor.color[IdGreen] = BGGetColor("Green", theme->ansicolor.color[IdGreen], file);
	theme->ansicolor.color[IdYellow] = BGGetColor("Yellow", theme->ansicolor.color[IdYellow], file);
	theme->ansicolor.color[IdBlue] = BGGetColor("Blue", theme->ansicolor.color[IdBlue], file);
	theme->ansicolor.color[IdMagenta] = BGGetColor("Magenta", theme->ansicolor.color[IdMagenta], file);
	theme->ansicolor.color[IdCyan] = BGGetColor("Cyan", theme->ansicolor.color[IdCyan], file);

	theme->ansicolor.color[IdFore + 8] = BGGetColor("DarkFore", theme->ansicolor.color[IdFore + 8], file);
	theme->ansicolor.color[IdBack + 8] = BGGetColor("DarkBack", theme->ansicolor.color[IdBack + 8], file);
	theme->ansicolor.color[IdRed + 8] = BGGetColor("DarkRed", theme->ansicolor.color[IdRed + 8], file);
	theme->ansicolor.color[IdGreen + 8] = BGGetColor("DarkGreen", theme->ansicolor.color[IdGreen + 8], file);
	theme->ansicolor.color[IdYellow + 8] = BGGetColor("DarkYellow", theme->ansicolor.color[IdYellow + 8], file);
	theme->ansicolor.color[IdBlue + 8] = BGGetColor("DarkBlue", theme->ansicolor.color[IdBlue + 8], file);
	theme->ansicolor.color[IdMagenta + 8] = BGGetColor("DarkMagenta", theme->ansicolor.color[IdMagenta + 8], file);
	theme->ansicolor.color[IdCyan + 8] = BGGetColor("DarkCyan", theme->ansicolor.color[IdCyan + 8], file);

	theme->vt.fg = BGGetColor("VTFore", theme->vt.fg, file);
	theme->vt.bg = BGGetColor("VTBack", theme->vt.bg, file);

	theme->blink.fg = BGGetColor("VTBlinkFore", theme->blink.fg, file);
	theme->blink.bg = BGGetColor("VTBlinkBack", theme->blink.bg, file);

	theme->bold.fg = BGGetColor("VTBoldFore", theme->bold.fg, file);
	theme->bold.bg = BGGetColor("VTBoldBack", theme->bold.bg, file);

	theme->underline.fg = BGGetColor("VTUnderlineFore", theme->underline.fg, file);
	theme->underline.bg = BGGetColor("VTUnderlineBack", theme->underline.bg, file);

	theme->reverse.fg = BGGetColor("VTReverseFore", theme->reverse.fg, file);
	theme->reverse.bg = BGGetColor("VTReverseBack", theme->reverse.bg, file);

	theme->url.fg = BGGetColor("URLFore", theme->url.fg, file);
	theme->url.bg = BGGetColor("URLBack", theme->url.bg, file);
}

/**
 *	BGをロード
 */
static void BGSaveColorOne(const TColorSetting *color, const char *key, const wchar_t *fn)
{
	char buf[512];
	COLORREF fg = color->fg;
	COLORREF bg = color->bg;

	sprintf(buf, "%d,%d, %d,%d,%d, %d,%d,%d", 1, 1,
			GetRValue(fg), GetGValue(fg), GetBValue(fg),
			GetRValue(bg), GetGValue(bg), GetBValue(bg));
	WritePrivateProfileStringAFileW("Color Theme", key, buf, fn);
}

static void BGSaveColorANSI(TAnsiColorSetting *color, const wchar_t *fn)
{
	int i;
	wchar_t *buff = NULL;
	awcscat(&buff, L"1, 1, ");

	for (i = 0; i < 16; i++) {
		wchar_t color_str[32];
		const COLORREF c = color->color[i];
		swprintf(color_str, _countof(color_str), L"%d,%d,%d, ", GetRValue(c), GetGValue(c), GetBValue(c));
		awcscat(&buff, color_str);
	}

	WritePrivateProfileStringW(L"Color Theme", L"ANSIColor", buff, fn);
	free(buff);
}

void ThemeSaveColor(TColorTheme *color_theme, const wchar_t *fn)
{
	WritePrivateProfileStringAFileW("Color Theme", "Theme", "teraterm theme editor", fn);

	BGSaveColorOne(&(color_theme->vt), "VTColor", fn);
	BGSaveColorOne(&(color_theme->bold), "BoldColor", fn);
	BGSaveColorOne(&(color_theme->blink), "BlinkColor", fn);
	BGSaveColorOne(&(color_theme->reverse), "ReverseColor", fn);
	BGSaveColorOne(&(color_theme->url), "URLColor", fn);

	BGSaveColorANSI(&(color_theme->ansicolor), fn);
}

void WriteInt3(const char *Sect, const char *Key, const wchar_t *FName,
			   int i1, int i2, int i3)
{
	char Temp[96];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d",
	            i1, i2,i3);
	WritePrivateProfileStringAFileW(Sect, Key, Temp, FName);
}

void WriteCOLORREF(const char *Sect, const char *Key, const wchar_t *FName, COLORREF color)
{
	int red = color & 0xff;
	int green = (color >> 8) & 0xff;
	int blue = (color >> 16) & 0xff;

	WriteInt3(Sect, Key, FName, red, green, blue);
}

static const char *GetBGPatternStr(BG_PATTERN id)
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

static BOOL GetBGPatternID(const char *str, BG_PATTERN *pattern)
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
		if (_stricmp(st->str, str) == 0) {
			*pattern = st->id;
			return TRUE;
		}
	}
}

void ThemeSaveBG(const BGTheme *bg_theme, const wchar_t *file)
{
	WritePrivateProfileStringAFileW(BG_SECTION, BG_DESTFILE, bg_theme->BGDest.file, file);
#if 0
	WritePrivateProfileStringAFileW(BG_SECTION, "BGDestType",
									bg_theme->BGDest.type == BG_PICTURE ? "picture" : "color", file);
#endif
	WriteCOLORREF(BG_SECTION, "BGDestColor", file, bg_theme->BGDest.color);
	WritePrivateProfileStringAFileW(BG_SECTION, "BGDestPattern", GetBGPatternStr(bg_theme->BGDest.pattern), file);

	WritePrivateProfileIntAFileW(BG_SECTION, "BGSrc1Alpha", bg_theme->BGSrc1.alpha, file);

	WritePrivateProfileIntAFileW(BG_SECTION, "BGSrc2Alpha", bg_theme->BGSrc2.alpha, file);
	WriteCOLORREF(BG_SECTION, "BGSrc2Color", file, bg_theme->BGSrc2.color);

	WritePrivateProfileIntW(BG_SECTIONW, L"BGReverseTextAlpha", bg_theme->BGReverseTextAlpha, file);
}

static int BGGetStrIndex(const char *name, int def, const wchar_t *file, const char * const *strList, int nList)
{
	char defstr[64], str[64];
	int i;

	def %= nList;

	strncpy_s(defstr, sizeof(defstr), strList[def], _TRUNCATE);
	GetPrivateProfileStringAFileW(BG_SECTION, name, defstr, str, 64, file);

	for (i = 0; i < nList; i++)
		if (!_stricmp(str, strList[i]))
			return i;

	return 0;
}

static BOOL BGGetOnOff(const char *name, BOOL def, const wchar_t *file)
{
	static const char * const strList[2] = {"Off", "On"};

	return (BOOL)BGGetStrIndex(name, def, file, strList, 2);
}

static BG_PATTERN BGGetPattern(const char *name, BG_PATTERN def, const wchar_t *file)
{
	BG_PATTERN retval;
	char str[64];
	GetPrivateProfileStringAFileW(BG_SECTION, name, "", str, _countof(str), file);
	if (str[0] == 0) {
		return def;
	}
	if (GetBGPatternID(str, &retval) == FALSE) {
		retval = def;
	}
	return retval;
}

static BG_TYPE BGGetType(const char *name, BG_TYPE def, const wchar_t *file)
{
	static const char *strList[3] = {"color", "picture", "wallpaper"};

	return (BG_TYPE)BGGetStrIndex(name, def, file, strList, 3);
}

/**
 *	BGをロード
 *		TODO 色の読出し
 */
void ThemeLoadBG(const wchar_t *file, BGTheme *bg_theme)
{
	char path[MAX_PATH];

	// Dest の読み出し
	bg_theme->BGDest.type = BGGetType("BGDestType", bg_theme->BGDest.type, file);
	bg_theme->BGDest.pattern = BGGetPattern("BGPicturePattern", bg_theme->BGDest.pattern, file);
	bg_theme->BGDest.pattern = BGGetPattern("BGDestPattern", bg_theme->BGDest.pattern, file);
	bg_theme->BGDest.antiAlias = BGGetOnOff("BGDestAntiAlias", bg_theme->BGDest.antiAlias, file);
	bg_theme->BGDest.color = BGGetColor("BGPictureBaseColor", bg_theme->BGDest.color, file);
	bg_theme->BGDest.color = BGGetColor("BGDestColor", bg_theme->BGDest.color, file);
	GetPrivateProfileStringAFileW(BG_SECTION, "BGPictureFile", bg_theme->BGDest.file, path, sizeof(bg_theme->BGDest.file), file);
	strcpy_s(bg_theme->BGDest.file, _countof(bg_theme->BGDest.file), path);
	GetPrivateProfileStringAFileW(BG_SECTION, BG_DESTFILE, bg_theme->BGDest.file, path, MAX_PATH, file);
	RandomFile(path, bg_theme->BGDest.file, sizeof(bg_theme->BGDest.file));
	if (bg_theme->BGDest.file[0] == 0) {
		// ファイル名が無効なので、Destを無効にする
		bg_theme->BGDest.type = BG_NONE;
	}

	// Src1 の読み出し
	bg_theme->BGSrc1.type = BGGetType("BGSrc1Type", bg_theme->BGSrc1.type, file);
	bg_theme->BGSrc1.pattern = BGGetPattern("BGSrc1Pattern", bg_theme->BGSrc1.pattern, file);
	bg_theme->BGSrc1.antiAlias = BGGetOnOff("BGSrc1AntiAlias", bg_theme->BGSrc1.antiAlias, file);
	bg_theme->BGSrc1.alpha = 255 - GetPrivateProfileIntAFileW(BG_SECTION, "BGPictureTone", 255 - bg_theme->BGSrc1.alpha, file);
	if (!strcmp(bg_theme->BGDest.file, ""))
		bg_theme->BGSrc1.alpha = 255;
	bg_theme->BGSrc1.alpha = GetPrivateProfileIntAFileW(BG_SECTION, "BGSrc1Alpha", bg_theme->BGSrc1.alpha, file);
	bg_theme->BGSrc1.color = BGGetColor("BGSrc1Color", bg_theme->BGSrc1.color, file);
	GetPrivateProfileStringAFileW(BG_SECTION, "BGSrc1File", bg_theme->BGSrc1.file, path, MAX_PATH, file);
	RandomFile(path, bg_theme->BGSrc1.file, sizeof(bg_theme->BGSrc1.file));

	// Src2 の読み出し
	bg_theme->BGSrc2.type = BGGetType("BGSrc2Type", bg_theme->BGSrc2.type, file);
	bg_theme->BGSrc2.pattern = BGGetPattern("BGSrc2Pattern", bg_theme->BGSrc2.pattern, file);
	bg_theme->BGSrc2.antiAlias = BGGetOnOff("BGSrc2AntiAlias", bg_theme->BGSrc2.antiAlias, file);
	bg_theme->BGSrc2.alpha = 255 - GetPrivateProfileIntAFileW(BG_SECTION, "BGFadeTone", 255 - bg_theme->BGSrc2.alpha, file);
	bg_theme->BGSrc2.alpha = GetPrivateProfileIntAFileW(BG_SECTION, "BGSrc2Alpha", bg_theme->BGSrc2.alpha, file);
	bg_theme->BGSrc2.color = BGGetColor("BGFadeColor", bg_theme->BGSrc2.color, file);
	bg_theme->BGSrc2.color = BGGetColor("BGSrc2Color", bg_theme->BGSrc2.color, file);
	GetPrivateProfileStringAFileW(BG_SECTION, "BGSrc2File", bg_theme->BGSrc2.file, path, MAX_PATH, file);
	RandomFile(path, bg_theme->BGSrc2.file, sizeof(bg_theme->BGSrc2.file));

	//その他読み出し
	bg_theme->BGReverseTextAlpha = GetPrivateProfileIntAFileW(BG_SECTION, "BGReverseTextTone", bg_theme->BGReverseTextAlpha, file);
	bg_theme->BGReverseTextAlpha = GetPrivateProfileIntAFileW(BG_SECTION, "BGReverseTextAlpha", bg_theme->BGReverseTextAlpha, file);
}

static void ReadANSIColorSetting(TAnsiColorSetting *color, const wchar_t *fn)
{
	char Buff[512];
	int c, r, g, b;
	// ANSIColor16は、明るい/暗いグループが入れ替わっている
	const static int index256[] = {
		0,
		9, 10, 11, 12, 13, 14, 15,
		8,
		1, 2, 3, 4, 5, 6, 7
	};

	GetPrivateProfileStringAFileW("Color Theme", "ANSIColor", "0", Buff, sizeof(Buff), fn);

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	color->enable = c;

	for (c=0; c<16; c++) {
		int idx = index256[c];
		GetNthNum(Buff, c * 3 + 3, &r);
		GetNthNum(Buff, c * 3 + 4, &g);
		GetNthNum(Buff, c * 3 + 5, &b);
		color->color[idx] = RGB(r, g, b);
	}
}

static void ReadColorSetting(TColorSetting *color, const char *key, const wchar_t *fn)
{
	char Buff[512];
	int c, r, g, b;

	GetPrivateProfileStringAFileW("Color Theme", key, "0", Buff, sizeof(Buff), fn);

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

	return;
}

/*
 *	カラーテーマiniファイルをロードする
 */
void ThemeLoadColor(const wchar_t *fn, TColorTheme *color_theme)
{
	GetPrivateProfileStringAFileW("Color Theme", "Theme", "", color_theme->name, _countof(color_theme->name), fn);

	ReadColorSetting(&(color_theme->vt), "VTColor", fn);
	ReadColorSetting(&(color_theme->bold), "BoldColor", fn);
	ReadColorSetting(&(color_theme->blink), "BlinkColor", fn);
	ReadColorSetting(&(color_theme->reverse), "ReverseColor", fn);
	ReadColorSetting(&(color_theme->url), "URLColor", fn);

	ReadANSIColorSetting(&(color_theme->ansicolor), fn);
}

#if 0
#define SECTION "Color Theme"

static void ReadANSIColorSetting(TAnsiColorSetting *color, const wchar_t *fn)
{
	char Buff[512];
	int c, r, g, b;

	GetPrivateProfileStringAFileW(SECTION, "ANSIColor", "0", Buff, sizeof(Buff), fn);

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	color->enable = c;

	for (c=0; c<16; c++) {
		GetNthNum(Buff, c * 3 + 3, &r);
		GetNthNum(Buff, c * 3 + 4, &g);
		GetNthNum(Buff, c * 3 + 5, &b);
		color->color[c] = RGB(r, g, b);
	}

	return;
}

static void ReadColorSetting(TColorSetting *color, char *ent, const wchar_t *fn)
{
	char Key[32], Buff[512];
	int c, r, g, b;

	_snprintf_s(Key, sizeof(Key), _TRUNCATE, "%s", ent);
	GetPrivateProfileStringAFileW(SECTION, Key, "0", Buff, sizeof(Buff), fn);

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	color->enable = c;

	GetNthNum(Buff, 3, &r);
	GetNthNum(Buff, 4, &g);
	GetNthNum(Buff, 5, &b);
	color->fg = RGB(r, g, b);

	GetNthNum(Buff, 6, &r);
	GetNthNum(Buff, 7, &g);
	GetNthNum(Buff, 8, &b);
	color->bg = RGB(r, g, b);
}

static void ReadColorSetting(TColorSetting *color, char *ent, const wchar_t *fn)
{
	char Key[32], Buff[512];
	int c, r, g, b;

	_snprintf_s(Key, sizeof(Key), _TRUNCATE, "%s", ent);
	GetPrivateProfileStringAFileW(SECTION, Key, "0", Buff, sizeof(Buff), fn);

	GetNthNum(Buff, 1, &c);
	color->change = c;

	GetNthNum(Buff, 2, &c);
	color->enable = c;

	GetNthNum(Buff, 3, &r);
	GetNthNum(Buff, 4, &g);
	GetNthNum(Buff, 5, &b);
	color->fg = RGB(r, g, b);

	GetNthNum(Buff, 6, &r);
	GetNthNum(Buff, 7, &g);
	GetNthNum(Buff, 8, &b);
	color->bg = RGB(r, g, b);
}

static void ReadColorTheme(const wchar_t *fn, TColorTheme *data)
{
	memset(data, 0, sizeof(*data));

	GetPrivateProfileStringAFileW(SECTION, "Theme", "", data->name,
								  sizeof(data->name), fn);
	if (data->name[0] == '\0')
		return;

	ReadColorSetting(&data->vt, "VTColor", fn);
	ReadColorSetting(&data->bold, "BoldColor", fn);
	ReadColorSetting(&data->blink, "BlinkColor", fn);
	ReadColorSetting(&data->reverse, "ReverseColor", fn);
	ReadColorSetting(&data->url, "URLColor", fn);

	ReadANSIColorSetting(&data->ansicolor, fn);
}

#endif

#if 0
static void WriteInt(PCHAR Sect, PCHAR Key, const wchar_t *FName, int i)
{
	char Temp[15];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", i);
	WritePrivateProfileStringAFileW(Sect, Key, Temp, FName);
}
#endif

/**
 *	テーマファイルの書き込み
 */
#if 0
void BGWriteThemeFile(const wchar_t *theme_file)
{
	WritePrivateProfileStringAFileW(BG_SECTION, BG_DESTFILE, BGDest.file, theme_file);
	BGSrc1.alpha = GetPrivateProfileIntAFileW(BG_SECTION, "BGSrc1Alpha", BGSrc1.alpha, theme_file);
	WriteInt(BG_SECTION, BG_THEME_IMAGE_BRIGHTNESS1, theme_file, BGSrc1.alpha);
	WriteInt(BG_SECTION, BG_THEME_IMAGE_BRIGHTNESS2, theme_file, BGSrc2.alpha);
}
#endif

/**
 *	テーマファイルの書き込み
 */
#if 0
static void BGWriteIniFile(const wchar_t *file)
{
	WritePrivateProfileStringAFileW(BG_SECTION, BG_DESTFILE, BGDest.file, file);
	WritePrivateProfileStringAFileW(BG_SECTION, "BGDestType",
									BGDest.type == BG_PICTURE ? "picture" : "color", file);
	WriteCOLORREF(BG_SECTION, "BGDestColor", file, BGDest.color);
	WritePrivateProfileStringAFileW(BG_SECTION, "BGDestPattern", GetBGPatternStr(BGDest.pattern), file);

	WritePrivateProfileIntAFileW(BG_SECTION, "BGSrc1Alpha", BGSrc1.alpha, file);

	WritePrivateProfileIntAFileW(BG_SECTION, "BGSrc2Alpha", BGSrc2.alpha, file);
	WriteCOLORREF(BG_SECTION, "BGSrc2Color", file, BGSrc2.color);
}
#endif

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
			if (_wcsicmp(p, L"BG") == 0) {
				bg = TRUE;
			}
			else if(_wcsicmp(p, L"Color Theme") == 0) {
				color = TRUE;
			}
			i += len;
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
