/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* misc. routines  */
#include "teraterm.h"
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "tttypes.h"
#include <shlobj.h>

// for _ismbblead
#include <mbctype.h>

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
	if (Ptr[0]=='\\')
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
void FitFileName(PCHAR FileName, int destlen, PCHAR DefExt)
{
	int i, j, NumOfDots;
	char Temp[MAXPATHLEN];
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
		    (j < MAXPATHLEN-1)) {
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
	struct _stat st;

	return (_stat(FName,&st)==0);
}

long GetFSize(PCHAR FName)
{
	struct _stat st;

	if (_stat(FName,&st)==-1) {
		return 0;
	}
	return (long)st.st_size;
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

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			i++;
			continue;
		}
		switch (FName[i]) {
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				return 1;
		}
	}
	return 0;
}

// ファイル名に使用できない文字を削除する (2006.8.28 maya)
void deleteInvalidFileNameChar(PCHAR FName)
{
	int i, j=0, len;

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			FName[j++] = FName[i];
			FName[j++] = FName[++i];
			continue;
		}
		switch (FName[i]) {
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				break;
			default:
				FName[j] = FName[i];
				j++;
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
	char buf[80];
	char *c;
	time_t time_local;
	struct tm *tm_local;

	// ファイル名部分のみを flename に格納
	ExtractFileName(FName, filename ,sizeof(filename));

	// strftime に使用できない文字を削除
	deleteInvalidStrftimeChar(filename);

	// 現在時刻を取得
	time(&time_local);
	tm_local = localtime(&time_local);

	// 時刻文字列に変換
	if (strftime(buf, sizeof(buf), filename, tm_local) == 0) {
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

void GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest)
{
	int i, j, k;
	char c;

	i = 1;
	j = 0;
	k = 0;
	do {
		c = Source[j];
		if ( c==',' ) {
			i++;
		}
		j++;
		if ( (i==Nth) && (c!=',') && (k<Size-1) ) {
			Dest[k] = c;
			k++;
		}
	}
	while ((i<=Nth) && (c!=0));
	Dest[k] = 0;
}

void GetNthNum(PCHAR Source, int Nth, int far *Num)
{
	char T[15];

	GetNthString(Source,Nth,sizeof(T),T);
	if (sscanf(T, "%d", Num) != 1) {
		*Num = 0;
	}
}

void WINAPI GetDefaultFName(char *home, char *file, char *dest, int destlen)
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
	if (GetFileAttributes(MyDocSetupFName) != -1) {
		strncpy_s(dest, destlen, MyDocSetupFName, _TRUNCATE);
		return;
	}

homedir:
	strncpy_s(dest, destlen, home, _TRUNCATE);
	AppendSlash(dest,destlen);
	strncat_s(dest, destlen, file, _TRUNCATE);
}

// デフォルトの TERATERM.INI のフルパスを ttpmacro からも
// 取得するために追加した。(2007.2.18 maya)
void GetDefaultSetupFName(char *home, char *dest, int destlen)
{
	GetDefaultFName(home, "TERATERM.INI", dest, destlen);
}

void GetUILanguageFile(char *buf, int buflen)
{
	char HomeDir[MAX_PATH];
	char Temp[MAX_PATH];
	char SetupFName[MAX_PATH];
	char CurDir[MAX_PATH];

	/* Get home directory */
	GetModuleFileName(NULL,Temp,sizeof(Temp));
	ExtractDirName(Temp, HomeDir);

	/* Get SetupFName */
	GetDefaultSetupFName(HomeDir, SetupFName, sizeof(SetupFName));
	
	/* Get LanguageFile name */
	GetPrivateProfileString("Tera Term", "UILanguageFile", "",
	                        Temp, sizeof(Temp), SetupFName);

	GetCurrentDirectory(sizeof(CurDir), CurDir);
	SetCurrentDirectory(HomeDir);
	_fullpath(buf, Temp, buflen);
	SetCurrentDirectory(CurDir);
}

// 指定したエントリを teraterm.ini から読み取る (2009.3.23 yutaka)
void GetOnOffEntryInifile(char *entry, char *buf, int buflen)
{
	char HomeDir[MAX_PATH];
	char Temp[MAX_PATH];
	char SetupFName[MAX_PATH];

	/* Get home directory */
	GetModuleFileName(NULL,Temp,sizeof(Temp));
	ExtractDirName(Temp, HomeDir);

	/* Get SetupFName */
	GetDefaultSetupFName(HomeDir, SetupFName, sizeof(SetupFName));
	
	/* Get LanguageFile name */
	GetPrivateProfileString("Tera Term", entry, "off",
	                        Temp, sizeof(Temp), SetupFName);

	strncpy_s(buf, buflen, Temp, _TRUNCATE);
}

void get_lang_msg(PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile)
{
	GetI18nStr("Tera Term", key, buf, buf_len, def, iniFile);
}

int get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PCHAR iniFile)
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

void doSelectFolder(HWND hWnd, char *path, int pathlen, char *msg)
{
	BROWSEINFO      bi;
	LPITEMIDLIST    pidlRoot;      // ブラウズのルートPIDL
	LPITEMIDLIST    pidlBrowse;    // ユーザーが選択したPIDL
	char buf[MAX_PATH];

	// ダイアログ表示時のルートフォルダのPIDLを取得
	// ※以下はデスクトップをルートとしている。デスクトップをルートとする
	//   場合は、単に bi.pidlRoot に０を設定するだけでもよい。その他の特
	//   殊フォルダをルートとする事もできる。詳細はSHGetSpecialFolderLoca
	//   tionのヘルプを参照の事。
	if (!SUCCEEDED(SHGetSpecialFolderLocation(hWnd, CSIDL_DESKTOP, &pidlRoot))) {
			return;
	}

	// BROWSEINFO構造体の初期値設定
	// ※BROWSEINFO構造体の各メンバの詳細説明もヘルプを参照
	bi.hwndOwner = hWnd;
	bi.pidlRoot = pidlRoot;
	bi.pszDisplayName = buf;
	bi.lpszTitle = msg;
	bi.ulFlags = 0;
	bi.lpfn = setDefaultFolder;
	bi.lParam = (LPARAM)path;
	// フォルダ選択ダイアログの表示 
	pidlBrowse = SHBrowseForFolder(&bi);
	if (pidlBrowse != NULL) {  
		// PIDL形式の戻り値のファイルシステムのパスに変換
		if (SHGetPathFromIDList(pidlBrowse, buf)) {
			// 取得成功
			strncpy_s(path, pathlen, buf, _TRUNCATE);
		}
		// SHBrowseForFolderの戻り値PIDLを解放
		CoTaskMemFree(pidlBrowse);
	}
	// クリーンアップ処理
	CoTaskMemFree(pidlRoot);
}

void OutputDebugPrintf(char *fmt, ...) {
	char tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnprintf(tmp, sizeof(tmp), fmt, arg);
	OutputDebugString(tmp);
}

BOOL is_NT4()
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
	    osvi.dwMajorVersion == 4) {
		return TRUE;
	}
	return FALSE;
}

int get_OPENFILENAME_SIZE()
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
	    osvi.dwMajorVersion >= 5) {
		return sizeof(OPENFILENAME);
	}
	//return OPENFILENAME_SIZE_VERSION_400;
	return 76;
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
		{1, 1, 1, 2, 2}, /* Korean */
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
		{1, 4, 4, 1, 1}, /* Korean */
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
