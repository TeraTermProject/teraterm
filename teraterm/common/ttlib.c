/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2019 TeraTerm Project
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

/* misc. routines  */

#include <sys/stat.h>
#include <sys/utime.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <shlobj.h>
#include <ctype.h>
#include <mbctype.h>	// for _ismbblead
#include <assert.h>

#include "teraterm_conf.h"
#include "teraterm.h"
#include "tttypes.h"
#include "compat_win.h"

#include "../teraterm/unicode_test.h"

/* OS version with GetVersionEx(*1)

                dwMajorVersion   dwMinorVersion    dwPlatformId
Windows95       4                0                 VER_PLATFORM_WIN32_WINDOWS
Windows98       4                10                VER_PLATFORM_WIN32_WINDOWS 
WindowsMe       4                90                VER_PLATFORM_WIN32_WINDOWS
WindowsNT4.0    4                0                 VER_PLATFORM_WIN32_NT
Windows2000     5                0                 VER_PLATFORM_WIN32_NT
WindowsXP       5                1                 VER_PLATFORM_WIN32_NT
WindowsXPx64    5                2                 VER_PLATFORM_WIN32_NT
WindowsVista    6                0                 VER_PLATFORM_WIN32_NT
Windows7        6                1                 VER_PLATFORM_WIN32_NT
Windows8        6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*2)  6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*3)  6                3                 VER_PLATFORM_WIN32_NT
Windows10(*2)   6                2                 VER_PLATFORM_WIN32_NT
Windows10(*3)   10               0                 VER_PLATFORM_WIN32_NT

(*1) GetVersionEx()が c4996 warning となるのは、VS2013(_MSC_VER=1800) からです。
(*2) manifestに supportedOS Id を追加していない。
(*3) manifestに supportedOS Id を追加している。
*/

// for isInvalidFileNameChar / replaceInvalidFileNameChar
static char *invalidFileNameChars = "\\/:*?\"<>|";

// ファイルに使用することができない文字
// cf. Naming Files, Paths, and Namespaces
//     http://msdn.microsoft.com/en-us/library/aa365247.aspx
// (2013.3.9 yutaka)
static char *invalidFileNameStrings[] = {
	"AUX", "CLOCK$", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
	"CON", "CONFIG$", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
	"NUL", "PRN",
	".", "..", 
	NULL
};


