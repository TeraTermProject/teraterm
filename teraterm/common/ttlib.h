/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* useful routines */

#include <time.h>	// for time_t

#pragma once

#include "i18n.h"
#include "ttlib_static_dir.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DllExport)
#define DllExport __declspec(dllexport)
#endif

#if defined(_MSC_VER) && !defined(_Printf_format_string_)
// 定義されていないときは何もしないように定義しておく
#define _Printf_format_string_
#endif

BOOL GetFileNamePos(const char *PathName, int *DirLen, int *FNPos);
BOOL GetFileNamePosU8(const char *PathName, int *DirLen, int *FNPos);
BOOL GetFileNamePosW(const wchar_t *PathName, size_t *DirLen, size_t *FNPos);
DllExport BOOL ExtractFileName(const char *PathName, PCHAR FileName, int destlen);
DllExport BOOL ExtractDirName(const char *PathName, PCHAR DirName);
wchar_t *ExtractDirNameW(const wchar_t *PathName);
wchar_t *ExtractFileNameW(const wchar_t *PathName);
void FitFileName(PCHAR FileName, int destlen, const char *DefExt);
void FitFileNameW(wchar_t *FileName, size_t destlen, const wchar_t *DefExt);
void AppendSlash(PCHAR Path, int destlen);
void AppendSlashW(wchar_t *Path, size_t destlen);
void DeleteSlash(PCHAR Path);
wchar_t *DeleteSlashW(const wchar_t *Path);
void Str2Hex(PCHAR Str, PCHAR Hex, int Len, int MaxHexLen, BOOL ConvSP);
wchar_t *Str2HexW(const wchar_t *Str, size_t Len, BOOL ConvSP);
BYTE ConvHexChar(BYTE b);
int Hex2Str(PCHAR Hex, PCHAR Str, int MaxLen);
wchar_t *Hex2StrW(const wchar_t *Hex, size_t MaxLen);
BOOL DoesFileExist(const char *FName);
BOOL DoesFolderExist(const char *FName);
BOOL DoesFolderExistW(const wchar_t *FName);
long GetFSize(const char *FName);
unsigned long long GetFSize64H(HANDLE hFile);
unsigned long long GetFSize64W(const wchar_t *FName);
unsigned long long GetFSize64A(const char *FName);
long GetFMtime(const char *FName);
BOOL SetFMtime(const char *FName, DWORD mtime);
void uint2str(UINT i, PCHAR Str, int destlen, int len);
BOOL isInvalidFileNameCharW(const wchar_t *FName);
int isInvalidFileNameChar(const char *FName);
#define deleteInvalidFileNameChar(name) replaceInvalidFileNameChar(name, 0)
wchar_t *replaceInvalidFileNameCharW(const wchar_t *FName, wchar_t c);
char *replaceInvalidFileNameCharU8(const char *FName, char c);
DllExport void replaceInvalidFileNameChar(PCHAR FName, unsigned char c);
BOOL isInvalidStrftimeCharW(const wchar_t *format);
int isInvalidStrftimeChar(PCHAR FName);
void deleteInvalidStrftimeCharW(wchar_t *FName);
void deleteInvalidStrftimeChar(PCHAR FName);
void ParseStrftimeFileName(PCHAR FName, int destlen);
#if 0
void ConvFName(const char *HomeDir, PCHAR Temp, int templen, const char *DefExt, PCHAR FName, int destlen);
void ConvFNameW(const wchar_t *HomeDir, wchar_t *Temp, size_t templen, const wchar_t *DefExt, wchar_t *FName, size_t destlen);
#endif
void RestoreNewLine(PCHAR Text);
size_t RestoreNewLineW(wchar_t *Text);
BOOL GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest);
void GetNthNum(PCHAR Source, int Nth, int *Num);
int GetNthNum2(PCHAR Source, int Nth, int defval);
wchar_t *GetDefaultFNameW(const wchar_t *home, const wchar_t *file);
wchar_t *GetDefaultSetupFNameW(const wchar_t *home);
BOOL GetNthStringW(const wchar_t *Source, int Nth, size_t Size, wchar_t *Dest);
void GetNthNumW(const wchar_t *Source, int Nth, int *Num);
int GetNthNum2W(const wchar_t *Source, int Nth, int defval);
wchar_t *GetUILanguageFileFullW(const wchar_t *SetupFNameW);
void GetOnOffEntryInifile(char *entry, char *buf, int buflen);
// __declspec(deprecated("Replace GetI18nStrWW()"))
void get_lang_msg(const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile);
// __declspec(deprecated("Replace GetI18nStrWW()"))
void get_lang_msgW(const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile);
int get_lang_font(const char *key, HWND dlg, PLOGFONT logfont, HFONT *font, const char *iniFile);
#if defined(_MSC_VER)
DllExport void OutputDebugPrintf(_Printf_format_string_ const char *fmt, ...);
void OutputDebugPrintfW(_Printf_format_string_ const wchar_t *fmt, ...);
#elif defined(__GNUC__)
DllExport void OutputDebugPrintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void OutputDebugPrintfW(const wchar_t *fmt, ...); // __attribute__ ((format (wprintf, 1, 2)));
#else
DllExport void OutputDebugPrintf(const char *fmt, ...);
void OutputDebugPrintfW(const wchar_t *fmt, ...);
#endif
void OutputDebugHexDump(const void *data, size_t len);
BOOL IsWindowsVer(DWORD dwPlatformId, DWORD dwMajorVersion, DWORD dwMinorVersion);
BOOL IsWindowsVerOrLater(DWORD dwMajorVersion, DWORD dwMinorVersion);
DllExport DWORD get_OPENFILENAME_SIZEA();
DllExport DWORD get_OPENFILENAME_SIZEW();
DllExport BOOL IsWindows95();
DllExport BOOL IsWindowsMe();
DllExport BOOL IsWindowsNT4();
DllExport BOOL IsWindowsNTKernel();
DllExport BOOL IsWindows2000();
DllExport BOOL IsWindows2000OrLater();
BOOL IsWindowsXPOrLater(void);
DllExport BOOL IsWindowsVistaOrLater();
DllExport BOOL IsWindows7OrLater();
DllExport BOOL HasMultiMonitorSupport();
DllExport BOOL HasDnsQuery();
DllExport BOOL HasBalloonTipSupport();
DllExport char *mctimelocal(const char *format, BOOL utc_flag);
char *strelapsed(DWORD start_time);

void b64encode(PCHAR dst, int dsize, PCHAR src, int len);
DllExport int b64decode(PCHAR dst, int dsize, PCHAR src);

DllExport wchar_t * PASCAL GetParam(wchar_t *buff, size_t size, wchar_t *param);
DllExport void PASCAL DequoteParam(wchar_t *dest, size_t dest_len, wchar_t *src);
void PASCAL DeleteComment(PCHAR dest, int dest_size, PCHAR src);
wchar_t *DeleteCommentW(const wchar_t *src);

void split_buffer(char *buffer, int delimiter, char **head, char **body);
BOOL GetPositionOnWindow(
	HWND hWnd, const POINT *point,
	BOOL *InWindow, BOOL *InClient, BOOL *InTitleBar);
DllExport void GetMessageboxFont(LOGFONTA *logfont);
void GetMessageboxFontW(LOGFONTW *logfont);
void GetDesktopRect(HWND hWnd, RECT *rect);
void GetDesktopRectFromPoint(const POINT *p, RECT *rect);
void CenterWindow(HWND hWnd, HWND hWndParent);
void MoveWindowToDisplay(HWND hWnd);
void MoveWindowToDisplayPoint(HWND hWnd, const POINT *p);

#define CheckFlag(var, flag)	(((var) & (flag)) != 0)

int SetDlgTextsW(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFileW);
int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile);
void SetDlgMenuTextsW(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const wchar_t *UILanguageFile);
void SetDlgMenuTexts(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile);
int GetMonitorDpiFromWindow(HWND hWnd);

#define	get_OPENFILENAME_SIZE() get_OPENFILENAME_SIZEA()

/*
 * シリアルポート関連の設定定義
 */
enum serial_port_conf {
	COM_DATABIT,
	COM_PARITY,
	COM_STOPBIT,
	COM_FLOWCTRL,
};

/*
 *	ttlib_static
 */
typedef struct {
	const char *section;			// セクション名
	const char *title_key;			// タイトル(NULLのとき、title_default を常に使用)
	const wchar_t *title_default;	//   lng ファイルに見つからなかったとき使用
	const char *message_key;		// メッセージ
	const wchar_t *message_default;	//   lng ファイルに見つからなかったとき使用
	UINT uType;						// メッセージボックスのタイプ
} TTMessageBoxInfoW;

int TTMessageBoxA(HWND hWnd, const TTMessageBoxInfoW *info, const char *UILanguageFile, ...);
int TTMessageBoxW(HWND hWnd, const TTMessageBoxInfoW *info, const wchar_t *UILanguageFile, ...);
wchar_t *GetClipboardTextW(HWND hWnd, BOOL empty);
char *GetClipboardTextA(HWND hWnd, BOOL empty);
BOOL CBSetTextW(HWND hWnd, const wchar_t *str_w, size_t str_len);
void TTInsertMenuItemA(HMENU hMenu, UINT targetItemID, UINT flags, UINT newItemID, const char *text, BOOL before);
BOOL IsTextW(const wchar_t *str, size_t len);
wchar_t *NormalizeLineBreakCR(const wchar_t *src, size_t *len);
wchar_t *NormalizeLineBreakCRLF(const wchar_t *src_);
BOOL IsRelativePathA(const char *path);
BOOL IsRelativePathW(const wchar_t *path);
DWORD TTWinExecA(const char *commandA);
void CreateBakupFile(const wchar_t *fname, const wchar_t *prev_str);
BOOL ConvertIniFileCharCode(const wchar_t *fname,  const wchar_t *bak_str);
wchar_t *MakeISO8601Str(time_t t);
void SaveBmpFromHDC(const wchar_t* fname, HDC hdc, int width, int height);
wchar_t *RandomFileW(const wchar_t *filespec_src);
void RandomFile(const char *filespec_src,char *filename, int destlen);
int __ismbblead(BYTE b, int code_page);
void SetDPIAwareness(const wchar_t *SetupFNameW);
wchar_t *GetFullPathW(const wchar_t *dir, const wchar_t *rel_path);
char *GetVersionSubstr(void);
DWORD TTCreateProcess(const wchar_t *cmd, const wchar_t *arg1, const wchar_t *arg2);

#ifdef __cplusplus
}
#endif

#include "ttcommdlg.h"