// for b64encode/b64decode
static char *b64enc_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char b64dec_table[] = {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
   -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void b64encode(PCHAR d, int dsize, PCHAR s, int len)
{
	unsigned int b = 0;
	int state = 0;
	unsigned char *src, *dst;

	src = (unsigned char *)s;
	dst = (unsigned char *)d;

	if (dsize == 0 || dst == NULL || src == NULL) {
		return;
	}
	if (dsize < 5) {
		*dst = 0;
		return;
	}

	while (len > 0) {
		b = (b << 8) | *src++;
		len--;
		state++;

		if (state == 3) {
			*dst++ = b64enc_table[(b>>18) & 0x3f];
			*dst++ = b64enc_table[(b>>12) & 0x3f];
			*dst++ = b64enc_table[(b>>6) & 0x3f];
			*dst++ = b64enc_table[b & 0x3f];
			dsize -= 4;
			state = 0;
			b = 0;
			if (dsize < 5)
				break;
		}
	}

	if (dsize >= 5) {
		if (state == 1) {
			*dst++ = b64enc_table[(b>>2) & 0x3f];
			*dst++ = b64enc_table[(b<<4) & 0x3f];
			*dst++ = '=';
			*dst++ = '=';
		}
		else if (state == 2) {
			*dst++ = b64enc_table[(b>>10) & 0x3f];
			*dst++ = b64enc_table[(b>>4) & 0x3f];
			*dst++ = b64enc_table[(b<<2) & 0x3f];
			*dst++ = '=';
		}
	}

	*dst = 0;
	return;
}

int b64decode(PCHAR dst, int dsize, PCHAR src)
{
	unsigned int b = 0;
	char c;
	int len = 0, state = 0;

	if (src == NULL || dst == NULL || dsize == 0)
		return -1;

	while (1) {
		if (isspace(*src)) {
			src++;
			continue;
		}

		if ((c = b64dec_table[*src++]) == -1)
			break;

		b = (b << 6) | c;
		state++;

		if (state == 4) {
			if (dsize > len)
				dst[len++] = (b >> 16) & 0xff;
			if (dsize > len)
				dst[len++] = (b >> 8) & 0xff;
			if (dsize > len)
				dst[len++] = b & 0xff;
			state = 0;
			if (dsize <= len)
				break;
		}
	}

	if (state == 2) {
		b <<= 4;
		if (dsize > len)
			dst[len++] = (b >> 8) & 0xff;
	}
	else if (state == 3) {
		b <<= 6;
		if (dsize > len)
			dst[len++] = (b >> 16) & 0xff;
		if (dsize > len)
			dst[len++] = (b >> 8) & 0xff;
	}
	if (len < dsize) {
		dst[len] = 0;
	}
	else {
		dst[dsize-1] = 0;
	}
	return len;
}

BOOL GetFileNamePos(PCHAR PathName, int far *DirLen, int far *FNPos)
{
	BYTE b;
	LPTSTR Ptr, DirPtr, FNPtr, PtrOld;

	*DirLen = 0;
	*FNPos = 0;
	if (PathName==NULL)
		return FALSE;

	if ((strlen(PathName)>=2) && (PathName[1]==':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr = CharNext(Ptr);

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		b = Ptr[0];
		PtrOld = Ptr;
		Ptr = CharNext(Ptr);
		switch (b) {
			case ':':
				return FALSE;
			case '/':	/* FALLTHROUGH */
			case '\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	*DirLen = DirPtr-PathName;
	*FNPos = FNPtr-PathName;
	return TRUE;
}

BOOL ExtractFileName(PCHAR PathName, PCHAR FileName, int destlen)
{
	int i, j;

	if (FileName==NULL)
		return FALSE;
	if (! GetFileNamePos(PathName,&i,&j))
		return FALSE;
	strncpy_s(FileName,destlen,&PathName[j],_TRUNCATE);
	return (strlen(FileName)>0);
}

BOOL ExtractDirName(PCHAR PathName, PCHAR DirName)
{
	int i, j;

	if (DirName==NULL)
		return FALSE;
	if (! GetFileNamePos(PathName,&i,&j))
		return FALSE;
	memmove(DirName,PathName,i); // do not use memcpy
	DirName[i] = 0;
	return TRUE;
}

/* fit a filename to the windows-filename format */
/* FileName must contain filename part only. */
void FitFileName(PCHAR FileName, int destlen, const char *DefExt)
{
	int i, j, NumOfDots;
	char Temp[MAX_PATH];
	BYTE b;

	NumOfDots = 0;
	i = 0;
	j = 0;
	/* filename started with a dot is illeagal */
	if (FileName[0]=='.') {
		Temp[0] = '_';  /* insert an underscore char */
		j++;
	}

	do {
		b = FileName[i];
		i++;
		if (b=='.')
			NumOfDots++;
		if ((b!=0) &&
		    (j < MAX_PATH-1)) {
			Temp[j] = b;
			j++;
		}
	} while (b!=0);
	Temp[j] = 0;

	if ((NumOfDots==0) &&
	    (DefExt!=NULL)) {
		/* add the default extension */
		strncat_s(Temp,sizeof(Temp),DefExt,_TRUNCATE);
	}

	strncpy_s(FileName,destlen,Temp,_TRUNCATE);
}

// Append a slash to the end of a path name
void AppendSlash(PCHAR Path, int destlen)
{
	if (strcmp(CharPrev((LPCTSTR)Path,
	           (LPCTSTR)(&Path[strlen(Path)])),
	           "\\") != 0) {
		strncat_s(Path,destlen,"\\",_TRUNCATE);
	}
}

// Delete slashes at the end of a path name
void DeleteSlash(PCHAR Path)
{
	size_t i;
	for (i=strlen(Path)-1; i>=0; i--) {
		if (i ==0 && Path[i] == '\\' ||
		    Path[i] == '\\' && !_ismbblead(Path[i-1])) {
			Path[i] = '\0';
		}
		else {
			break;
		}
	}
}

void Str2Hex(PCHAR Str, PCHAR Hex, int Len, int MaxHexLen, BOOL ConvSP)
{
	BYTE b, low;
	int i, j;

	if (ConvSP)
		low = 0x20;
	else
		low = 0x1F;

	j = 0;
	for (i=0; i<=Len-1; i++) {
		b = Str[i];
		if ((b!='$') && (b>low) && (b<0x7f)) {
			if (j < MaxHexLen) {
				Hex[j] = b;
				j++;
			}
		}
		else {
			if (j < MaxHexLen-2) {
				Hex[j] = '$';
				j++;
				if (b<=0x9f) {
					Hex[j] = (char)((b >> 4) + 0x30);
				}
				else {
					Hex[j] = (char)((b >> 4) + 0x37);
				}
				j++;
				if ((b & 0x0f) <= 0x9) {
					Hex[j] = (char)((b & 0x0f) + 0x30);
				}
				else {
					Hex[j] = (char)((b & 0x0f) + 0x37);
				}
				j++;
			}
		}
	}
	Hex[j] = 0;
}

BYTE ConvHexChar(BYTE b)
{
	if ((b>='0') && (b<='9')) {
		return (b - 0x30);
	}
	else if ((b>='A') && (b<='F')) {
		return (b - 0x37);
	}
	else if ((b>='a') && (b<='f')) {
		return (b - 0x57);
	}
	else {
		return 0;
	}
}

int Hex2Str(PCHAR Hex, PCHAR Str, int MaxLen)
{
	BYTE b, c;
	int i, imax, j;

	j = 0;
	imax = strlen(Hex);
	i = 0;
	while ((i < imax) && (j<MaxLen)) {
		b = Hex[i];
		if (b=='$') {
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = ConvHexChar(c) << 4;
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = b + ConvHexChar(c);
		};

		Str[j] = b;
		j++;
		i++;
	}
	if (j<MaxLen) {
		Str[j] = 0;
	}

	return j;
}

BOOL DoesFileExist(PCHAR FName)
{
	// check if a file exists or not
	// フォルダまたはファイルがあれば TRUE を返す
	struct _stat st;

	return (_stat(FName,&st)==0);
}

BOOL DoesFolderExist(PCHAR FName)
{
	// check if a folder exists or not
	// マクロ互換性のため
	// DoesFileExist は従来通りフォルダまたはファイルがあれば TRUE を返し
	// DoesFileExist はフォルダがある場合のみ TRUE を返す。
	struct _stat st;

	if (_stat(FName,&st)==0) {
		if ((st.st_mode & _S_IFDIR) > 0) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	else {
		return FALSE;
	}
}

long GetFSize(PCHAR FName)
{
	struct _stat st;

	if (_stat(FName,&st)==-1) {
		return 0;
	}
	return (long)st.st_size;
}

long GetFMtime(PCHAR FName)
{
	struct _stat st;

	if (_stat(FName,&st)==-1) {
		return 0;
	}
	return (long)st.st_mtime;
}

BOOL SetFMtime(PCHAR FName, DWORD mtime)
{
	struct _utimbuf filetime;

	filetime.actime = mtime;
	filetime.modtime = mtime;
	return _utime(FName, &filetime);
}

void uint2str(UINT i, PCHAR Str, int destlen, int len)
{
	char Temp[20];

	memset(Temp, 0, sizeof(Temp));
	_snprintf_s(Temp,sizeof(Temp),_TRUNCATE,"%u",i);
	Temp[len] = 0;
	strncpy_s(Str,destlen,Temp,_TRUNCATE);
}

void QuoteFName(PCHAR FName)
{
	int i;

	if (FName[0]==0) {
		return;
	}
	if (strchr(FName,' ')==NULL) {
		return;
	}
	i = strlen(FName);
	memmove(&(FName[1]),FName,i);
	FName[0] = '\"';
	FName[i+1] = '\"';
	FName[i+2] = 0;
}

// ファイル名に使用できない文字が含まれているか確かめる (2006.8.28 maya)
int isInvalidFileNameChar(PCHAR FName)
{
	int i, len;
	char **p, c;

	// チェック対象の文字を強化した。(2013.3.9 yutaka)
	p = invalidFileNameStrings;
	while (*p) {
		if (_strcmpi(FName, *p) == 0) {
			return 1;  // Invalid
		}
		p++;
	}

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			i++;
			continue;
		}
		if ((FName[i] >= 0 && FName[i] < ' ') || strchr(invalidFileNameChars, FName[i])) {
			return 1;
		}
	}

	// ファイル名の末尾にピリオドおよび空白はNG。
	c = FName[len - 1];
	if (c == '.' || c == ' ')
		return 1;

	return 0;
}

// ファイル名に使用できない文字を c に置き換える
// c に 0 を指定した場合は文字を削除する
void replaceInvalidFileNameChar(PCHAR FName, unsigned char c)
{
	int i, j=0, len;

	if ((c >= 0 && c < ' ') || strchr(invalidFileNameChars, c)) {
		c = 0;
	}

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			FName[j++] = FName[i];
			FName[j++] = FName[++i];
			continue;
		}
		if ((FName[i] >= 0 && FName[i] < ' ') || strchr(invalidFileNameChars, FName[i])) {
			if (c) {
				FName[j++] = c;
			}
		}
		else {
			FName[j++] = FName[i];
		}
	}
	FName[j] = 0;
}

// strftime に渡せない文字が含まれているか確かめる (2006.8.28 maya)
int isInvalidStrftimeChar(PCHAR FName)
{
	int i, len, p;

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (FName[i] == '%') {
			if (FName[i+1] != 0) {
				p = i+1;
				if (FName[i+2] != 0 && FName[i+1] == '#') {
					p = i+2;
				}
				switch (FName[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						i = p;
						break;
					default:
						return 1;
				}
			}
			else {
				// % で終わっている場合はエラーとする
				return 1;
			}
		}
	}

	return 0;
}

// strftime に渡せない文字を削除する (2006.8.28 maya)
void deleteInvalidStrftimeChar(PCHAR FName)
{
	int i, j=0, len, p;

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (FName[i] == '%') {
			if (FName[i+1] != 0) {
				p = i+1;
				if (FName[i+2] != 0 && FName[i+1] == '#') {
					p = i+2;
				}
				switch (FName[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						FName[j] = FName[i]; // %
						j++;
						i++;
						if (p-i == 2) {
							FName[j] = FName[i]; // #
							j++;
							i++;
						}
						FName[j] = FName[i];
						j++;
						break;
					default:
						i++; // %
						if (p-i == 2) {
							i++; // #
						}
				}
			}
			// % で終わっている場合はコピーしない
		}
		else {
			FName[j] = FName[i];
			j++;
		}
	}

	FName[j] = 0;
}

// フルパスから、ファイル名部分のみを strftime で変換する (2006.8.28 maya)
void ParseStrftimeFileName(PCHAR FName, int destlen)
{
	char filename[MAX_PATH];
	char dirname[MAX_PATH];
	char buf[MAX_PATH];
	char *c;
	time_t time_local;
	struct tm tm_local;

	// ファイル名部分のみを flename に格納
	ExtractFileName(FName, filename ,sizeof(filename));

	// strftime に使用できない文字を削除
	deleteInvalidStrftimeChar(filename);

	// 現在時刻を取得
	time(&time_local);
	localtime_s(&tm_local, &time_local);

	// 時刻文字列に変換
	if (strftime(buf, sizeof(buf), filename, &tm_local) == 0) {
		strncpy_s(buf, sizeof(buf), filename, _TRUNCATE);
	}

	// ファイル名に使用できない文字を削除
	deleteInvalidFileNameChar(buf);

	c = strrchr(FName, '\\');
	if (c != NULL) {
		ExtractDirName(FName, dirname);
		strncpy_s(FName, destlen, dirname, _TRUNCATE);
		AppendSlash(FName,destlen);
		strncat_s(FName, destlen, buf, _TRUNCATE);
	}
	else { // "\"を含まない(フルパスでない)場合に対応 (2006.11.30 maya)
		strncpy_s(FName, destlen, buf, _TRUNCATE);
	}
}

void ConvFName(PCHAR HomeDir, PCHAR Temp, int templen, PCHAR DefExt, PCHAR FName, int destlen)
{
	// destlen = sizeof FName
	int DirLen, FNPos;

	FName[0] = 0;
	if ( ! GetFileNamePos(Temp,&DirLen,&FNPos) ) {
		return;
	}
	FitFileName(&Temp[FNPos],templen - FNPos,DefExt);
	if ( DirLen==0 ) {
		strncpy_s(FName,destlen,HomeDir,_TRUNCATE);
		AppendSlash(FName,destlen);
	}
	strncat_s(FName,destlen,Temp,_TRUNCATE);
}

// "\n" を改行に変換する (2006.7.29 maya)
// "\t" をタブに変換する (2006.11.6 maya)
void RestoreNewLine(PCHAR Text)
{
	int i, j=0, size=strlen(Text);
	char *buf = (char *)_alloca(size+1);

	memset(buf, 0, size+1);
	for (i=0; i<size; i++) {
		if (Text[i] == '\\' && i<size ) {
			switch (Text[i+1]) {
				case '\\':
					buf[j] = '\\';
					i++;
					break;
				case 'n':
					buf[j] = '\n';
					i++;
					break;
				case 't':
					buf[j] = '\t';
					i++;
					break;
				case '0':
					buf[j] = '\0';
					i++;
					break;
				default:
					buf[j] = '\\';
			}
			j++;
		}
		else {
			buf[j] = Text[i];
			j++;
		}
	}
	/* use memcpy to copy with '\0' */
	memcpy(Text, buf, size);
}

void RestoreNewLineW(wchar_t *Text)
{
	int i, j=0;
	int size= wcslen(Text);
	wchar_t *buf = (wchar_t *)_alloca((size+1) * sizeof(wchar_t));

	memset(buf, 0, (size+1) * sizeof(wchar_t));
	for (i=0; i<size; i++) {
		if (Text[i] == L'\\' && i<size ) {
			switch (Text[i+1]) {
				case L'\\':
					buf[j] = L'\\';
					i++;
					break;
				case L'n':
					buf[j] = L'\n';
					i++;
					break;
				case L't':
					buf[j] = L'\t';
					i++;
					break;
				case L'0':
					buf[j] = L'\0';
					i++;
					break;
				default:
					buf[j] = L'\\';
			}
			j++;
		}
		else {
			buf[j] = Text[i];
			j++;
		}
	}
	/* use memcpy to copy with '\0' */
	memcpy(Text, buf, size * sizeof(wchar_t));
}

BOOL GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest)
{
	int i, j, k;

	i = 1;
	j = 0;
	k = 0;

	while (i<Nth && Source[j] != 0) {
		if (Source[j++] == ',') {
			i++;
		}
	}

	if (i == Nth) {
		while (Source[j] != 0 && Source[j] != ',' && k<Size-1) {
			Dest[k++] = Source[j++];
		}
	}

	Dest[k] = 0;
	return (i>=Nth);
}

void GetNthNum(PCHAR Source, int Nth, int far *Num)
{
	char T[15];

	GetNthString(Source,Nth,sizeof(T),T);
	if (sscanf_s(T, "%d", Num) != 1) {
		*Num = 0;
	}
}

int GetNthNum2(PCHAR Source, int Nth, int defval)
{
	char T[15];
	int v;

	GetNthString(Source, Nth, sizeof(T), T);
	if (sscanf_s(T, "%d", &v) != 1) {
		v = defval;
	}
	
	return v;
}

void GetDownloadFolder(char *dest, int destlen)
{
	HMODULE hDll;
	typedef GUID KNOWNFOLDERID;
	typedef HRESULT(WINAPI *SHGETKNOWNFOLDERPATH)(KNOWNFOLDERID*, DWORD, HANDLE, PWSTR*);
	// {374DE290-123F-4565-9164-39C4925E467B}
	KNOWNFOLDERID FOLDERID_Downloads = { 0x374de290, 0x123f, 0x4565, 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b };
	char download[MAX_PATH];

	memset(download, 0, sizeof(download));
	if (hDll = LoadLibrary("shell32.dll")) {
		SHGETKNOWNFOLDERPATH pSHGetKnownFolderPath = (SHGETKNOWNFOLDERPATH)GetProcAddress(hDll, "SHGetKnownFolderPath");
		if (pSHGetKnownFolderPath) {
			PWSTR pBuffer = NULL;
			pSHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &pBuffer);
			WideCharToMultiByte(CP_ACP, 0, pBuffer, -1, download, sizeof(download), NULL, NULL);
		}
		FreeLibrary(hDll);
	}
	if (strlen(download) == 0) {
		LPITEMIDLIST pidl;
		if (SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl) == NOERROR) {
			SHGetPathFromIDList(pidl, download);
			CoTaskMemFree(pidl);
		}
	}
	if (strlen(download) > 0) {
		strncpy_s(dest, destlen, download, _TRUNCATE);
	}
}

void GetDefaultFName(const char *home, const char *file, char *dest, int destlen)
{
	// My Documents に file がある場合、
	// それを読み込むようにした。(2007.2.18 maya)
	char MyDoc[MAX_PATH];
	char MyDocSetupFName[MAX_PATH];
	LPITEMIDLIST pidl;

	IMalloc *pmalloc;
	SHGetMalloc(&pmalloc);
	if (SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl) == S_OK) {
		SHGetPathFromIDList(pidl, MyDoc);
		pmalloc->lpVtbl->Free(pmalloc, pidl);
		pmalloc->lpVtbl->Release(pmalloc);
	}
	else {
		pmalloc->lpVtbl->Release(pmalloc);
		goto homedir;
	}
	strncpy_s(MyDocSetupFName, sizeof(MyDocSetupFName), MyDoc, _TRUNCATE);
	AppendSlash(MyDocSetupFName,sizeof(MyDocSetupFName));
	strncat_s(MyDocSetupFName, sizeof(MyDocSetupFName), file, _TRUNCATE);
	if (GetFileAttributes(MyDocSetupFName) != INVALID_FILE_ATTRIBUTES) {
		strncpy_s(dest, destlen, MyDocSetupFName, _TRUNCATE);
		return;
	}

homedir:
	strncpy_s(dest, destlen, home, _TRUNCATE);
	AppendSlash(dest,destlen);
	strncat_s(dest, destlen, file, _TRUNCATE);
}

/*
 * Get home(exe,dll) directory
 * @param[in]		hInst		WinMain()の HINSTANCE または NULL
 * @param[in,out]	HomeDir
 * @param[out]		HomeDirLen
 */
void GetHomeDir(HINSTANCE hInst, char *HomeDir, size_t HomeDirLen)
{
	char Temp[MAX_PATH];
	DWORD result = GetModuleFileName(NULL,Temp,sizeof(Temp));
	if (result == 0 || result == _countof(Temp)) {
		// パスの取得に失敗した。致命的、abort() する。
		abort();
		// ここでreturnしてもプラグイン(ttpset.dll)のロードに失敗してabort()する
	}
	ExtractDirName(Temp, Temp);
	strncpy_s(HomeDir, HomeDirLen, Temp, _TRUNCATE);
}

// デフォルトの TERATERM.INI のフルパスを ttpmacro からも
// 取得するために追加した。(2007.2.18 maya)
void GetDefaultSetupFName(const char *home, char *dest, int destlen)
{
	GetDefaultFName(home, "TERATERM.INI", dest, destlen);
}

/*
 *	UILanguageFileのフルパスを取得する
 *
 *	@param[in]		HomeDir					exe,dllの存在するフォルダ GetHomeDir()で取得できる
 *	@param[in]		UILanguageFileRel		lngファイル、HomeDirからの相対パス
 *	@param[in,out]	UILanguageFileFull		lngファイルptr、フルパス
 *	@param[in]		UILanguageFileFullLen	lngファイルlen、フルパス
 */
void GetUILanguageFileFull(const char *HomeDir, const char *UILanguageFileRel,
						   char *UILanguageFileFull, size_t UILanguageFileFullLen)
{
	char CurDir[MAX_PATH];

	/* Get UILanguageFile Full Path */
	GetCurrentDirectory(sizeof(CurDir), CurDir);
	SetCurrentDirectory(HomeDir);
	_fullpath(UILanguageFileFull, UILanguageFileRel, UILanguageFileFullLen);
	SetCurrentDirectory(CurDir);
}

void GetUILanguageFile(char *buf, int buflen)
{
	char HomeDir[MAX_PATH];
	char Temp[MAX_PATH];
	char SetupFName[MAX_PATH];

	/* Get home directory */
	GetHomeDir(NULL, HomeDir, sizeof(HomeDir));

	/* Get SetupFName */
	GetDefaultSetupFName(HomeDir, SetupFName, sizeof(SetupFName));

	/* Get LanguageFile name */
	GetPrivateProfileString("Tera Term", "UILanguageFile", "lang\\Default.lng",
	                        Temp, sizeof(Temp), SetupFName);

	GetUILanguageFileFull(HomeDir, Temp, buf, buflen);
}

// 指定したエントリを teraterm.ini から読み取る (2009.3.23 yutaka)
void GetOnOffEntryInifile(char *entry, char *buf, int buflen)
{
	char HomeDir[MAX_PATH];
	char Temp[MAX_PATH];
	char SetupFName[MAX_PATH];

	/* Get home directory */
	GetHomeDir(NULL, HomeDir, sizeof(HomeDir));

	/* Get SetupFName */
	GetDefaultSetupFName(HomeDir, SetupFName, sizeof(SetupFName));
	
	/* Get LanguageFile name */
	GetPrivateProfileString("Tera Term", entry, "off",
	                        Temp, sizeof(Temp), SetupFName);

	strncpy_s(buf, buflen, Temp, _TRUNCATE);
}

void get_lang_msgW(const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile)
{
	GetI18nStrW("Tera Term", key, buf, buf_len, def, iniFile);
}

void get_lang_msg(const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile)
{
	GetI18nStr("Tera Term", key, buf, buf_len, def, iniFile);
}

int get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, const char *iniFile)
{
	if (GetI18nLogfont("Tera Term", key, logfont,
	                   GetDeviceCaps(GetDC(dlg),LOGPIXELSY),
	                   iniFile) == FALSE) {
		return FALSE;
	}

	if ((*font = CreateFontIndirect(logfont)) == NULL) {
		return FALSE;
	}

	return TRUE;
}

//
// cf. http://homepage2.nifty.com/DSS/VCPP/API/SHBrowseForFolder.htm
//
int CALLBACK setDefaultFolder(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if(uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	}
	return 0;
}

BOOL doSelectFolder(HWND hWnd, char *path, int pathlen, const char *def, const char *msg)
{
	BROWSEINFOA     bi;
	LPITEMIDLIST    pidlRoot;      // ブラウズのルートPIDL
	LPITEMIDLIST    pidlBrowse;    // ユーザーが選択したPIDL
	char buf[MAX_PATH];
	BOOL ret = FALSE;

	// ダイアログ表示時のルートフォルダのPIDLを取得
	// ※以下はデスクトップをルートとしている。デスクトップをルートとする
	//   場合は、単に bi.pidlRoot に０を設定するだけでもよい。その他の特
	//   殊フォルダをルートとする事もできる。詳細はSHGetSpecialFolderLoca
	//   tionのヘルプを参照の事。
	if (!SUCCEEDED(SHGetSpecialFolderLocation(hWnd, CSIDL_DESKTOP, &pidlRoot))) {
		return FALSE;
	}

	// BROWSEINFO構造体の初期値設定
	// ※BROWSEINFO構造体の各メンバの詳細説明もヘルプを参照
	bi.hwndOwner = hWnd;
	bi.pidlRoot = pidlRoot;
	bi.pszDisplayName = buf;
	bi.lpszTitle = msg;
	bi.ulFlags = 0;
	bi.lpfn = setDefaultFolder;
	bi.lParam = (LPARAM)def;
	// フォルダ選択ダイアログの表示 
	pidlBrowse = SHBrowseForFolder(&bi);
	if (pidlBrowse != NULL) {  
		// PIDL形式の戻り値のファイルシステムのパスに変換
		if (SHGetPathFromIDList(pidlBrowse, buf)) {
			// 取得成功
			strncpy_s(path, pathlen, buf, _TRUNCATE);
			ret = TRUE;
		}
		// SHBrowseForFolderの戻り値PIDLを解放
		CoTaskMemFree(pidlBrowse);
	}
	// クリーンアップ処理
	CoTaskMemFree(pidlRoot);

	return ret;
}

void OutputDebugPrintf(const char *fmt, ...)
{
	char tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringA(tmp);
}

#if UNICODE_API // defined(UNICODE)
void OutputDebugPrintfW(const wchar_t *fmt, ...)
{
	wchar_t tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnwprintf_s(tmp, _countof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringW(tmp);
}
#endif

#if (_MSC_VER < 1800)
BOOL vercmp(
	DWORD cond_val,
	DWORD act_val,
	DWORD dwTypeMask)
{
	switch (dwTypeMask) {
	case VER_EQUAL:
		if (act_val == cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER:
		if (act_val > cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER_EQUAL:
		if (act_val >= cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS:
		if (act_val < cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS_EQUAL:
		if (act_val <= cond_val) {
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/*
DWORDLONG dwlConditionMask
| 000 | 000 | 000 | 000 | 000 | 000 | 000 | 000 |
   |     |     |     |     |     |     |     +- condition of dwMinorVersion
   |     |     |     |     |     |     +------- condition of dwMajorVersion
   |     |     |     |     |     +------------- condition of dwBuildNumber
   |     |     |     |     +------------------- condition of dwPlatformId
   |     |     |     +------------------------- condition of wServicePackMinor
   |     |     +------------------------------- condition of wServicePackMajor
   |     +------------------------------------- condition of wSuiteMask
   +------------------------------------------- condition of wProductType
*/
BOOL _myVerifyVersionInfo(
	LPOSVERSIONINFOEX lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
	OSVERSIONINFO osvi;
	WORD cond;
	BOOL ret, check_next;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	if (dwTypeMask & VER_BUILDNUMBER) {
		cond = (WORD)((dwlConditionMask >> (2*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwBuildNumber, osvi.dwBuildNumber, cond)) {
			return FALSE;
		}
	}
	if (dwTypeMask & VER_PLATFORMID) {
		cond = (WORD)((dwlConditionMask >> (3*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwPlatformId, osvi.dwPlatformId, cond)) {
			return FALSE;
		}
	}
	ret = TRUE;
	if (dwTypeMask & (VER_MAJORVERSION | VER_MINORVERSION)) {
		check_next = TRUE;
		if (dwTypeMask & VER_MAJORVERSION) {
			cond = (WORD)((dwlConditionMask >> (1*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond);
				// ret: result of major version
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, VER_EQUAL)) {
					// !vercmp(...: result of GRATOR/LESS than (not "GRATOR/LESS than or equal to") of major version
					// e.g.
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					check_next = FALSE;
				}
			}
		}
		if (check_next && (dwTypeMask & VER_MINORVERSION)) {
			cond = (WORD)((dwlConditionMask >> (0*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond);
			}
		}
	}
	return ret;
}
#endif

BOOL myVerifyVersionInfo(
	LPOSVERSIONINFOEX lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
#if (_MSC_VER >= 1800)
	return VerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask);
#else
	return _myVerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask);
#endif
}

ULONGLONG _myVerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	ULONGLONG result, mask;
	BYTE op = dwConditionMask & 0x07;

	switch (dwTypeBitMask) {
		case VER_MINORVERSION:
			mask = 0x07 << (0 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (0 * 3);
			break;
		case VER_MAJORVERSION:
			mask = 0x07 << (1 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (1 * 3);
			break;
		case VER_BUILDNUMBER:
			mask = 0x07 << (2 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (2 * 3);
			break;
		case VER_PLATFORMID:
			mask = 0x07 << (3 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (3 * 3);
			break;
		case VER_SERVICEPACKMINOR:
			mask = 0x07 << (4 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (4 * 3);
			break;
		case VER_SERVICEPACKMAJOR:
			mask = 0x07 << (5 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (5 * 3);
			break;
		case VER_SUITENAME:
			mask = 0x07 << (6 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (6 * 3);
			break;
		case VER_PRODUCT_TYPE:
			mask = 0x07 << (7 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (7 * 3);
			break;
	}

	return result;
}

ULONGLONG myVerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	typedef ULONGLONG(WINAPI *func)(ULONGLONG, DWORD, BYTE);
	static HMODULE hmodKernel32 = NULL;
	static func pVerSetConditionMask = NULL;
	char kernel32_dll[MAX_PATH];

	GetSystemDirectory(kernel32_dll, sizeof(kernel32_dll));
	strncat_s(kernel32_dll, sizeof(kernel32_dll), "\\kernel32.dll", _TRUNCATE);
	if (hmodKernel32 == NULL) {
		hmodKernel32 = LoadLibrary(kernel32_dll);
		if (hmodKernel32 != NULL) {
			pVerSetConditionMask = (func)GetProcAddress(hmodKernel32, "VerSetConditionMask");
		}
	}

	if (pVerSetConditionMask == NULL) {
		return _myVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
	}

	return pVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
}

// OSが 指定されたバージョンと等しい かどうかを判別する。
BOOL IsWindowsVer(DWORD dwPlatformId, DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = dwPlatformId;
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = myVerifyVersionInfo(&osvi, VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OSが 指定されたバージョン以降 かどうかを判別する。
//   dwPlatformId を見ていないので NT カーネル内でしか比較できない
//   5.0 以上で比較すること
BOOL IsWindowsVerOrLater(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = myVerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OSが WindowsNT カーネルかどうかを判別する。
//
// return TRUE:  NT kernel
//        FALSE: Not NT kernel
BOOL IsWindowsNTKernel()
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	ret = myVerifyVersionInfo(&osvi, VER_PLATFORMID, dwlConditionMask);
	return (ret);
}

// OSが Windows95 かどうかを判別する。
BOOL IsWindows95()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_WINDOWS, 4, 0);
}

// OSが WindowsMe かどうかを判別する。
BOOL IsWindowsMe()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_WINDOWS, 4, 90);
}

// OSが WindowsNT4.0 かどうかを判別する。
BOOL IsWindowsNT4()
{
	// VS2013以上だと GetVersionEx() が警告となるため、VerifyVersionInfo() を使う。
	// しかし、VS2013でビルドしたプログラムは、そもそも NT4.0 では動作しないため、
	// 無条件に FALSE を返してもよいかもしれない。
	return IsWindowsVer(VER_PLATFORM_WIN32_NT, 4, 0);
}

// OSが Windows2000 かどうかを判別する。
BOOL IsWindows2000()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_NT, 5, 0);
}

// OSが Windows2000 以降 かどうかを判別する。
//
// return TRUE:  2000 or later
//        FALSE: NT4 or earlier
BOOL IsWindows2000OrLater(void)
{
	return IsWindowsVerOrLater(5, 0);
}

// OSが WindowsVista 以降 かどうかを判別する。
//
// return TRUE:  Vista or later
//        FALSE: XP or earlier
BOOL IsWindowsVistaOrLater(void)
{
	return IsWindowsVerOrLater(6, 0);
}

// OSが Windows7 以降 かどうかを判別する。
//
// return TRUE:  7 or later
//        FALSE: Vista or earlier
BOOL IsWindows7OrLater(void)
{
	return IsWindowsVerOrLater(6, 1);
}

// OS がマルチモニタ API をサポートしているかどうかを判別する。
//   98 以降/2000 以降は TRUE を返す
BOOL HasMultiMonitorSupport()
{
	HMODULE mod;

	if (((mod = GetModuleHandle("user32.dll")) != NULL) &&
	    (GetProcAddress(mod, "MonitorFromPoint") != NULL)) {
		return TRUE;
	}
	return FALSE;
}

// OS が GetAdaptersAddresses をサポートしているかどうかを判別する。
//   XP 以降は TRUE を返す
//   2000 以降は IPv6 に対応しているが GetAdaptersAddresses がない
BOOL HasGetAdaptersAddresses()
{
	HMODULE mod;

	if (((mod = GetModuleHandle("iphlpapi.dll")) != NULL) &&
		(GetProcAddress(mod, "GetAdaptersAddresses") != NULL)) {
		return TRUE;
	}
	return FALSE;
}

// OS が DnsQuery をサポートしているかどうかを判別する。
//   2000 以降は TRUE を返す
BOOL HasDnsQuery()
{
	HMODULE mod;

	if (((mod = GetModuleHandle("Dnsapi.dll")) != NULL) &&
		(GetProcAddress(mod, "DnsQuery") != NULL)) {
		return TRUE;
	}
	return FALSE;
}

// 通知アイコンでのバルーンチップに対応しているか判別する。
// Me/2000 以降で TRUE を返す
BOOL HasBalloonTipSupport()
{
	return IsWindows2000OrLater() || IsWindowsMe();
}

// OPENFILENAMEA.lStructSize に代入する値
DWORD get_OPENFILENAME_SIZE()
{
	if (IsWindows2000OrLater()) {
		return sizeof(OPENFILENAMEA);
	}
	return OPENFILENAME_SIZE_VERSION_400A;
}

// convert table for KanjiCodeID and ListID
// cf. KanjiList,KanjiListSend
//     KoreanList,KoreanListSend
//     Utf8List,Utf8ListSend
//     IdSJIS, IdEUC, IdJIS, IdUTF8, IdUTF8m
//     IdEnglish, IdJapanese, IdRussian, IdKorean, IdUtf8
/* KanjiCode2List(Language,KanjiCodeID) returns ListID */
int KanjiCode2List(int lang, int kcode)
{
	int Table[5][5] = {
		{1, 2, 3, 4, 5}, /* English (dummy) */
		{1, 2, 3, 4, 5}, /* Japanese(dummy) */
		{1, 2, 3, 4, 5}, /* Russian (dummy) */
		{1, 1, 1, 2, 3}, /* Korean */
		{1, 1, 1, 1, 2}, /* Utf8 */
	};
	lang--;
	kcode--;
	return Table[lang][kcode];
}
/* List2KanjiCode(Language,ListID) returns KanjiCodeID */
int List2KanjiCode(int lang, int list)
{
	int Table[5][5] = {
		{1, 2, 3, 4, 5}, /* English (dummy) */
		{1, 2, 3, 4, 5}, /* Japanese(dummy) */
		{1, 2, 3, 4, 5}, /* Russian (dummy) */
		{1, 4, 5, 1, 1}, /* Korean */
		{4, 5, 4, 4, 4}, /* Utf8 */
	};
	lang--;
	list--;
	if (list < 0) {
		list = 0;
	}
	return Table[lang][list];
}
/* KanjiCodeTranslate(Language(dest), KanjiCodeID(source)) returns KanjiCodeID */
int KanjiCodeTranslate(int lang, int kcode)
{
	int Table[5][5] = {
		{1, 2, 3, 4, 5}, /* to English (dummy) */
		{1, 2, 3, 4, 5}, /* to Japanese(dummy) */
		{1, 2, 3, 4, 5}, /* to Russian (dummy) */
		{1, 1, 1, 4, 5}, /* to Korean */
		{4, 4, 4, 4, 5}, /* to Utf8 */
	};
	lang--;
	kcode--;
	return Table[lang][kcode];
}

char *mctimelocal(char *format, BOOL utc_flag)
{
	SYSTEMTIME systime;
	static char strtime[29];
	char week[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char tmp[5];
	unsigned int i = strlen(format);

	if (utc_flag) {
		GetSystemTime(&systime);
	}
	else {
		GetLocalTime(&systime);
	}
	memset(strtime, 0, sizeof(strtime));
	for (i=0; i<strlen(format); i++) {
		if (format[i] == '%') {
			char c = format[i + 1];
			switch (c) {
				case 'a':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s", week[systime.wDayOfWeek]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'b':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s", month[systime.wMonth - 1]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'd':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wDay);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'e':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%2d", systime.wDay);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'H':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wHour);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'N':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%03d", systime.wMilliseconds);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'm':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wMonth);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'M':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wMinute);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'S':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wSecond);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'w':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%d", systime.wDayOfWeek);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'Y':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%04d", systime.wYear);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case '%':
					strncat_s(strtime, sizeof(strtime), "%", _TRUNCATE);
					i++;
					break;
				default:
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", format[i]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					break;
			}
		}
		else {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", format[i]);
			strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
		}
	}

	return strtime;
}

char *strelapsed(DWORD start_time)
{
	static char strtime[20];
	int days, hours, minutes, seconds, msecs;
	DWORD delta = GetTickCount() - start_time;

	msecs = delta % 1000;
	delta /= 1000;

	seconds = delta % 60;
	delta /= 60;

	minutes = delta % 60;
	delta /= 60;

	hours = delta % 24;
	days = delta / 24;

	_snprintf_s(strtime, sizeof(strtime), _TRUNCATE,
		"%d %02d:%02d:%02d.%04d",
		days, hours, minutes, seconds, msecs);

	return strtime;
}

PCHAR PASCAL GetParam(PCHAR buff, int size, PCHAR param)
{
	int i = 0;
	BOOL quoted = FALSE;

	while (*param == ' ' || *param == '\t') {
		param++;
	}

	if (*param == '\0' || *param == ';') {
		return NULL;
	}

	while (*param != '\0' && (quoted || (*param != ';' && *param != ' ' && *param != '\t'))) {
		if (*param == '"') {
			if (*(param+1) != '"') {
				quoted = !quoted;
			}
			else {
				if (i < size - 1) {
					buff[i++] = *param;
				}
				param++;
			}
		}
		if (i < size - 1) {
			buff[i++] = *param;
		}
		param++;
	}
	if (!quoted && (buff[i-1] == ';')) {
		i--;
	}

	buff[i] = '\0';
	return (param);
}

void PASCAL DequoteParam(PCHAR dest, int dest_len, PCHAR src)
{
	BOOL quoted = FALSE;
	PCHAR dest_end = dest + dest_len - 1;

	if (src == dest) {
		while (*src != '\0' && *src != '"' && dest < dest_end) {
			src++; dest++;
		}
	}

	while (*src != '\0' && dest < dest_end) {
		if (*src != '"' || (*++src == '"' && quoted)) {
			*dest++ = *src++;
		}
		else {
			quoted = !quoted;
		}
	}

	*dest = '\0';
}

void PASCAL DeleteComment(PCHAR dest, int dest_size, PCHAR src)
{
	BOOL quoted = FALSE;
	PCHAR dest_end = dest + dest_size - 1;

	while (*src != '\0' && dest < dest_end && (quoted || *src != ';')) {
		*dest++ = *src;

		if (*src++ == '"') {
			if (*src == '"' && dest < dest_end) {
				*dest++ = *src++;
			}
			else {
				quoted = !quoted;
			}
		}
	}

	*dest = '\0';

	return;
}

void split_buffer(char *buffer, int delimiter, char **head, char **body)
{
	char *p1, *p2;

	*head = *body = NULL;

	if (!isalnum(*buffer) || (p1 = strchr(buffer, delimiter)) == NULL) {
		return;
	}

	*head = buffer;

	p2 = buffer;
	while (p2 < p1 && !isspace(*p2)) {
		p2++;
	}

	*p2 = '\0';

	p1++;
	while (*p1 && isspace(*p1)) {
		p1++;
	}

	*body = p1;
}

/**
 *	ウィンドウ上の位置を取得する
 *	@Param[in]		hWnd
 *	@Param[in]		point		位置(x,y)
 *	@Param[in,out]	InWindow	ウィンドウ上
 *	@Param[in,out]	InClient	クライアント領域上
 *	@Param[in,out]	InTitleBar	タイトルバー上
 *	@retval			FALSE		無効なhWnd
 */
BOOL GetPositionOnWindow(
	HWND hWnd, const POINT *point,
	BOOL *InWindow, BOOL *InClient, BOOL *InTitleBar)
{
	const int x = point->x;
	const int y = point->y;
	RECT winRect;
	RECT clientRect;

	if (InWindow != NULL) *InWindow = FALSE;
	if (InClient != NULL) *InClient = FALSE;
	if (InTitleBar != NULL) *InTitleBar = FALSE;

	if (!GetWindowRect(hWnd, &winRect)) {
		return FALSE;
	}

	if ((x < winRect.left) || (winRect.right < x) ||
		(y < winRect.top) || (winRect.bottom < y))
	{
		return TRUE;
	}
	if (InWindow != NULL) *InWindow = TRUE;

	{
		POINT pos;
		GetClientRect(hWnd, &clientRect);
		pos.x = clientRect.left;
		pos.y = clientRect.top;
		ClientToScreen(hWnd, &pos);
		clientRect.left = pos.x;
		clientRect.top = pos.y;

		pos.x = clientRect.right;
		pos.y = clientRect.bottom;
		ClientToScreen(hWnd, &pos);
		clientRect.right = pos.x;
		clientRect.bottom = pos.y;
	}

	if ((clientRect.left <= x) && (x < clientRect.right) &&
		(clientRect.top <= y) && (y < clientRect.bottom))
	{
		if (InClient != NULL) *InClient = TRUE;
		if (InTitleBar != NULL) *InTitleBar = FALSE;
		return TRUE;
	}
	if (InClient != NULL) *InClient = FALSE;

	if (InTitleBar != NULL) {
		*InTitleBar = (y < clientRect.top) ? TRUE : FALSE;
	}

	return TRUE;
}

int SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	return SetI18DlgStrs("Tera Term", hDlgWnd, infos, infoCount, UILanguageFile);
}

void SetDlgMenuTexts(HMENU hMenu, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	SetI18MenuStrs("Tera Term", hMenu, infos, infoCount, UILanguageFile);
}

/**
 *	ダイアログフォントを取得する
 *	エラーは発生しない
 */
void GetMessageboxFont(LOGFONT *logfont)
{
	NONCLIENTMETRICS nci;
	const int st_size = CCSIZEOF_STRUCT(NONCLIENTMETRICS, lfMessageFont);
	BOOL r;

	memset(&nci, 0, sizeof(nci));
	nci.cbSize = st_size;
	r = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, st_size, &nci, 0);
	assert(r == TRUE);
	*logfont = nci.lfStatusFont;
}

/**
 *	ウィンドウ表示されているディスプレイのデスクトップの範囲を取得する
 *	@param[in]		hWnd	ウィンドウのハンドル
 *	@param[out]		rect	デスクトップ
 */
void GetDesktopRect(HWND hWnd, RECT *rect)
{
	if (HasMultiMonitorSupport()) {
		// マルチモニタがサポートされている場合
		MONITORINFO monitorInfo;
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &monitorInfo);
		*rect = monitorInfo.rcWork;
	} else {
		// マルチモニタがサポートされていない場合
		SystemParametersInfo(SPI_GETWORKAREA, 0, rect, 0);
	}
}

/**
 *	ウィンドウをディスプレイからはみ出さないように移動する
 *	はみ出ていない場合は移動しない
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 */
void MoveWindowToDisplay(HWND hWnd)
{
	RECT desktop;
	RECT win_rect;
	int win_width;
	int win_height;
	int win_x;
	int win_y;
	BOOL modify = FALSE;

	GetDesktopRect(hWnd, &desktop);

	GetWindowRect(hWnd, &win_rect);
	win_x = win_rect.left;
	win_y = win_rect.top;
	win_height = win_rect.bottom - win_rect.top;
	win_width = win_rect.right - win_rect.left;
	if (win_y < desktop.top) {
		win_y = desktop.top;
		modify = TRUE;
	}
	else if (win_y + win_height > desktop.bottom) {
		win_y = desktop.bottom - win_height;
		modify = TRUE;
	}
	if (win_x < desktop.left) {
		win_x = desktop.left;
		modify = TRUE;
	}
	else if (win_x + win_width > desktop.right) {
		win_x = desktop.right - win_width;
		modify = TRUE;
	}

	if (modify) {
		SetWindowPos(hWnd, NULL, win_x, win_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

/**
 *	ウィンドウをディスプレイの中央に配置する
 *
 *	@param[in]	hWnd		位置を調整するウィンドウ
 *	@param[in]	hWndParent	このウィンドウの中央に移動する
 *							(NULLの場合ディスプレイの中央)
 *
 *	hWndParentの指定がある場合
 *		hWndParentが表示状態の場合
 *			- hWndParentの中央に配置
 *			- ただし表示されているディスプレイからはみ出す場合は調整される
 *		hWndParentが非表示状態の場合
 *			- hWndが表示されているディスプレイの中央に配置される
 *	hWndParentがNULLの場合
 *		- hWndが表示されているディスプレイの中央に配置される
 */
void CenterWindow(HWND hWnd, HWND hWndParent)
{
	RECT rcWnd;
	LONG WndWidth;
	LONG WndHeight;
	int NewX;
	int NewY;
	RECT rcDesktop;
	BOOL r;

	r = GetWindowRect(hWnd, &rcWnd);
	assert(r != FALSE); (void)r;
	WndWidth = rcWnd.right - rcWnd.left;
	WndHeight = rcWnd.bottom - rcWnd.top;

	if (hWndParent == NULL || !IsWindowVisible(hWndParent) || IsIconic(hWndParent)) {
		// 親が設定されていない or 表示されていない or icon化されている 場合
		// ウィンドウの表示されているディスプレイの中央に表示する
		GetDesktopRect(hWnd, &rcDesktop);

		// デスクトップ(表示されているディスプレイ)の中央
		NewX = (rcDesktop.left + rcDesktop.right) / 2 - WndWidth / 2;
		NewY = (rcDesktop.top + rcDesktop.bottom) / 2 - WndHeight / 2;
	} else {
		RECT rcParent;
		r = GetWindowRect(hWndParent, &rcParent);
		assert(r != FALSE); (void)r;

		// hWndParentの中央
		NewX = (rcParent.left + rcParent.right) / 2 - WndWidth / 2;
		NewY = (rcParent.top + rcParent.bottom) / 2 - WndHeight / 2;

		GetDesktopRect(hWndParent, &rcDesktop);
	}

	// デスクトップからはみ出す場合、調整する
	if (NewX + WndWidth > rcDesktop.right)
		NewX = rcDesktop.right - WndWidth;
	if (NewX < rcDesktop.left)
		NewX = rcDesktop.left;

	if (NewY + WndHeight > rcDesktop.bottom)
		NewY = rcDesktop.bottom - WndHeight;
	if (NewY < rcDesktop.top)
		NewY = rcDesktop.top;

	// 移動する
	SetWindowPos(hWnd, NULL, NewX, NewY, 0, 0,
				 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 *	hWndの表示されているモニタのDPIを取得する
 *	Per-monitor DPI awareness対応
 *
 *	@retval	DPI値(通常のDPIは96)
 */
int GetMonitorDpiFromWindow(HWND hWnd)
{
	static HRESULT (__stdcall  *pGetDpiForMonitor)(HMONITOR hmonitor, int/*enum MONITOR_DPI_TYPE*/ dpiType, UINT *dpiX, UINT *dpiY);
	static HMODULE hDll;
	if (hDll == NULL) {
		hDll = LoadLibraryA("Shcore.dll");
		if (hDll != NULL) {
			pGetDpiForMonitor = (void *)GetProcAddress(hDll, "GetDpiForMonitor");
		}
	}
	if (pGetDpiForMonitor == NULL) {
		// ダイアログ内では自動スケーリングが効いているので
		// 常に96を返すようだ
		int dpiY;
		HDC hDC = GetDC(hWnd);
		dpiY = GetDeviceCaps(hDC,LOGPIXELSY);
		ReleaseDC(hWnd, hDC);
		return dpiY;
	} else {
		UINT dpiX;
		UINT dpiY;
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		pGetDpiForMonitor(hMonitor, 0 /*0=MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY);
		return (int)dpiY;
	}
}
