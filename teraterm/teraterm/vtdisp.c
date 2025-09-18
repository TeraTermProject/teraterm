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

/* TERATERM.EXE, VT terminal display routines */
#include "teraterm.h"
#include "teraterml.h"
#include "tttypes.h"
#include <string.h>
#include <olectl.h>
#include <assert.h>
#include <stdio.h>

#include "ttwinman.h"
#include "ttime.h"
#include "ttdialog.h"
#include "ttcommon.h"
#include "compat_win.h"
#include "unicode_test.h"
#include "setting.h"
#include "codeconv.h"
#include "libsusieplugin.h"
#include "asprintf.h"
#include "inifile_com.h"
#include "win32helper.h"
#include "ttknownfolders.h" // for FOLDERID_Desktop
#include "ttlib.h"
#if ENABLE_GDIPLUS
#include "ttgdiplus.h"
#endif

#include "theme.h"
#include "vtdisp.h"

#define CurWidth 2

#include "defaultcolortable.c"

#if 1
#define _OutputDebugPrintf(...)  OutputDebugPrintf(__VA_ARGS__)
#else
#define _OutputDebugPrintf(...)  (void)0
#endif

int WinWidth, WinHeight;		// 画面に表示されている文字数
static BOOL Active = FALSE;
static BOOL CompletelyVisible;
HFONT VTFont[AttrFontMask+1];
int FontHeight, FontWidth;
int ScreenWidth, ScreenHeight;	// スクリーンサイズ = (セル数)*(Font幅or高さ) ≒ Client Area
BOOL AdjustSize;
BOOL DontChangeSize=FALSE;
static int CRTWidth, CRTHeight;
int CursorX, CursorY;
/* Virtual screen region */
static RECT VirtualScreen;

// --- scrolling status flags
int WinOrgX, WinOrgY;			// 現在の表示位置
int NewOrgX, NewOrgY;			// 更新後の表示位置

int NumOfLines, NumOfColumns;	// バッファリングしている文字数
int PageStart, BuffEnd;			// 表示しているバッファ内の位置

static BOOL CursorOnDBCS = FALSE;
static BOOL SaveWinSize = FALSE;
static int WinWidthOld, WinHeightOld;
static HBRUSH Background;
static BOOL FontReSizeEnableInit = TRUE;	// font_resize_enable の初期値
/*  TODO
 *	iniファイルの読み込みの後(DispEnableResizedFont()が呼ばれた後)
 *	初期化(InitDisp())が行われるため初期値を変数で持つ。
 *	1回しか行わない初期はもっと早く、
 *	iniファイル読み込みよりも前に行い、
 *	変数FontReSizeEnableInitを不要にする。
 */

/*
 *	ANSI color table
 *		0		黒,Black
 *		1-6		少し暗い色(Red, Green, Yellow, Blue, Magenta, Cyan)
 *		7		Gray (15より暗い,8より明るい)
 *		8		Gray (7より暗い,0より明るい)
 *		9-14	明るい色,原色 (Bright Red, Green, Yellow, Blue, Magenta, Cyan)
 *		15		白,White 255 (Bright White)
 *		16-255	DefaultColorTable[16-255]
 */
static COLORREF ANSIColor[256];

// caret variables
static int CaretStatus;
static BOOL CaretEnabled = TRUE;
BOOL IMEstat;				/* IME Status  TRUE=IME ON */
BOOL IMECompositionState;	/* 変換状態 TRUE=変換中 */

// ---- device context and status flags
static HDC VTDC = NULL; /* Device context for VT window */
static TCharAttr DCAttr;
static TCharAttr CurCharAttr;
static HFONT DCPrevFont;

TCharAttr DefCharAttr = {
  AttrDefault,
  AttrDefault,
  AttrDefault,
  AttrDefaultFG,
  AttrDefaultBG
};

// scrolling
static int ScrollCount = 0;
static int dScroll = 0;
static int SRegionTop;
static int SRegionBottom;

typedef struct _BGSrc
{
	HDC        hdc;
	BOOL       enable;
	BG_TYPE    type;
	BG_PATTERN pattern;
	BOOL       antiAlias;
	COLORREF   color;
	BYTE       alpha;
	int        width;
	int        height;
	wchar_t    *fileW;
} BGSrc;

static BGSrc BGDest;	// 背景画像用
static BGSrc BGSrc1;	// 壁紙(Windowsのデスクトップ背景)用
static BGSrc BGSrc2;	// fill color用

static int  BGEnable;
static BYTE BGReverseTextAlpha;

static COLORREF BGVTColor[2];
static COLORREF BGVTBoldColor[2];
static COLORREF BGVTUnderlineColor[2];	// SGR 4
static COLORREF BGVTBlinkColor[2];
static COLORREF BGVTReverseColor[2];
static COLORREF BGURLColor[2];			// URL属性色

static RECT BGPrevRect;

static BOOL   BGInSizeMove;
//static HBRUSH BGBrushInSizeMove;

static HDC hdcBGWork;
static HDC hdcBGBuffer;
static HDC hdcBG;

typedef struct tagWallpaperInfo
{
	wchar_t *filename;
	int  pattern;
} WallpaperInfo;

static BOOL (WINAPI *BGAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);

typedef struct {
	BOOL bg_enable;
	BYTE alpha_vtback;
	BYTE alpha_back;
	BOOL debug_drawbox_text;	// 文字描画毎にboxを描画する
	//
	BYTE DCBackAlpha;
	COLORREF DCBackColor;

	BOOL font_resize_enable;
} vtdisp_work_t;
static vtdisp_work_t vtdisp_work;

static HBITMAP GetBitmapHandleW(const wchar_t *File);
static void InitColorTable(const COLORREF *ANSIColor16);
static void UpdateBGBrush(void);
static void GetDrawAttr(const TCharAttr *Attr, BOOL _reverse, COLORREF *fore_color, COLORREF *back_color, BYTE *_alpha);

// LoadImage() しか使えない環境かどうかを判別する。
// LoadImage()では .bmp 以外の画像ファイルが扱えないので要注意。
// (2014.4.20 yutaka)
static BOOL IsLoadImageOnlyEnabled(void)
{
	// Vista 未満の場合には、今まで通りの読み込みをするようにした
	// cf. SVN#4571(2011.8.4)
	return !IsWindowsVistaOrLater();
}

static HBITMAP CreateScreenCompatibleBitmap(int width,int height)
{
  HDC     hdc;
  HBITMAP hbm;

  _OutputDebugPrintf("CreateScreenCompatibleBitmap : width = %d height = %d\n",width,height);

  hdc = GetDC(NULL);

  hbm = CreateCompatibleBitmap(hdc,width,height);

  ReleaseDC(NULL,hdc);

  if(!hbm)
	  _OutputDebugPrintf("CreateScreenCompatibleBitmap : fail in CreateCompatibleBitmap\n");

  return hbm;
}

static HBITMAP CreateDIB24BPP(int width,int height,unsigned char **buf,int *lenBuf)
{
  HDC        hdc;
  HBITMAP    hbm;
  BITMAPINFO bmi;

  _OutputDebugPrintf("CreateDIB24BPP : width = %d height = %d\n",width,height);

  if(!width || !height)
    return NULL;

  ZeroMemory(&bmi,sizeof(bmi));

  *lenBuf = ((width * 3 + 3) & ~3) * height;

  bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth       = width;
  bmi.bmiHeader.biHeight      = height;
  bmi.bmiHeader.biPlanes      = 1;
  bmi.bmiHeader.biBitCount    = 24;
  bmi.bmiHeader.biSizeImage   = *lenBuf;
  bmi.bmiHeader.biCompression = BI_RGB;

  hdc = GetDC(NULL);

  hbm = CreateDIBSection(hdc,&bmi,DIB_RGB_COLORS,(void**)buf,NULL,0);

  ReleaseDC(NULL,hdc);

  return hbm;
}

static HDC  CreateBitmapDC(HBITMAP hbm)
{
  HDC hdc;

  _OutputDebugPrintf("CreateBitmapDC : hbm = %p\n",hbm);

  hdc = CreateCompatibleDC(NULL);

  SaveDC(hdc);
  SelectObject(hdc,hbm);

  return hdc;
}

static void DeleteBitmapDC(HDC *hdc)
{
  HBITMAP hbm;

  _OutputDebugPrintf("DeleteBitmapDC : *hdc = %p\n",hdc);

  if(!hdc)
    return;

  if(!(*hdc))
    return;

  hbm = GetCurrentObject(*hdc,OBJ_BITMAP);

  RestoreDC(*hdc,-1);
  DeleteObject(hbm);
  DeleteDC(*hdc);

  *hdc = 0;
}

static void FillBitmapDC(HDC hdc,COLORREF color)
{
  HBITMAP hbm;
  BITMAP  bm;
  RECT    rect;
  HBRUSH  hBrush;

  _OutputDebugPrintf("FillBitmapDC : hdc = %p color = %08lx\n",hdc,color);

  if(!hdc)
    return;

  hbm = GetCurrentObject(hdc,OBJ_BITMAP);
  GetObject(hbm,sizeof(bm),&bm);

  SetRect(&rect,0,0,bm.bmWidth,bm.bmHeight);
  hBrush = CreateSolidBrush(color);
  FillRect(hdc,&rect,hBrush);
  DeleteObject(hBrush);
}

static void DebugSaveFile(const wchar_t* fname, HDC hdc, int width, int height)
{
#if 1
	(void)fname;
	(void)hdc;
	(void)width;
	(void)height;
#else
	if (IsRelativePathW(fname)) {
		wchar_t *desktop;
		wchar_t *bmpfile;
		_SHGetKnownFolderPath(&FOLDERID_Desktop, KF_FLAG_CREATE, NULL, &desktop);
		bmpfile = NULL;
		awcscats(&bmpfile, desktop, L"\\", fname, NULL);
		free(desktop);
		SaveBmpFromHDC(bmpfile, hdc, width, height);
		free(bmpfile);
	}
	else {
		SaveBmpFromHDC(fname, hdc, width, height);
	}
#endif
}

/**
 *	BMP保存、デバグに使うかもしれないので残しておく
 */
#if 0
static BOOL SaveBitmapFile(const char *nameFile,unsigned char *pbuf,BITMAPINFO *pbmi)
{
  int    bmiSize;
  DWORD  writtenByte;
  HANDLE hFile;
  BITMAPFILEHEADER bfh;

  hFile = CreateFile(nameFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile == INVALID_HANDLE_VALUE)
    return FALSE;

  bmiSize = pbmi->bmiHeader.biSize;

  switch(pbmi->bmiHeader.biBitCount)
  {
    case 1:
      bmiSize += pbmi->bmiHeader.biClrUsed ? sizeof(RGBQUAD) * 2 : 0;
      break;

    case 2 :
      bmiSize += sizeof(RGBQUAD) * 4;
      break;

    case 4 :
      bmiSize += sizeof(RGBQUAD) * 16;
      break;

    case 8 :
      bmiSize += sizeof(RGBQUAD) * 256;
      break;
  }

  ZeroMemory(&bfh,sizeof(bfh));
  bfh.bfType    = MAKEWORD('B','M');
  bfh.bfOffBits = sizeof(bfh) + bmiSize;
  bfh.bfSize    = bfh.bfOffBits + pbmi->bmiHeader.biSizeImage;

  WriteFile(hFile,&bfh,sizeof(bfh)                ,&writtenByte,0);
  WriteFile(hFile,pbmi,bmiSize                    ,&writtenByte,0);
  WriteFile(hFile,pbuf,pbmi->bmiHeader.biSizeImage,&writtenByte,0);

  CloseHandle(hFile);

  return TRUE;
}
#endif

static HBITMAP CreateBitmapFromBITMAPINFO(const BITMAPINFO *pbmi, const unsigned char *pbuf)
{
	void* pvBits;
	HBITMAP hBmp = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

	if (pbuf != NULL) {
		memcpy(pvBits, pbuf, pbmi->bmiHeader.biSizeImage);
	}

	return hBmp;
}

static BOOL WINAPI AlphaBlendWithoutAPI(HDC hdcDest,int dx,int dy,int width,int height,HDC hdcSrc,int sx,int sy,int sw,int sh,BLENDFUNCTION bf)
{
  HDC hdcDestWork,hdcSrcWork;
  int i,invAlpha,alpha;
  int lenBuf;
  unsigned char *bufDest;
  unsigned char *bufSrc;

  if(dx != 0 || dy != 0 || sx != 0 || sy != 0 || width != sw || height != sh)
    return FALSE;

  hdcDestWork = CreateBitmapDC(CreateDIB24BPP(width,height,&bufDest,&lenBuf));
  hdcSrcWork  = CreateBitmapDC(CreateDIB24BPP(width,height,&bufSrc ,&lenBuf));

  if(!bufDest || !bufSrc)
    return FALSE;

  BitBlt(hdcDestWork,0,0,width,height,hdcDest,0,0,SRCCOPY);
  BitBlt(hdcSrcWork ,0,0,width,height,hdcSrc ,0,0,SRCCOPY);

  alpha = bf.SourceConstantAlpha;
  invAlpha = 255 - alpha;

  for(i = 0;i < lenBuf;i++,bufDest++,bufSrc++)
    *bufDest = (*bufDest * invAlpha + *bufSrc * alpha)>>8;

  BitBlt(hdcDest,0,0,width,height,hdcDestWork,0,0,SRCCOPY);

  DeleteBitmapDC(&hdcDestWork);
  DeleteBitmapDC(&hdcSrcWork);

  return TRUE;
}

// 画像読み込み関係
static void BGPreloadPicture(BGSrc *src)
{
	HBITMAP hbm = NULL;
	const wchar_t *load_file = src->fileW;
	const wchar_t *spi_path = ts.EtermLookfeel.BGSPIPathW;

	// Susie plugin で読み込み
	if (hbm == NULL) {
		HANDLE hbmi;
		HANDLE hbuf;
		BOOL r = SusieLoadPicture(load_file, spi_path, &hbmi, &hbuf);
		if (r != FALSE) {
			hbm = CreateBitmapFromBITMAPINFO(hbmi, hbuf);
			LocalFree(hbmi);
			LocalFree(hbuf);
		}
	}

	// GDI+ ライブラリを使って読み込む
#if ENABLE_GDIPLUS
	if (hbm == NULL) {
		hbm = GDIPLoad(load_file);
	}
#endif

	// OLE を利用して画像(jpeg)を読む
	//		LoadImage()のみ許可されている環境ではないとき
	if (hbm == NULL && !IsLoadImageOnlyEnabled()) {
		hbm = GetBitmapHandleW(load_file);
	}

	// LoadImageW() API で読み込む
	if (hbm == NULL) {
		// LoadImageW() APIは、
		// Windows 10 のとき高さがマイナスのbmpファイルはロードに失敗する
		// Windows 7 のときは成功する
		hbm = LoadImageW(0,load_file,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	}

	if(hbm) {
		BITMAP bm;

		GetObject(hbm,sizeof(bm),&bm);

		src->hdc    = CreateBitmapDC(hbm);
		src->width  = bm.bmWidth;
		src->height = bm.bmHeight;
	}else{
		src->type = BG_COLOR;
	}
}

static void BGGetWallpaperInfo(WallpaperInfo *wi)
{
	DWORD length;
	int style;
	int  tile;
	char str[256];
	HKEY hKey;

	wi->pattern = BG_CENTER;
	wi->filename = NULL;

	//レジストリキーのオープン
	if(RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return;

	//壁紙ファイル名ゲット
	hRegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (void **)&wi->filename, NULL);

	//壁紙スタイルゲット
	length = sizeof(str);
	RegQueryValueExA(hKey,"WallpaperStyle",NULL,NULL,(BYTE*)str,&length);
	style = atoi(str);

	//壁紙スタイルゲット
	length = sizeof(str);
	RegQueryValueExA(hKey,"TileWallpaper" ,NULL,NULL,(BYTE*)str,&length);
	tile = atoi(str);

	//レジストリキーのクローズ
	RegCloseKey(hKey);

	//これでいいの？
	if(tile)
		wi->pattern = BG_TILE;
	else {
		switch (style) {
		case 0: // Center(中央に表示)
			wi->pattern = BG_CENTER;
			break;
		case 2: // Stretch(画面に合わせて伸縮) アスペクト比は無視される
			wi->pattern = BG_STRETCH;
			break;
		case 10: // Fill(ページ横幅に合わせる) とあるが、和訳がおかしい
			// アスペクト比を維持して、はみ出してでも最大表示する
			wi->pattern = BG_AUTOFILL;
			break;
		case 6: // Fit(ページ縦幅に合わせる) とあるが、和訳がおかしい
			// アスペクト比を維持して、はみ出さないように最大表示する
			wi->pattern = BG_AUTOFIT;
			break;
		}
	}
}

/**
 *	OleLoadPicture() を使った画像読み込み
 * 	jpeg, bmp を読み込むことができる
 *	(Windowsによっては他の形式も読めるかもしれない)
 *
 */
// .bmp以外の画像ファイルを読む。
// 壁紙が .bmp 以外のファイルになっていた場合への対処。
// この関数は Windows 2000 未満の場合には呼んではいけない
// TODO:
//		IsLoadImageOnlyEnabled() は Vista 未満となっている
//
static HBITMAP GetBitmapHandleW(const wchar_t *File)
{
	OLE_HANDLE hOle = 0;
	IStream *iStream=NULL;
	IPicture *iPicture;
	HGLOBAL hMem;
	LPVOID pvData;
	DWORD nReadByte=0,nFileSize;
	HANDLE hFile;
	short type;
	HBITMAP hBitmap = NULL;
	HRESULT result;

	hFile=CreateFileW(File,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	nFileSize=GetFileSize(hFile,NULL);
	hMem=GlobalAlloc(GMEM_MOVEABLE,nFileSize);
	pvData=GlobalLock(hMem);

	ReadFile(hFile,pvData,nFileSize,&nReadByte,NULL);

	GlobalUnlock(hMem);
	CloseHandle(hFile);

	CreateStreamOnHGlobal(hMem,TRUE,&iStream);

	result = OleLoadPicture(iStream, nFileSize, FALSE, &IID_IPicture, (LPVOID *)&iPicture);
	if (result != S_OK || iPicture == NULL) {
		// 画像ファイルではない,対応した画像ファイル場合
		return NULL;
	}

	iStream->lpVtbl->Release(iStream);

	iPicture->lpVtbl->get_Type(iPicture,&type);
	if(type==PICTYPE_BITMAP){
		iPicture->lpVtbl->get_Handle(iPicture,&hOle);
	}

	hBitmap=(HBITMAP)(UINT_PTR)hOle;

	return hBitmap;
}

// 線形補完法により比較的鮮明にビットマップを拡大・縮小する。
// Windows 9x/NT対応
// cf.http://katahiromz.web.fc2.com/win32/bilinear.html
static HBITMAP CreateStretched32BppBitmapBilinear(HBITMAP hbm, INT cxNew, INT cyNew)
{
    INT ix, iy, x0, y0, x1, y1;
    DWORD x, y;
    BITMAP bm;
    HBITMAP hbmNew;
    HDC hdc;
    BITMAPINFO bi;
    BYTE *pbNewBits, *pbBits, *pbNewLine, *pbLine0, *pbLine1;
    DWORD wfactor, hfactor;
    DWORD ex0, ey0, ex1, ey1;
    DWORD r0, g0, b0, a0, r1, g1, b1, a1;
    DWORD c00, c01, c10, c11;
    LONG nWidthBytes, nWidthBytesNew;
    BOOL fAlpha;

    if (GetObject(hbm, sizeof(BITMAP), &bm) == 0)
        return NULL;

    hbmNew = NULL;
    hdc = CreateCompatibleDC(NULL);
    if (hdc != NULL)
    {
        nWidthBytes = bm.bmWidth * 4;
        ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = bm.bmWidth;
        bi.bmiHeader.biHeight = bm.bmHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        fAlpha = (bm.bmBitsPixel == 32);
        pbBits = (BYTE *)HeapAlloc(GetProcessHeap(), 0,
                                   nWidthBytes * bm.bmHeight);
        if (pbBits == NULL)
            return NULL;
        GetDIBits(hdc, hbm, 0, bm.bmHeight, pbBits, &bi, DIB_RGB_COLORS);
        bi.bmiHeader.biWidth = cxNew;
        bi.bmiHeader.biHeight = cyNew;
        hbmNew = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS,
                                  (VOID **)&pbNewBits, NULL, 0);
        if (hbmNew != NULL)
        {
            nWidthBytesNew = cxNew * 4;
            wfactor = (bm.bmWidth << 8) / cxNew;
            hfactor = (bm.bmHeight << 8) / cyNew;
            if (!fAlpha)
                a0 = 255;
            for(iy = 0; iy < cyNew; iy++)
            {
                y = hfactor * iy;
                y0 = y >> 8;
                y1 = min(y0 + 1, (INT)bm.bmHeight - 1);
                ey1 = y & 0xFF;
                ey0 = 0x100 - ey1;
                pbNewLine = pbNewBits + iy * nWidthBytesNew;
                pbLine0 = pbBits + y0 * nWidthBytes;
                pbLine1 = pbBits + y1 * nWidthBytes;
                for(ix = 0; ix < cxNew; ix++)
                {
                    x = wfactor * ix;
                    x0 = x >> 8;
                    x1 = min(x0 + 1, (INT)bm.bmWidth - 1);
                    ex1 = x & 0xFF;
                    ex0 = 0x100 - ex1;
                    c00 = ((LPDWORD)pbLine0)[x0];
                    c01 = ((LPDWORD)pbLine1)[x0];
                    c10 = ((LPDWORD)pbLine0)[x1];
                    c11 = ((LPDWORD)pbLine1)[x1];

                    b0 = ((ex0 * (c00 & 0xFF)) +
                          (ex1 * (c10 & 0xFF))) >> 8;
                    b1 = ((ex0 * (c01 & 0xFF)) +
                          (ex1 * (c11 & 0xFF))) >> 8;
                    g0 = ((ex0 * ((c00 >> 8) & 0xFF)) +
                          (ex1 * ((c10 >> 8) & 0xFF))) >> 8;
                    g1 = ((ex0 * ((c01 >> 8) & 0xFF)) +
                          (ex1 * ((c11 >> 8) & 0xFF))) >> 8;
                    r0 = ((ex0 * ((c00 >> 16) & 0xFF)) +
                          (ex1 * ((c10 >> 16) & 0xFF))) >> 8;
                    r1 = ((ex0 * ((c01 >> 16) & 0xFF)) +
                          (ex1 * ((c11 >> 16) & 0xFF))) >> 8;
                    b0 = (ey0 * b0 + ey1 * b1) >> 8;
                    g0 = (ey0 * g0 + ey1 * g1) >> 8;
                    r0 = (ey0 * r0 + ey1 * r1) >> 8;

                    if (fAlpha)
                    {
                        a0 = ((ex0 * ((c00 >> 24) & 0xFF)) +
                              (ex1 * ((c10 >> 24) & 0xFF))) >> 8;
                        a1 = ((ex0 * ((c01 >> 24) & 0xFF)) +
                              (ex1 * ((c11 >> 24) & 0xFF))) >> 8;
                        a0 = (ey0 * a0 + ey1 * a1) >> 8;
                    }
                    ((LPDWORD)pbNewLine)[ix] =
                        MAKELONG(MAKEWORD(b0, g0), MAKEWORD(r0, a0));
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, pbBits);
        DeleteDC(hdc);
    }
    return hbmNew;
}

static void BGPreloadWallpaper(BGSrc *src)
{
	HBITMAP       hbm;
	WallpaperInfo wi;
	int s_width, s_height;

	BGGetWallpaperInfo(&wi);

	if (IsLoadImageOnlyEnabled()) {
		//壁紙を読み込み
		//LR_CREATEDIBSECTION を指定するのがコツ
		if (wi.pattern == BG_STRETCH) {
			hbm = LoadImageW(0, wi.filename, IMAGE_BITMAP, CRTWidth, CRTHeight, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		}
		else {
			hbm = LoadImageW(0, wi.filename, IMAGE_BITMAP,         0,        0, LR_LOADFROMFILE);
		}
	}
	else {
		BITMAP bm;
		float ratio;

		hbm = GetBitmapHandleW(wi.filename);
		if (hbm == NULL) {
			goto load_finish;
		}

		GetObject(hbm,sizeof(bm),&bm);
		// 壁紙の設定に合わせて、画像のストレッチサイズを決める。
		if (wi.pattern == BG_STRETCH) {
			s_width = CRTWidth;
			s_height = CRTHeight;
		} else if (wi.pattern == BG_AUTOFILL || wi.pattern == BG_AUTOFIT) {
			if (wi.pattern == BG_AUTOFILL) {
				if ((bm.bmHeight * CRTWidth) < (bm.bmWidth * CRTHeight)) {
					wi.pattern = BG_FIT_HEIGHT;
				}
				else {
					wi.pattern = BG_FIT_WIDTH;
				}
			}
			if (wi.pattern == BG_AUTOFIT) {
				if ((bm.bmHeight * CRTWidth) < (bm.bmWidth * CRTHeight)) {
					wi.pattern = BG_FIT_WIDTH;
				}
				else {
					wi.pattern = BG_FIT_HEIGHT;
				}
			}
			if (wi.pattern == BG_FIT_WIDTH) {
				ratio = (float)CRTWidth / bm.bmWidth;
				s_width = CRTWidth;
				s_height = (int)(bm.bmHeight * ratio);
			}
			else {
				ratio = (float)CRTHeight / bm.bmHeight;
				s_width = (int)(bm.bmWidth * ratio);
				s_height = CRTHeight;
			}

		} else {
			s_width = 0;
			s_height = 0;
		}

		if (s_width && s_height) {
			HBITMAP newhbm = CreateStretched32BppBitmapBilinear(hbm, s_width, s_height);
			DeleteObject(hbm);
			hbm = newhbm;

			wi.pattern = BG_STRETCH;
		}
	}
load_finish:
	free(wi.filename);

	//壁紙DCを作る
	if(hbm)
	{
		BITMAP bm;

		GetObject(hbm,sizeof(bm),&bm);

		src->hdc     = CreateBitmapDC(hbm);
		src->width   = bm.bmWidth;
		src->height  = bm.bmHeight;
		src->pattern = wi.pattern;

	}else{
		src->hdc = NULL;
	}

	src->color = GetSysColor(COLOR_DESKTOP);
}

static void BGPreloadSrc(BGSrc *src)
{
	if (!src->enable) {
		return;
	}

	DeleteBitmapDC(&(src->hdc));

	switch (src->type) {
		case BG_COLOR:
			break;

		case BG_WALLPAPER:
			BGPreloadWallpaper(src);
			break;

		case BG_PICTURE:
			BGPreloadPicture(src);
			break;

		default:
			break;
	}
}

static void BGStretchPicture(HDC hdcDest,BGSrc *src,int x,int y,int width,int height,BOOL bAntiAlias)
{
	const wchar_t *filename = src->fileW;
	if(!hdcDest || !src)
		return;

	if(bAntiAlias)
	{
		if(src->width != width || src->height != height)
		{
			HBITMAP hbm;

			if (IsLoadImageOnlyEnabled()) {
				hbm = LoadImageW(0,filename,IMAGE_BITMAP,width,height,LR_LOADFROMFILE);
			} else {
				HBITMAP newhbm;
				hbm = GetBitmapHandleW(filename);
				newhbm = CreateStretched32BppBitmapBilinear(hbm, width, height);
				DeleteObject(hbm);
				hbm = newhbm;
			}

			if(!hbm)
				return;

			DeleteBitmapDC(&(src->hdc));
			src->hdc = CreateBitmapDC(hbm);
			src->width  = width;
			src->height = height;
		}

		BitBlt(hdcDest,x,y,width,height,src->hdc,0,0,SRCCOPY);
	}else{
		SetStretchBltMode(src->hdc,COLORONCOLOR);
		StretchBlt(hdcDest,x,y,width,height,src->hdc,0,0,src->width,src->height,SRCCOPY);
	}
}

static void BGLoadPicture(HDC hdcDest,BGSrc *src)
{
  int x,y,width,height,pattern;

  FillBitmapDC(hdcDest,src->color);

  if(!src->height || !src->width)
    return;

  if(src->pattern == BG_AUTOFIT){
    if((src->height * ScreenWidth) > (ScreenHeight * src->width))
      pattern = BG_FIT_WIDTH;
    else
      pattern = BG_FIT_HEIGHT;
  }else{
    pattern = src->pattern;
  }

  switch(pattern)
  {
    case BG_STRETCH :
      BGStretchPicture(hdcDest,src,0,0,ScreenWidth,ScreenHeight,src->antiAlias);
      break;

    case BG_FIT_WIDTH :

      height = (src->height * ScreenWidth) / src->width;
      y      = (ScreenHeight - height) / 2;

      BGStretchPicture(hdcDest,src,0,y,ScreenWidth,height,src->antiAlias);
      break;

    case BG_FIT_HEIGHT :

      width = (src->width * ScreenHeight) / src->height;
      x     = (ScreenWidth - width) / 2;

      BGStretchPicture(hdcDest,src,x,0,width,ScreenHeight,src->antiAlias);
      break;

    case BG_TILE :
      for(x = 0;x < ScreenWidth ;x += src->width )
      for(y = 0;y < ScreenHeight;y += src->height)
        BitBlt(hdcDest,x,y,src->width,src->height,src->hdc,0,0,SRCCOPY);
      break;

    case BG_CENTER :
      x = (ScreenWidth  -  src->width) / 2;
      y = (ScreenHeight - src->height) / 2;

      BitBlt(hdcDest,x,y,src->width,src->height,src->hdc,0,0,SRCCOPY);
      break;
  }
}

typedef struct tagLoadWallpaperStruct
{
  RECT *rectClient;
  HDC hdcDest;
  BGSrc *src;
}LoadWallpaperStruct;

static BOOL CALLBACK BGLoadWallpaperEnumFunc(HMONITOR hMonitor,HDC hdcMonitor,LPRECT lprcMonitor,LPARAM dwData)
{
  RECT rectDest;
  RECT rectRgn;
  int  monitorWidth;
  int  monitorHeight;
  int  destWidth;
  int  destHeight;
  HRGN hRgn;
  int  x;
  int  y;
  LoadWallpaperStruct *lws;

  (void)hMonitor;
  (void)hdcMonitor;

  lws = (LoadWallpaperStruct*)dwData;

  if(!IntersectRect(&rectDest,lprcMonitor,lws->rectClient))
    return TRUE;

  //モニターにかかってる部分をマスク
  SaveDC(lws->hdcDest);
  CopyRect(&rectRgn,&rectDest);
  OffsetRect(&rectRgn,- lws->rectClient->left,- lws->rectClient->top);
  hRgn = CreateRectRgnIndirect(&rectRgn);
  SelectObject(lws->hdcDest,hRgn);

  //モニターの大きさ
  monitorWidth  = lprcMonitor->right  - lprcMonitor->left;
  monitorHeight = lprcMonitor->bottom - lprcMonitor->top;

  destWidth  = rectDest.right  - rectDest.left;
  destHeight = rectDest.bottom - rectDest.top;

  switch(lws->src->pattern)
  {
    case BG_CENTER  :
    case BG_STRETCH :

      SetWindowOrgEx(lws->src->hdc,
                     lprcMonitor->left + (monitorWidth  - lws->src->width )/2,
                     lprcMonitor->top  + (monitorHeight - lws->src->height)/2,NULL);
      BitBlt(lws->hdcDest ,rectDest.left,rectDest.top,destWidth,destHeight,
             lws->src->hdc,rectDest.left,rectDest.top,SRCCOPY);

      break;
    case BG_TILE :

      SetWindowOrgEx(lws->src->hdc,0,0,NULL);

      for(x = rectDest.left - (rectDest.left % lws->src->width ) - lws->src->width ;
          x < rectDest.right ;x += lws->src->width )
      for(y = rectDest.top  - (rectDest.top  % lws->src->height) - lws->src->height;
          y < rectDest.bottom;y += lws->src->height)
        BitBlt(lws->hdcDest,x,y,lws->src->width,lws->src->height,lws->src->hdc,0,0,SRCCOPY);
      break;
    default:
      break;
  }

  //リージョンを破棄
  RestoreDC(lws->hdcDest,-1);
  DeleteObject(hRgn);

  return TRUE;
}

static void BGLoadWallpaper(HDC hdcDest,BGSrc *src)
{
  RECT  rectClient;
  POINT point;
  LoadWallpaperStruct lws;

  //取りあえずデスクトップ色で塗りつぶす
  FillBitmapDC(hdcDest,src->color);

  //壁紙が設定されていない
  if(!src->hdc)
    return;

  //hdcDestの座標系を仮想スクリーンに合わせる
  point.x = 0;
  point.y = 0;
  ClientToScreen(HVTWin,&point);

  SetWindowOrgEx(hdcDest,point.x,point.y,NULL);

  //仮想スクリーンでのクライアント領域
  GetClientRect(HVTWin,&rectClient);
  OffsetRect(&rectClient,point.x,point.y);

  //モニターを列挙
  lws.rectClient = &rectClient;
  lws.src        = src;
  lws.hdcDest    = hdcDest;

  if(pEnumDisplayMonitors != NULL)
  {
    (*pEnumDisplayMonitors)(NULL,NULL,BGLoadWallpaperEnumFunc,(LPARAM)&lws);
  }else{
    RECT rectMonitor;

    SetRect(&rectMonitor,0,0,CRTWidth,CRTHeight);
    BGLoadWallpaperEnumFunc(NULL,NULL,&rectMonitor,(LPARAM)&lws);
  }

  //座標系を戻す
  SetWindowOrgEx(hdcDest,0,0,NULL);
}

static void BGLoadSrc(HDC hdcDest, BGSrc *src)
{
	switch (src->type) {
		case BG_COLOR:
			FillBitmapDC(hdcDest, src->color);
			break;

		case BG_WALLPAPER:
			BGLoadWallpaper(hdcDest, src);
			break;

		case BG_PICTURE:
			BGLoadPicture(hdcDest, src);
			break;

		default:
			break;
	}
}

/**
 *	32bit bitmap でも alphaが使用されていないときは
 *	不透明なbitmapと同様に扱う
 */
static BOOL IsAlphaValidBitmap(HBITMAP hBmp)
{
	BITMAP bm;
	BOOL alpha = FALSE;

	assert(hBmp != NULL);

	// 1pixelあたりのbit数をチェックする
	GetObject(hBmp, sizeof(bm), &bm);
	if (bm.bmBitsPixel != 32) {
		// 32bit/pixel 以外の時は alpha 情報はない
		return FALSE;
	}

	// alpha情報が使用されているかチェックする
	//		すべてのピクセルで不透明となっているか調べる
	{
		// DIBを作成する hBmp のコピー先
		//  hBmp が DDB のとき pixel情報をチェックできないので DIB へコピー(BitBlt)する
		LONG width;
		LONG height;
		BITMAPINFO bmi;
		void* pvBits;
		HBITMAP hBmpDIB;
		HBITMAP hBmpCopy;
		HDC HDCDest;
		HDC HDCSrc;
		HBITMAP prev1;
		HBITMAP prev2;
		DWORD *p;
		LONG i;

		width = bm.bmWidth;
		height = bm.bmHeight;
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(bmi);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;	// = 8*4
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = width * height * 4;
		hBmpDIB = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

		// hBmp が SelectObject() されていたとき
		// 別HDCにSelectObject()できないのでコピーしておく
		hBmpCopy = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

		// hBmp(hBmpCopy)をDIBにコピー(BitBlt)する
		HDCDest = CreateCompatibleDC(NULL);
		HDCSrc = CreateCompatibleDC(NULL);
		prev1 = SelectObject(HDCDest, hBmpDIB);
		prev2 = SelectObject(HDCSrc, hBmpCopy);
		BitBlt(HDCDest, 0, 0, width, height, HDCSrc, 0, 0, SRCCOPY);

		// alpha値をチェックする
		p = pvBits;
		for (i = 0; i < width * height; i++) {
			DWORD pix = *p++;
			if ((pix & 0xff000000) != 0xff000000) {
				alpha = TRUE;
				break;
			}
		}

		// 破棄
		SelectObject(HDCDest, prev1);
		SelectObject(HDCSrc, prev2);
		DeleteObject(hBmpDIB);
		DeleteObject(hBmpCopy);
		DeleteDC(HDCDest);
		DeleteDC(HDCSrc);
	}
	return alpha;
}

static BOOL IsAlphaValidBitmapHDC(HDC hDC)
{
	HBITMAP hBmp;

	assert(hDC != NULL);
	hBmp = GetCurrentObject(hDC, OBJ_BITMAP);
	return IsAlphaValidBitmap(hBmp);
}

/**
 *	背景画像生成
 */
static HDC CreateBGImage(int width, int height)
{
	BLENDFUNCTION bf;
	HDC hdc_work;
	HDC hdc_bg;

	hdc_bg = CreateBitmapDC(CreateScreenCompatibleBitmap(width, height));
	hdc_work = CreateBitmapDC(CreateScreenCompatibleBitmap(width, height));

	// 壁紙(Windowsのデスクトップ背景)
	if (BGSrc1.enable) {
		BGLoadSrc(hdc_bg, &BGSrc1);
	}
	DebugSaveFile(L"bg_1.bmp", hdc_bg, width, height);

	// 指定画像
	if (BGDest.enable && BGDest.hdc != NULL) {
		BOOL alpha_valid;

		// 画像をロード(描画)する
		BGLoadSrc(hdc_work, &BGDest);
		alpha_valid = IsAlphaValidBitmapHDC(BGDest.hdc);

		// 準備した画像を貼り付ける
		memset(&bf, 0, sizeof(bf));
		bf.BlendOp = AC_SRC_OVER;
		bf.SourceConstantAlpha = BGDest.alpha;
		bf.AlphaFormat = 0;
		if (alpha_valid) {
			// 32bitビットマップ(alpha値が存在している)場合はalpha値を参照する
			bf.AlphaFormat = AC_SRC_ALPHA;
		}
		if (!BGSrc1.enable) {
			// 壁紙がない場合はあらかじめ塗りつぶしておく
			FillBitmapDC(hdc_bg, BGDest.color);
		}
		BGAlphaBlend(hdc_bg, 0, 0, width, height, hdc_work, 0, 0, width, height, bf);
	}
	DebugSaveFile(L"bg_2.bmp", hdc_bg, width, height);

	// 単色プレーン
	if (BGSrc2.enable) {
		// 画像を準備
		BGLoadSrc(hdc_work, &BGSrc2);

		// 貼り付ける
		memset(&bf, 0, sizeof(bf));
		bf.BlendOp = AC_SRC_OVER;
		if (BGDest.enable || BGSrc1.enable) {
			bf.SourceConstantAlpha = BGSrc2.alpha;
		}
		else {
			// ブレンドするものがなければalphaは使わない(単純な塗りつぶし)
			bf.SourceConstantAlpha = 255;
		}
		bf.AlphaFormat = 0;
		BGAlphaBlend(hdc_bg, 0, 0, width, height, hdc_work, 0, 0, width, height, bf);
	}
	DebugSaveFile(L"bg_3.bmp", hdc_bg, width, height);

	DeleteBitmapDC(&hdc_work);

	return hdc_bg;
}

/**
 *
 *	ThemeSetBG(), ThemeSetColor() のあとにコールする
 *
 *	@param	forceSetup		FALSE	WM_PAINT時
 *							TRUE	以外
 *
 */
void BGSetupPrimary(BOOL forceSetup)
{
  POINT point;
  RECT rect;

  if(!BGEnable)
    return;

  //窓の位置、大きさが変わったかチェック
  point.x = 0;
  point.y = 0;
  ClientToScreen(HVTWin,&point);

  GetClientRect(HVTWin,&rect);
  OffsetRect(&rect,point.x,point.y);

  if(!forceSetup && EqualRect(&rect,&BGPrevRect))
    return;

  CopyRect(&BGPrevRect,&rect);

  //壁紙 or 背景をプリロード
  BGPreloadSrc(&BGDest);
  BGPreloadSrc(&BGSrc1);
  BGPreloadSrc(&BGSrc2);

  _OutputDebugPrintf("BGSetupPrimary : BGInSizeMove = %d\n",BGInSizeMove);

  //作業用 DC 作成
  if(hdcBGWork)   DeleteBitmapDC(&hdcBGWork);
  if(hdcBGBuffer) DeleteBitmapDC(&hdcBGBuffer);

  hdcBGWork   = CreateBitmapDC(CreateScreenCompatibleBitmap(ScreenWidth,FontHeight));
  hdcBGBuffer = CreateBitmapDC(CreateScreenCompatibleBitmap(ScreenWidth,FontHeight));

  //hdcBGBuffer の属性設定
  SetBkMode(hdcBGBuffer,TRANSPARENT);

  if(!BGInSizeMove)
  {
	  //背景 HDC
	  if(hdcBG) {
		  DeleteBitmapDC(&hdcBG);
	  }

	  hdcBG = CreateBGImage(ScreenWidth, ScreenHeight);
  }
}

/**
 *	テーマファイルを読み込んで設定する
 *
 *	@param file		NULLの時ファイルを読み込まない(デフォルト値で設定)
 */
static void BGReadIniFile(const wchar_t *file)
{
	BGTheme bg_theme;
	TColorTheme color_theme;
	ThemeLoad(file, &bg_theme, &color_theme);
	ThemeSetBG(&bg_theme);
	ThemeSetColor(&color_theme);
}

static void BGDestruct(void)
{
  if(!BGEnable)
    return;

  DeleteBitmapDC(&hdcBGBuffer);
  DeleteBitmapDC(&hdcBGWork);
  DeleteBitmapDC(&hdcBG);
  DeleteBitmapDC(&(BGDest.hdc));
  DeleteBitmapDC(&(BGSrc1.hdc));
  DeleteBitmapDC(&(BGSrc2.hdc));

  BGEnable = FALSE;
}

static void BGSetDefaultColor(TTTSet *pts)
{
	TColorTheme color_theme;
	ThemeGetColorDefaultTS(pts, &color_theme);
	ThemeSetColor(&color_theme);
}

/*
 * Eterm lookfeel機能による初期化処理
 *
 * initialize_once:
 *    TRUE: Tera Termの起動時
 *    FALSE: Tera Termの起動時以外
 */
void BGInitialize(BOOL initialize_once)
{
	(void)initialize_once;

	InitColorTable(ts.ANSIColor);

	BGSetDefaultColor(&ts);

	//リソース解放
	BGDestruct();

	// AlphaBlend のアドレスを読み込み
	if (ts.EtermLookfeel.BGUseAlphaBlendAPI) {
		if (pAlphaBlend != NULL)
			BGAlphaBlend = pAlphaBlend;
		else
			BGAlphaBlend = AlphaBlendWithoutAPI;
	}
	else {
		BGAlphaBlend = AlphaBlendWithoutAPI;
	}
}

/**
 *	テーマの設定をチェックして BG処理を行うか(BGEnable=TRUE/FALSE)を決める
 */
static void DecideBGEnable(void)
{
	vtdisp_work_t *w = &vtdisp_work;

	// 背景画像チェック
	if (BGDest.fileW == NULL || BGDest.fileW[0] == 0) {
		// 背景画像は使用しない
		BGDest.enable = FALSE;
	}

	if (BGDest.enable == FALSE && BGSrc1.enable == FALSE && BGSrc2.enable == FALSE) {
		// BGは使用しない
		BGEnable = FALSE;
		w->bg_enable = FALSE;
	}
	else {
		BGEnable = TRUE;
		w->bg_enable = TRUE;
	}
}

/**
 *	テーマの設定を行う
 *		テーマ無しならデフォルト設定する
 *		テーマありならテーマファイルを読み出して設定する
 */
void BGLoadThemeFile(const TTTSet *pts)
{
	// コンフィグファイル(テーマファイル)の決定
	switch(pts->EtermLookfeel.BGEnable) {
	case 0:
	default:
		// テーマ無し
		BGReadIniFile(NULL);
		BGEnable = FALSE;
		break;
	case 1:
		if (pts->EtermLookfeel.BGThemeFileW != NULL) {
			// テーマファイルの指定がある
			BGReadIniFile(pts->EtermLookfeel.BGThemeFileW);
			BGEnable = TRUE;
		}
		else {
			BGEnable = FALSE;
		}
		break;
	case 2: {
		// ランダムテーマ (or テーマファイルを指定がない)
		wchar_t *theme_mask;
		wchar_t *theme_file;
		aswprintf(&theme_mask, L"%s\\theme\\*.ini", pts->HomeDirW);
		theme_file = RandomFileW(theme_mask);
		free(theme_mask);
		BGReadIniFile(theme_file);
		free(theme_file);
		BGEnable = TRUE;
		break;
	}
	}

	DecideBGEnable();
}

/**
 *	デバグ用、boxを描画する
 */
static void DrawBox(HDC hdc, int sx, int sy, int width, int height, COLORREF rgb)
{
	HPEN red_pen = CreatePen(PS_SOLID, 0, rgb);
	HGDIOBJ old_pen = SelectObject(hdc, red_pen);
	width--;
	height--;
	MoveToEx(hdc, sx, sy, NULL);
	LineTo(hdc, sx + width, sy);
	LineTo(hdc, sx + width, sy + height);
	LineTo(hdc, sx, sy + height);
	LineTo(hdc, sx, sy);
	MoveToEx(hdc, sx, sy, NULL);
	LineTo(hdc, sx + width, sy + height);
	MoveToEx(hdc, sx + width, sy, NULL);
	LineTo(hdc, sx, sy + height);
	SelectObject(hdc, old_pen);
	DeleteObject(red_pen);
}

/**
 * @brief 文字の背景を作成する
 *		hdc に (0,0)-(width,height) の文字背景を作成する
 *		alphaの値によって背景画像(hdcBG)をブレンドする
 *
 * @param hdc		描画するhdc
 * @param X			文字位置(背景画像の位置)
 * @param Y
 * @param width		文字サイズ
 * @param height
 * @param alpha		背景画像の不透明度 0..255
 *						0=背景画像がそのまま転送される
 *						255=背景は透明(影響なし)
 */
static void DrawTextBGImage(HDC hdc, int X, int Y, int width, int height, COLORREF color, unsigned char alpha)
{
	HBRUSH hbr = CreateSolidBrush(color);

	// hdcに背景画像を描画
	if (BGInSizeMove) {
		// BGInSizeMove!=0時(窓の移動、リサイズ中)
		RECT rect;
		SetRect(&rect, 0, 0, width, height);
		//FillRect(hdc, &rect, BGBrushInSizeMove);	//   背景を BGBrushInSizeMove で塗りつぶす
		FillRect(hdc, &rect, hbr);
	}
	else {
		RECT rect;
		BLENDFUNCTION bf;
		BOOL r;

		// ワークDCに文字の背景を描画
		SetRect(&rect, 0, 0, width, height);
		FillRect(hdcBGWork, &rect, hbr);

		// 不透明時
		//   背景画像をそのまま文字背景に転送
		BitBlt(hdc, 0, 0, width, height, hdcBG, X, Y, SRCCOPY);

		// hdcにワークDCに用意した文字の背景をalphablend
		ZeroMemory(&bf, sizeof(bf));
		bf.BlendOp = AC_SRC_OVER;
		bf.SourceConstantAlpha = alpha;
		r = BGAlphaBlend(hdc, 0, 0, width, height, hdcBGWork, 0, 0, width, height, bf);
		assert(r == TRUE);
		(void)r;
	}

	DeleteObject(hbr);
}

static void BGScrollWindow(HWND hwnd, int xa, int ya, RECT *Rect, RECT *ClipRect)
{
	RECT r;

	if (BGEnable) {
		InvalidateRect(HVTWin, ClipRect, FALSE);
	}
	else if (IsZoomed(hwnd)) {
		// ウィンドウ最大化時の文字欠け対策
		switch (ts.MaximizedBugTweak) {
		case 1: // type 1: ScrollWindow を使わずにすべて書き直す
			InvalidateRect(HVTWin, ClipRect, FALSE);
			break;
		case 2: // type 2: スクロール領域が全体(NULL)の時は隙間部分を除いた領域に差し替える
			if (Rect == NULL) {
				GetClientRect(hwnd, &r);
				r.bottom -= r.bottom % ts.TerminalHeight;
				Rect = &r;
			}
			/* FALLTHROUGH */
		default:
			ScrollWindow(hwnd, xa, ya, Rect, ClipRect);
			break;
		}
	}
	else {
		ScrollWindow(hwnd, xa, ya, Rect, ClipRect);
	}
}

void BGOnEnterSizeMove(void)
{
  if(!BGEnable || !ts.EtermLookfeel.BGFastSizeMove)
    return;

  BGInSizeMove = TRUE;
}

void BGOnExitSizeMove(void)
{
  if(!BGEnable || !ts.EtermLookfeel.BGFastSizeMove)
    return;

  BGInSizeMove = FALSE;

  BGSetupPrimary(TRUE);
  InvalidateRect(HVTWin,NULL,FALSE);

#if 0
  //ブラシを削除
  if(BGBrushInSizeMove)
  {
    DeleteObject(BGBrushInSizeMove);
    BGBrushInSizeMove = NULL;
  }
#endif
}

/**
 *	WM_SETTINGCHANGE 時に呼び出す
 */
void BGOnSettingChange(void)
{
	if(!BGEnable)
		return;

	// TODO モニタ(ディスプレイ)をまたぐとサイズが変化するのでは?
	CRTWidth  = GetSystemMetrics(SM_CXSCREEN);
	CRTHeight = GetSystemMetrics(SM_CYSCREEN);

	BGSetupPrimary(TRUE);
	InvalidateRect(HVTWin, NULL, FALSE);
}

/**
 *	旧16色カラーテーブル(ts.ANSIColor[16])の色番号から
 *	256色カラーテーブル(ANSIColor[256])の色番号を取得
 *
 * @param index16 	16色の色番号
 */
static int GetIndex256From16(int index16)
{
	// ANSIColor16は、明るい/暗いグループが入れ替わっている
	static const int index256[] = {
		0,
		9, 10, 11, 12, 13, 14, 15,
		8,
		1, 2, 3, 4, 5, 6, 7
	};
	return index16 < _countof(index256) ? index256[index16] : index16;
}

/**
 *	256色カラーテーブル(ANSIColor[256])の色番号から
 *	旧16色カラーテーブル(ts.ANSIColor[16])の色番号を取得
 *
 * @param index256 	256色の色番号
 */
static int GetIndex16From256(int index256)
{
	return GetIndex256From16(index256);
}

/**
 *	ANSIカラーテーブル(ANSIColor[256])を初期化する
 *
 *	@param ANSIColor16	ts.ANSIColor[16]
 *						旧16色カラーテーブル
 */
static void InitColorTable(const COLORREF *ANSIColor16)
{
	int i;

	// ANSIColor[] の先頭16色を初期化
	//		ANSIColor16は16色カラーテーブル
	for (i = 0 ; i < 16 ; i++) {
		int i256 = GetIndex256From16(i);
		ANSIColor[i256] = ANSIColor16[i];
	}

	// ANSIColor[] の16番以降を初期化
	for (i=16; i<=255; i++) {
		ANSIColor[i] = RGB(DefaultColorTable[i][0], DefaultColorTable[i][1], DefaultColorTable[i][2]);
	}
}

static void DispSetNearestColors(int start, int end, HDC DispCtx)
{
#if 1
	// 無効化
	(void)start;
	(void)end;
	(void)DispCtx;
#else
  HDC TmpDC;
  int i;

  if (DispCtx) {
	TmpDC = DispCtx;
  }
  else {
	TmpDC = GetDC(NULL);
  }

  for (i = start ; i <= end; i++)
    ANSIColor[i] = GetNearestColor(TmpDC, ANSIColor[i]);

  if (!DispCtx) {
	ReleaseDC(NULL, TmpDC);
  }
#endif
}

void InitDisp(void)
{
  HDC TmpDC;
  BOOL bMultiDisplaySupport = FALSE;
  vtdisp_work_t *w = &vtdisp_work;

  TmpDC = GetDC(NULL);

  CRTWidth  = GetSystemMetrics(SM_CXSCREEN);
  CRTHeight = GetSystemMetrics(SM_CYSCREEN);

  BGInitialize(TRUE);

  DispSetNearestColors(IdBack, 255, TmpDC);

  /* background paintbrush */
  Background = CreateSolidBrush(ts.VTColor[1]);
  /* CRT width & height */
  if (HasMultiMonitorSupport()) {
    bMultiDisplaySupport = TRUE;
  }
  if( bMultiDisplaySupport ) {
	  VirtualScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	  VirtualScreen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	  VirtualScreen.right = VirtualScreen.left +  GetSystemMetrics(SM_CXVIRTUALSCREEN);
	  VirtualScreen.bottom = VirtualScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
  } else {
	  VirtualScreen.left = 0;
	  VirtualScreen.top = 0;
	  VirtualScreen.right = GetDeviceCaps(TmpDC,HORZRES);
	  VirtualScreen.bottom = GetDeviceCaps(TmpDC,VERTRES);
  }

  ReleaseDC(NULL, TmpDC);

  if ( (ts.VTPos.x > VirtualScreen.right) || (ts.VTPos.y > VirtualScreen.bottom) )
  {
    ts.VTPos.x = CW_USEDEFAULT;
    ts.VTPos.y = CW_USEDEFAULT;
  }
  else if ( (ts.VTPos.x < VirtualScreen.left-20) || (ts.VTPos.y < VirtualScreen.top-20) )
  {
    ts.VTPos.x = CW_USEDEFAULT;
    ts.VTPos.y = CW_USEDEFAULT;
  }
  else {
    if ( ts.VTPos.x < VirtualScreen.left ) ts.VTPos.x = VirtualScreen.left;
    if ( ts.VTPos.y < VirtualScreen.top ) ts.VTPos.y = VirtualScreen.top;
  }

  if ( (ts.TEKPos.x >  VirtualScreen.right) || (ts.TEKPos.y > VirtualScreen.bottom) )
  {
    ts.TEKPos.x = CW_USEDEFAULT;
    ts.TEKPos.y = CW_USEDEFAULT;
  }
  else if ( (ts.TEKPos.x < VirtualScreen.left-20) || (ts.TEKPos.y < VirtualScreen.top-20) )
  {
    ts.TEKPos.x = CW_USEDEFAULT;
    ts.TEKPos.y = CW_USEDEFAULT;
  }
  else {
    if ( ts.TEKPos.x < VirtualScreen.left ) ts.TEKPos.x = VirtualScreen.left;
    if ( ts.TEKPos.y < VirtualScreen.top ) ts.TEKPos.y = VirtualScreen.top;
  }

  w->bg_enable = FALSE;
  w->alpha_back = 255;
  w->alpha_vtback = 255;
  w->debug_drawbox_text = FALSE;
  w->font_resize_enable = FontReSizeEnableInit;
  BGReverseTextAlpha = 255;
}

void EndDisp(void)
{
  int i, j;

  if (VTDC!=NULL) DispReleaseDC();

  /* Delete fonts */
  for (i = 0 ; i <= AttrFontMask; i++)
  {
    for (j = i+1 ; j <= AttrFontMask ; j++)
      if (VTFont[j]==VTFont[i])
        VTFont[j] = 0;
    if (VTFont[i]!=0) DeleteObject(VTFont[i]);
  }

  if (Background!=0)
  {
	DeleteObject(Background);
	Background = 0;
  }

  BGDestruct();

  free(BGDest.fileW);
  BGDest.fileW = NULL;
}

void DispReset(void)
{
  /* Cursor */
  CursorX = 0;
  CursorY = 0;

  /* Scroll status */
  ScrollCount = 0;
  dScroll = 0;

  if (IsCaretOn()) CaretOn();
  DispEnableCaret(TRUE); // enable caret
}

void DispConvWinToScreen
  (int Xw, int Yw, int *Xs, int *Ys, PBOOL Right)
// Converts window coordinate to screen cordinate
//   Xs: horizontal position in window coordinate (pixels)
//   Ys: vertical
//  Output
//	 Xs, Ys: screen coordinate
//   Right: TRUE if the (Xs,Ys) is on the right half of
//			 a character cell.
{
  if (Xs!=NULL)
	*Xs = Xw / FontWidth + WinOrgX;
  *Ys = Yw / FontHeight + WinOrgY;
  if ((Xs!=NULL) && (Right!=NULL))
    *Right = (Xw - (*Xs-WinOrgX)*FontWidth) >= FontWidth/2;
}

void DispConvScreenToWin
  (int Xs, int Ys, int *Xw, int *Yw)
// Converts screen coordinate to window cordinate
//   Xs: horizontal position in screen coordinate (characters)
//   Ys: vertical
//  Output
//      Xw, Yw: window coordinate
{
  if (Xw!=NULL)
       *Xw = (Xs - WinOrgX) * FontWidth;
  if (Yw!=NULL)
       *Yw = (Ys - WinOrgY) * FontHeight;
}

/**
 *	VTFont の取得
 *
 *	@param[out]		VTlf	取得したフォント情報
 *	@param[in]		dpi		DPIに合わせたサイズのフォントを取得
 *							0 の時DPIに合わせた拡大をしない
 */
static void DispSetLogFont(LOGFONTW *VTlf, unsigned int dpi)
{
  memset(VTlf, 0, sizeof(*VTlf));
  VTlf->lfWeight = FW_NORMAL;
  VTlf->lfItalic = 0;
  VTlf->lfUnderline = 0;
  VTlf->lfStrikeOut = 0;
  VTlf->lfWidth = ts.VTFontSize.x;
  VTlf->lfHeight = ts.VTFontSize.y;
  VTlf->lfCharSet = ts.VTFontCharSet;
  VTlf->lfOutPrecision  = OUT_CHARACTER_PRECIS;
  VTlf->lfClipPrecision = CLIP_CHARACTER_PRECIS;
  VTlf->lfQuality       = (BYTE)ts.FontQuality;
  VTlf->lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
  ACPToWideChar_t(ts.VTFont, VTlf->lfFaceName, _countof(VTlf->lfFaceName));
  if (dpi != 0) {
	  VTlf->lfWidth = MulDiv(VTlf->lfWidth, dpi, 96);
	  VTlf->lfHeight = MulDiv(VTlf->lfHeight, dpi, 96);
  }
}

/**
 *	フォントを変更
 *
 *	@param	dpi		DPIを指定する
 *					0のとき現在のディスプレイのDPI
 */
void ChangeFont(unsigned int dpi)
{
	int i, j;
	LOGFONTW VTlf;

	/* Delete Old Fonts */
	for (i = 0 ; i <= AttrFontMask ; i++)
	{
		for (j = i+1 ; j <= AttrFontMask ; j++)
			if (VTFont[j]==VTFont[i])
				VTFont[j] = 0;
		if (VTFont[i]!=0)
			DeleteObject(VTFont[i]);
	}

	if (dpi == 0) {
		dpi = GetMonitorDpiFromWindow(HVTWin);
	}

	/* Normal Font */
	DispSetLogFont(&VTlf, dpi);
	VTFont[AttrDefault] = CreateFontIndirectW(&VTlf);

	if (ts.UseIME > 0) {
		if (ts.IMEInline > 0) {
			/* set IME font */
			SetConversionLogFont(HVTWin, &VTlf);
		}
	}

	{
		HDC TmpDC = GetDC(HVTWin);
		TEXTMETRIC Metrics;

		SelectObject(TmpDC, VTFont[AttrDefault]);
		GetTextMetrics(TmpDC, &Metrics);
		FontWidth = Metrics.tmAveCharWidth + ts.FontDW;
		FontHeight = Metrics.tmHeight + ts.FontDH;

		ReleaseDC(HVTWin,TmpDC);
	}

	/* Underline */
	if ((ts.FontFlag & FF_UNDERLINE) || (ts.FontFlag & FF_URLUNDERLINE)) {
		VTlf.lfUnderline = 1;
		VTFont[AttrUnder] = CreateFontIndirectW(&VTlf);
	}
	else {
		VTFont[AttrUnder] = VTFont[AttrDefault];
	}

	if (ts.FontFlag & FF_BOLD) {
		/* Bold */
		VTlf.lfUnderline = 0;
		VTlf.lfWeight = FW_BOLD;
		VTFont[AttrBold] = CreateFontIndirectW(&VTlf);

		/* Bold + Underline */
		if (ts.FontFlag & FF_UNDERLINE || ts.FontFlag & FF_URLUNDERLINE) {
			VTlf.lfUnderline = 1;
			VTFont[AttrBold | AttrUnder] = CreateFontIndirectW(&VTlf);
		}
		else {
			VTFont[AttrBold | AttrUnder] = VTFont[AttrBold];
		}
	}
	else {
		VTFont[AttrBold] = VTFont[AttrDefault];
		VTFont[AttrBold | AttrUnder] = VTFont[AttrUnder];
	}

	/* Special font */
	VTlf.lfWeight = FW_NORMAL;
	VTlf.lfUnderline = 0;
	VTlf.lfWidth = FontWidth + 1; /* adjust width */
	VTlf.lfHeight = FontHeight;
	VTlf.lfCharSet = SYMBOL_CHARSET;

	wcsncpy_s(VTlf.lfFaceName, _countof(VTlf.lfFaceName), L"Tera Special", _TRUNCATE);
	VTFont[AttrSpecial] = CreateFontIndirectW(&VTlf);

	/* Special font (Underline) */
	if (ts.FontFlag & FF_UNDERLINE || ts.FontFlag & FF_URLUNDERLINE) {
		VTlf.lfUnderline = 1;
		VTlf.lfHeight = FontHeight - 1; // adjust for underline
		VTFont[AttrSpecial | AttrUnder] = CreateFontIndirectW(&VTlf);
	}
	else {
		VTFont[AttrSpecial | AttrUnder] = VTFont[AttrSpecial];
	}

	if (ts.FontFlag & FF_BOLD) {
		/* Special font (Bold) */
		VTlf.lfUnderline = 0;
		VTlf.lfHeight = FontHeight;
		VTlf.lfWeight = FW_BOLD;
		VTFont[AttrSpecial | AttrBold] = CreateFontIndirectW(&VTlf);

		/* Special font (Bold + Underline) */
		if (ts.FontFlag & FF_UNDERLINE || ts.FontFlag & FF_URLUNDERLINE) {
			VTlf.lfUnderline = 1;
			VTlf.lfHeight = FontHeight - 1; // adjust for underline
			VTFont[AttrSpecial | AttrBold | AttrUnder] = CreateFontIndirectW(&VTlf);
		}
		else {
			VTFont[AttrSpecial | AttrBold | AttrUnder] = VTFont[AttrSpecial | AttrBold];
		}
	}
	else {
		VTFont[AttrSpecial | AttrBold] = VTFont[AttrSpecial];
		VTFont[AttrSpecial | AttrBold | AttrUnder] = VTFont[AttrSpecial | AttrUnder];
	}
}

void ResetIME(void)
{
	/* reset IME */
	if ((IMEEnabled() == TRUE) && (ts.UseIME > 0)) {

		// IME初期化
		if (! LoadIME()) {
			static const TTMessageBoxInfoW info = {
				"Tera Term",
				"MSG_TT_ERROR", L"Tera Term: Error",
				"MSG_USE_IME_ERROR", L"Can't use IME",
				MB_ICONEXCLAMATION
			};
			TTMessageBoxW(0, &info, ts.UILanguageFileW);
			WritePrivateProfileStringW(L"Tera Term", L"IME", L"off", ts.SetupFNameW);
			ts.UseIME = 0;
		}

		if (ts.UseIME > 0) {
			if (ts.IMEInline>0) {
				LOGFONTW VTlf;
				DispSetLogFont(&VTlf, GetMonitorDpiFromWindow(HVTWin));
				SetConversionLogFont(HVTWin, &VTlf);
			}
			else {
				SetConversionWindow(HVTWin,-1,0);
			}
		}
	}
	else {
		FreeIME(HVTWin);
	}

	if (IsCaretOn()) CaretOn();
}

void ChangeCaret(void)
{
  UINT T;

  if (! Active) return;
  DestroyCaret();
  switch (ts.CursorShape) {
    case IdVCur:
	CreateCaret(HVTWin, 0, CurWidth, FontHeight);
	break;
    case IdHCur:
	CreateCaret(HVTWin, 0, FontWidth, CurWidth);
	break;
  }
  if (CaretEnabled) {
	CaretStatus = 1;
  }
  CaretOn();
  if (CaretEnabled && (ts.NonblinkingCursor!=0)) {
    T = GetCaretBlinkTime() * 2 / 3;
    SetTimer(HVTWin,IdCaretTimer,T,NULL);
  }
  UpdateCaretPosition(TRUE);
}

// WM_KILLFOCUSされたときのカーソルを自分で描く
void CaretKillFocus(BOOL show)
{
  int CaretX, CaretY;
  POINT p[5];
  HPEN oldpen;
  HDC hdc;

  if (ts.KillFocusCursor == 0)
	  return;

  // Eterm lookfeelの場合は何もしない
  if (BGEnable)
	  return;

  /* Get Device Context */
  DispInitDC();
  hdc = VTDC;

  CaretX = (CursorX-WinOrgX)*FontWidth;
  CaretY = (CursorY-WinOrgY)*FontHeight;

  p[0].x = CaretX;
  p[0].y = CaretY;
  p[1].x = CaretX;
  p[1].y = CaretY + FontHeight - 1;
  if (CursorOnDBCS)
	p[2].x = CaretX + FontWidth*2 - 1;
  else
	p[2].x = CaretX + FontWidth - 1;
  p[2].y = CaretY + FontHeight - 1;
  if (CursorOnDBCS)
	p[3].x = CaretX + FontWidth*2 - 1;
  else
	p[3].x = CaretX + FontWidth - 1;
  p[3].y = CaretY;
  p[4].x = CaretX;
  p[4].y = CaretY;

  if (show) {  // ポリゴンカーソルを表示（非フォーカス時）
	  oldpen = SelectObject(hdc, CreatePen(PS_SOLID, 0, ts.VTColor[0]));
  } else {
	  oldpen = SelectObject(hdc, CreatePen(PS_SOLID, 0, ts.VTColor[1]));
  }
  Polyline(VTDC, p, 5);
  oldpen = SelectObject(hdc, oldpen);
  DeleteObject(oldpen);

  /* release device context */
  DispReleaseDC();
}

// ポリゴンカーソルを消したあとに、その部分の文字を再描画する。
//
// CaretOff()の直後に呼ぶこと。CaretOff()内から呼ぶと、無限再帰呼び出しとなり、
// stack overflowになる。
//
// カーソル形状変更時(ChangeCaret)にも呼ぶことにしたため、関数名変更 -- 2009/04/17 doda.
//
void UpdateCaretPosition(BOOL enforce)
{
  int CaretX, CaretY;
  RECT rc;

  CaretX = (CursorX-WinOrgX)*FontWidth;
  CaretY = (CursorY-WinOrgY)*FontHeight;

  if (!enforce && !ts.KillFocusCursor)
	  return;

  // Eterm lookfeelの場合は何もしない
  if (BGEnable)
	  return;

  if (enforce == TRUE || !Active) {
	  rc.left = CaretX;
	  rc.top = CaretY;
	  if (CursorOnDBCS)
		rc.right = CaretX + FontWidth*2;
	  else
		rc.right = CaretX + FontWidth;
	  rc.bottom = CaretY + FontHeight;
	  // 指定よりも1ピクセル小さい範囲が再描画されるため
	  // rc の right, bottom は1ピクセル大きくしている。
	  InvalidateRect(HVTWin, &rc, FALSE);
  }
}

void CaretOn(void)
// Turn on the cursor
{
#if UNICODE_DEBUG_CARET_OFF
	return;
#endif
	if (ts.KillFocusCursor == 0 && !Active)
		return;

	if (! CaretEnabled) return;

	if (Active) {
		int CaretX, CaretY, H;
		HBITMAP color;

		/* IMEのon/off状態を見て、カーソルの色を変更する。
		 * WM_INPUTLANGCHANGE, WM_IME_NOTIFY ではカーソルの再描画のみ行う。
		 * (2010.5.20 yutaka)
		 */
		if ((ts.WindowFlag & WF_IMECURSORCHANGE) == 0) {
			color = 0;
		} else {
			if (IMEstat) {
				color = (HBITMAP)1;
			} else {
				color = 0;
			}
		}

		CaretX = (CursorX-WinOrgX)*FontWidth;
		CaretY = (CursorY-WinOrgY)*FontHeight;

		if (IMEstat && IMECompositionState) {
			// IME ON && 変換中の場合のみの処理する。
			// 変換中(漢字や候補ウィンドウが表示されている状態)で
			// ホストからのエコーを受信してcaret位置が変化した場合、
			// 変換している位置を更新する必要がある。
			SetConversionWindow(HVTWin,CaretX,CaretY);
		}

		if (ts.CursorShape!=IdVCur) {
			if (ts.CursorShape==IdHCur) {
				CaretY = CaretY+FontHeight-CurWidth;
				H = CurWidth;
			}
			else {
				H = FontHeight;
			}

			DestroyCaret();
			if (CursorOnDBCS) {
				/* double width caret */
				CreateCaret(HVTWin, color, FontWidth*2, H);
			}
			else {
				/* single width caret */
				CreateCaret(HVTWin, color, FontWidth, H);
			}
			CaretStatus = 1;
		}
		SetCaretPos(CaretX,CaretY);
	}

	while (CaretStatus > 0) {
		if (! Active) {
			CaretKillFocus(TRUE);
		} else {
			ShowCaret(HVTWin);
		}
		CaretStatus--;
	}
}

void CaretOff(void)
{
	if (ts.KillFocusCursor == 0 && !Active)
		return;

	if (CaretStatus == 0) {
		if (! Active) {
			CaretKillFocus(FALSE);
		} else {
			HideCaret(HVTWin);
		}
		CaretStatus++;
	}
}

void DispDestroyCaret(void)
{
  DestroyCaret();
  if (ts.NonblinkingCursor!=0)
	KillTimer(HVTWin,IdCaretTimer);
}

BOOL IsCaretOn(void)
// check if caret is on
{
	return ((ts.KillFocusCursor || Active) && (CaretStatus==0));
}

void DispEnableCaret(BOOL On)
{
#if UNICODE_DEBUG_CARET_OFF
  On = FALSE;
#endif
  if (! On) CaretOff();
  CaretEnabled = On;
}

BOOL IsCaretEnabled(void)
{
  return CaretEnabled;
}

void DispSetCaretWidth(BOOL DW)
{
  /* TRUE if cursor is on a DBCS character */
  CursorOnDBCS = DW;
}

/**
 *	ウィンドウのサイズが変化
 *	BuffChangeWinSize() からコールされる
 *
 */
void DispChangeWinSize(int Nx, int Ny)
{
  LONG W,H,dW,dH;
  RECT R;

  if (SaveWinSize)
  {
    WinWidthOld = WinWidth;
    WinHeightOld = WinHeight;
    SaveWinSize = FALSE;
  }
  else {
    WinWidthOld = NumOfColumns;
    WinHeightOld = NumOfLines;
  }

  WinWidth = Nx;
  WinHeight = Ny;

  ScreenWidth = WinWidth*FontWidth;
  ScreenHeight = WinHeight*FontHeight;

  AdjustScrollBar();

  GetWindowRect(HVTWin,&R);
  W = R.right-R.left;
  H = R.bottom-R.top;
  GetClientRect(HVTWin,&R);
  dW = ScreenWidth - R.right + R.left;
  dH = ScreenHeight - R.bottom + R.top;

  if ((dW!=0) || (dH!=0))
  {
	AdjustSize = TRUE;

	// SWP_NOMOVE を指定しているのになぜか 0,0 が反映され、
	// マルチディスプレイ環境ではプライマリモニタに
	// 移動してしまうのを修正 (2008.5.29 maya)
	//SetWindowPos(HVTWin,HWND_TOP,0,0,W+dW,H+dH,SWP_NOMOVE);

	// マルチディスプレイ環境で最大化したときに、
	// 隣のディスプレイにウィンドウの端がはみ出す問題を修正 (2008.5.30 maya)
	// また、上記の状態では最大化状態でもウィンドウを移動させることが出来る。
	if (!IsZoomed(HVTWin)) {
		SetWindowPos(HVTWin,HWND_TOP,R.left,R.top,W+dW,H+dH,SWP_NOMOVE);
	}
  }
  else
    InvalidateRect(HVTWin,NULL,FALSE);
}

/**
 *	ウィンドウのサイズが変化する時(WM_SIZEが発生した時)に、
 *	AdjustSize == TRUEの時にコールされる
 *
 *	@param	x	ウィンドウのleft
 *	@param	y	ウィンドウのtop
 *	@param	w	リサイズ前のウィンドウw
 *	@param	h	リサイズ前のウィンドウh
 *	@param	cw	新しいクライアント領域のw
 *	@param	ch	新しいクライアント領域のh
 */
void ResizeWindow(int x, int y, int w, int h, int cw, int ch)
{
  int dw,dh, NewX, NewY;
  POINT Point;

  if (! AdjustSize) return;
  dw = ScreenWidth - cw;
  dh = ScreenHeight - ch;
  if ((dw!=0) || (dh!=0)) {
    SetWindowPos(HVTWin,HWND_TOP,x,y,w+dw,h+dh,SWP_NOMOVE);
    AdjustSize = FALSE;
  }
  else {
    AdjustSize = FALSE;

    NewX = x;
    NewY = y;
    if (x+w > VirtualScreen.right)
    {
      NewX = VirtualScreen.right-w;
      if (NewX < 0) NewX = 0;
    }
    if (y+h > VirtualScreen.bottom)
    {
      NewY =  VirtualScreen.bottom-h;
      if (NewY < 0) NewY = 0;
    }
    if ((NewX!=x) || (NewY!=y))
      SetWindowPos(HVTWin,HWND_TOP,NewX,NewY,w,h,SWP_NOSIZE);

    Point.x = 0;
    Point.y = ScreenHeight;
    ClientToScreen(HVTWin,&Point);
    CompletelyVisible = (Point.y <= VirtualScreen.bottom);
    if (IsCaretOn()) CaretOn();
  }
}

void PaintWindow(HDC PaintDC, RECT PaintRect, BOOL fBkGnd,
		 int* Xs, int* Ys, int* Xe, int* Ye)
//  Paint window with background color &
//  convert paint region from window coord. to screen coord.
//  Called from WM_PAINT handler
//    PaintRect: Paint region in window coordinate
//    Return:
//	*Xs, *Ys: upper left corner of the region
//		    in screen coord.
//	*Xe, *Ye: lower right
{
  if (VTDC!=NULL)
	DispReleaseDC();
  VTDC = PaintDC;
  DCPrevFont = SelectObject(VTDC, VTFont[0]);
  DispInitDC();

  if(!BGEnable && fBkGnd)
    FillRect(VTDC, &PaintRect,Background);

  *Xs = PaintRect.left / FontWidth + WinOrgX;
  *Ys = PaintRect.top / FontHeight + WinOrgY;
  *Xe = (PaintRect.right-1) / FontWidth + WinOrgX;
  *Ye = (PaintRect.bottom-1) / FontHeight + WinOrgY;
}

void DispEndPaint(void)
{
  if (VTDC==NULL) return;
  SelectObject(VTDC,DCPrevFont);
  VTDC = NULL;
}

void DispClearWin(void)
{
  InvalidateRect(HVTWin,NULL,FALSE);

  ScrollCount = 0;
  dScroll = 0;
  if (WinHeight > NumOfLines)
    DispChangeWinSize(NumOfColumns,NumOfLines);
  else {
    if ((NumOfLines==WinHeight) && (ts.EnableScrollBuff>0))
    {
      SetScrollRange(HVTWin,SB_VERT,0,1,FALSE);
    }
    else
      SetScrollRange(HVTWin,SB_VERT,0,NumOfLines-WinHeight,FALSE);

    SetScrollPos(HVTWin,SB_HORZ,0,TRUE);
    SetScrollPos(HVTWin,SB_VERT,0,TRUE);
  }
  if (IsCaretOn()) CaretOn();
}

void DispChangeBackground(void)
{
	DispReleaseDC();

	UpdateBGBrush();

	InvalidateRect(HVTWin,NULL,TRUE);
}

void DispChangeWin(void)
{
  /* Change window caption */
  ChangeTitle();

  /* Menu bar / Popup menu */
  SwitchMenu();

  SwitchTitleBar();

  /* Change caret shape */
  ChangeCaret();

  /* change background color */
  DispChangeBackground();
}

void DispInitDC(void)
{
  if (VTDC==NULL)
  {
    VTDC = GetDC(HVTWin);
    DCPrevFont = SelectObject(VTDC, VTFont[0]);
  }
  else
    SelectObject(VTDC, VTFont[0]);

  SetTextColor(VTDC, BGVTColor[0]);
  SetBkColor(VTDC, BGVTColor[1]);

  SetBkMode(VTDC,OPAQUE);
  DCAttr = DefCharAttr;
}

void DispReleaseDC(void)
{
  if (VTDC==NULL) return;
  SelectObject(VTDC, DCPrevFont);
  ReleaseDC(HVTWin,VTDC);
  VTDC = NULL;
}

/**
 * シーケンスのcolor_indexをANSIColor[]のindexへ変換する
 *
 * 8色モード
 *	 原色(明るい色)が使われる
 * 16色以上でPC-style 16 colors以外
 *   引数 color_index は ANSIColor[] のindexと等しい
 * PC-style 16 colors (pcbold16が0以外の時)
 *	 pcbold16_bright が 0 のとき
 *		少し暗い色
 *	 pcbold16_bright が 0 以外(Bold属性 or Blonk属性)のとき
 *		文字色属性(0-7)の組み合わせで明るい文字色を表す
 *
 * @param color_index			色番号
 * @param pcbold16				0/0以外 = 16 color mode PC Styleではない/である
 * @param pcbold16_bright		0/0以外 = 色を明るくしない/する
 * @return ANSIColor[]のindex (ANSI color 256色のindex)
 */
static int Get16ColorIndex(int color_index_256, int pcbold16, int pcbold16_bright)
{
	if ((ts.ColorFlag & CF_FULLCOLOR) == 0) {
		// 8色モード
		//		input	output
		//		0    	0			黒,Black
		//		1-7  	9-14		明るい色,原色 (Bright color)
		if (color_index_256 == 0) {
			return 0;
		} else if (color_index_256 < 8) {
			return color_index_256 + 8;
		} else {
			return color_index_256;
		}
	}
	else if (pcbold16) {
		// 16 color mode PC Style
		if (color_index_256 < 8) {
			if (pcbold16_bright) {
				return color_index_256 + 8;
			}
			else {
				return color_index_256;
			}
		}
		else {
			return color_index_256;
		}
	}
	else {
		// 16/256色
		return color_index_256;
	}
}

/**
 *	文字のアトリビュートと反転(領域選択)状態から描画色などを得る
 *
 *	@param[in]	Attr		文字属性情報
 *	@param[in]	reverse		反転(領域選択)状態
 *	@param[out]	fore_color	文字前景色
 *	@param[out]	back_color	文字背景色
 *	@param[ot]	alpha		混合割合
 */
static void GetDrawAttr(const TCharAttr *Attr, BOOL _reverse, COLORREF *fore_color, COLORREF *back_color, BYTE *_alpha)
{
	COLORREF TextColor, BackColor;
	WORD AttrFlag;	// Attr + Flag
	WORD Attr2Flag;	// Attr2 + Flag
	BOOL reverse;
	const BOOL use_normal_bg_color = ts.UseNormalBGColor;
	vtdisp_work_t *w = &vtdisp_work;
	BYTE alpha;

	// ts.ColorFlag と Attr を合成した Attr を作る
	AttrFlag = 0;
	AttrFlag |= ((ts.ColorFlag & CF_URLCOLOR) && (Attr->Attr & AttrURL)) ? AttrURL : 0;
	AttrFlag |= ((ts.ColorFlag & CF_UNDERLINE) && (Attr->Attr & AttrUnder)) ? AttrUnder : 0;
	AttrFlag |= ((ts.ColorFlag & CF_BOLDCOLOR) && (Attr->Attr & AttrBold)) ? AttrBold : 0;
	AttrFlag |= ((ts.ColorFlag & CF_BLINKCOLOR) && (Attr->Attr & AttrBlink)) ? AttrBlink : 0;
	AttrFlag |= (Attr->Attr & AttrReverse) ? AttrReverse : 0;
	Attr2Flag = 0;
	Attr2Flag |= ((ts.ColorFlag & CF_ANSICOLOR) && (Attr->Attr2 & Attr2Fore)) ? Attr2Fore : 0;
	Attr2Flag |= ((ts.ColorFlag & CF_ANSICOLOR) && (Attr->Attr2 & Attr2Back)) ? Attr2Back : 0;

	// 反転
	reverse = FALSE;
	if (_reverse) {
		reverse = TRUE;
	}
	if ((AttrFlag & AttrReverse) != 0) {
		reverse = reverse ? FALSE : TRUE;
	}
	if ((ts.ColorFlag & CF_REVERSEVIDEO) != 0) {
		reverse = reverse ? FALSE : TRUE;
	}

	// 色を決定する
	TextColor = BGVTColor[0];
	BackColor = BGVTColor[1];
	if ((AttrFlag & (AttrURL | AttrUnder | AttrBold | AttrBlink)) == 0) {
		if (!reverse) {
			TextColor = BGVTColor[0];
			BackColor = BGVTColor[1];
		}
		else {
			if ((ts.ColorFlag & CF_REVERSECOLOR) == 0) {
				TextColor = BGVTColor[1];
				BackColor = BGVTColor[0];
			}
			else {
				// 反転属性色が有効
				TextColor = BGVTReverseColor[0];
				BackColor = BGVTReverseColor[1];
			}
		}
	} else if (AttrFlag & AttrBlink) {
		if (!reverse) {
			TextColor = BGVTBlinkColor[0];
			if (!use_normal_bg_color) {
				BackColor = BGVTBlinkColor[1];
			} else {
				BackColor = BGVTColor[1];
			}
		} else {
			if (!use_normal_bg_color) {
				TextColor = BGVTBlinkColor[1];
			} else {
				TextColor = BGVTColor[1];
			}
			BackColor = BGVTBlinkColor[0];
		}
	} else if (AttrFlag & AttrBold) {
		if (!reverse) {
			TextColor = BGVTBoldColor[0];
			if (!use_normal_bg_color) {
				BackColor = BGVTBoldColor[1];
			} else {
				BackColor = BGVTColor[1];
			}
		} else {
			if (!use_normal_bg_color) {
				TextColor = BGVTBoldColor[1];
			} else {
				TextColor = BGVTColor[1];
			}
			BackColor = BGVTBoldColor[0];
		}
	} else if (AttrFlag & AttrUnder) {
		if (!reverse) {
			TextColor = BGVTUnderlineColor[0];
			if (!use_normal_bg_color) {
				BackColor = BGVTUnderlineColor[1];
			} else {
				BackColor = BGVTColor[1];
			}
		} else {
			if (!use_normal_bg_color) {
				TextColor = BGVTUnderlineColor[1];
			} else {
				TextColor = BGVTColor[1];
			}
			BackColor = BGVTUnderlineColor[0];
		}
	} else if (AttrFlag & AttrURL) {
		if (!reverse) {
			TextColor = BGURLColor[0];
			if (!use_normal_bg_color) {
				BackColor = BGURLColor[1];
			} else {
				BackColor = BGVTColor[1];
			}
		} else {
			if (!use_normal_bg_color) {
				TextColor = BGURLColor[1];
			} else {
				TextColor = BGVTColor[1];
			}
			BackColor = BGURLColor[0];
		}
	}

	//	ANSIColor/Fore
	if (Attr2Flag & Attr2Fore) {
		const int index = Get16ColorIndex(Attr->Fore, ts.ColorFlag & CF_PCBOLD16, AttrFlag & AttrBold);
		if (!reverse) {
			TextColor = ANSIColor[index];
		}
		else {
			BackColor = ANSIColor[index];
		}
	}

	//	ANSIColor/Back
	if (Attr2Flag & Attr2Back) {
		const int index = Get16ColorIndex(Attr->Back, ts.ColorFlag & CF_PCBOLD16, AttrFlag & AttrBlink);
		if (!reverse) {
			BackColor = ANSIColor[index];
		}
		else {
			TextColor = ANSIColor[index];
		}
	}

	// UseTextColor=on のときの処理
	//	背景色(Back)を考慮せずに文字色(Fore)だけを変更するアプリを使っていて
	//	文字が見えない状態になったら通常文字色か反転属性文字色を使用する
	if ((ts.ColorFlag & CF_USETEXTCOLOR) !=0) {
		if ((Attr2Flag & Attr2Fore) && (Attr2Flag & Attr2Back)) {
			const int is_target_color = (Attr->Fore == IdFore || Attr->Fore == IdBack || Attr->Fore == 15);
//			const int is_target_color = 1;
			if (Attr->Fore == Attr->Back && is_target_color) {
				if (!reverse) {
					TextColor = BGVTColor[0];
					BackColor = BGVTColor[1];
				}
				else {
					TextColor = BGVTReverseColor[0];
					BackColor = BGVTReverseColor[1];
				}
			}
		}
	}

	if (BackColor == BGVTColor[1]) {
		// 通常(アトリビュートなし)のback
		alpha = w->alpha_vtback;
	}
	else if (!reverse) {
		// 一般(反転属性(SGR 4)以外)のback
		alpha = w->alpha_back;
	}
	else {
		// 反転属性(SGR 4)のback
		alpha = BGReverseTextAlpha;
	}

	*fore_color = TextColor;
	*back_color = BackColor;
	*_alpha = alpha;
}

/**
 * Setup device context
 *   Attr: character attributes
 *   Reverse: true if text is selected (reversed) by mouse
 */
void DispSetupDC(TCharAttr Attr, BOOL Reverse)
{
	vtdisp_work_t *w = &vtdisp_work;
	COLORREF TextColor, BackColor;
	BYTE alpha;

	GetDrawAttr(&Attr, Reverse, &TextColor, &BackColor, &alpha);

	if (VTDC == NULL)
		DispInitDC();

	// フォント設定
	if (((ts.FontFlag & FF_URLUNDERLINE) && (Attr.Attr & AttrURL)) ||
		((ts.FontFlag & FF_UNDERLINE) && (Attr.Attr & AttrUnder))) {
		SelectObject(VTDC, VTFont[(Attr.Attr & AttrFontMask) | AttrUnder]);
	}
	else {
		SelectObject(VTDC, VTFont[Attr.Attr & (AttrBold|AttrSpecial)]);
	}

	// 色設定
	SetTextColor(VTDC, TextColor);
	SetBkColor(VTDC, BackColor);

	w->DCBackColor = BackColor;
	w->DCBackAlpha = alpha;
}

/**
 *	1行描画 ANSI
 */
void DrawStrA(HDC DC, HDC BGDC, const char *StrA, const char *WidthInfo, int Count, int font_width, int font_height, int Y, int *X)
{
	int Dx[TermWidthMax];
	int HalfCharCount = 0;
	int i;
	int width;
	int height;
	BOOL direct_draw;
	BYTE alpha = 0;
	vtdisp_work_t *w = &vtdisp_work;

	{
		const char *wp = WidthInfo;
		int *d = Dx;
		for (i = 0; i < Count; i++) {
			int w = *wp++;
			int j;
			*d++ = font_width;
			for (j = 0; j < w - 1; j++) {
				*d++ = font_width;
				wp++;
				i++;
			}
			HalfCharCount += w;
		}
	}

	direct_draw = FALSE;
	if (BGDC == NULL) {
		// ワークの指定がないと直接描画
		direct_draw = TRUE;
	}
	else {
		alpha = w->DCBackAlpha;
		if (alpha == 255) {
			direct_draw = TRUE;
		}
	}

	// テキスト描画領域
	width = HalfCharCount * font_width;
	height = font_height;
	if (direct_draw) {
		RECT RText;
		SetRect(&RText, *X, Y, *X + width, Y + height);

		ExtTextOutA(DC, *X + ts.FontDX, Y + ts.FontDY, ETO_CLIPPED | ETO_OPAQUE, &RText, StrA, Count, &Dx[0]);
	}
	else {
		HFONT hPrevFont;
		RECT rect;

		// BGDCに背景画像を描画
		const COLORREF BackColor = GetBkColor(DC);
		DrawTextBGImage(BGDC, *X, Y, width, height, BackColor, alpha);

		// BGDCに文字を描画
		hPrevFont = SelectObject(BGDC, GetCurrentObject(DC, OBJ_FONT));
		SetTextColor(BGDC, GetTextColor(DC));
		SetBkColor(BGDC, GetBkColor(DC));
		SetRect(&rect, 0, 0, 0 + width, 0 + height);
		ExtTextOutA(BGDC, ts.FontDX, ts.FontDY, ETO_CLIPPED, &rect, StrA, Count, &Dx[0]);
		SelectObject(BGDC, hPrevFont);

		// BGDCに描画した文字をWindowに貼り付け
		BitBlt(DC, *X, Y, width, height, BGDC, 0, 0, SRCCOPY);

		SelectObject(BGDC, hPrevFont);
	}

	*X += width;

	if (w->debug_drawbox_text) {
		DrawBox(DC, *X, Y, width, height, RGB(0xff, 0, 0));
	}
}

static void DrawChar(HDC hDC, HDC BGDC, int x, int y, const wchar_t *str, size_t len, int cell)
{
	SIZE char_size;
	HDC char_dc;
	HBITMAP bitmap;
	HBITMAP prev_bitmap;
	RECT rc;
	vtdisp_work_t *w = &vtdisp_work;
	int width;
	int height;

	GetTextExtentPoint32W(hDC, str, (int)len, &char_size);

	char_dc = CreateCompatibleDC(hDC);
	SetTextColor(char_dc, GetTextColor(hDC));
	SetBkColor(char_dc, GetBkColor(hDC));
	SelectObject(char_dc, GetCurrentObject(hDC, OBJ_FONT));
	bitmap = CreateCompatibleBitmap(hDC, char_size.cx, char_size.cy);
	prev_bitmap = SelectObject(char_dc, bitmap);

	rc.top = 0;
	rc.left = 0;
	rc.right = char_size.cx;
	rc.bottom = char_size.cy;
	ExtTextOutW(char_dc, 0, 0, ETO_OPAQUE, &rc, str, (UINT)len, 0);

	// 横をcell幅(cell*FontWidth pixel)に拡大/縮小して描画
	width = cell * FontWidth;
	height = char_size.cy;
	if (pTransparentBlt == NULL || BGDC == NULL || w->DCBackAlpha == 255) {
		// 直接描画
		SetStretchBltMode(hDC, COLORONCOLOR);
		StretchBlt(hDC, x, y, width, height, char_dc, 0, 0, char_size.cx, char_size.cy, SRCCOPY);
	}
	else {
		// BGDCに背景画像を描画
		const COLORREF BackColor = GetBkColor(hDC);
		DrawTextBGImage(BGDC, x, y, width, height, BackColor, w->DCBackAlpha);

		// BGDCに文字を描画
		SetStretchBltMode(hDC, COLORONCOLOR);
		pTransparentBlt(BGDC, 0, 0, width, height, char_dc, 0, 0, char_size.cx, char_size.cy, GetBkColor(hDC));

		// BGDCに描画した文字をWindowに貼り付け
		BitBlt(hDC, x, y, width, height, BGDC, 0, 0, SRCCOPY);
	}

	SelectObject(char_dc, prev_bitmap);
	DeleteObject(bitmap);
	DeleteDC(char_dc);
}

static void DrawStrWSub(HDC DC, HDC BGDC, const wchar_t *StrW, const int *Dx,
						int Count, int cells, int font_width, int font_height,
						int Y, int *X)
{
	int HalfCharCount = cells;	// セル数
	int width;
	int height;
	BOOL direct_draw;
	BYTE alpha = 0;
	vtdisp_work_t *w = &vtdisp_work;

	direct_draw = FALSE;
	if (BGDC == NULL) {
		// ワークの指定がないと直接描画
		direct_draw = TRUE;
	}
	else {
		alpha = w->DCBackAlpha;
		if (alpha == 255) {
			direct_draw = TRUE;
		}
	}
	// テキスト描画領域
	width = HalfCharCount * font_width;
	height = font_height;
	if (direct_draw) {
		RECT RText;
		SetRect(&RText, *X, Y, *X + width, Y + height);
		ExtTextOutW(DC, *X + ts.FontDX, Y + ts.FontDY, ETO_CLIPPED | ETO_OPAQUE, &RText, StrW, Count, &Dx[0]);
	}
	else {
		HFONT hPrevFont;
		RECT rect;

		// BGDCに背景画像を描画
		const COLORREF BackColor = GetBkColor(DC);
		DrawTextBGImage(BGDC, *X, Y, width, height, BackColor, alpha);

		// BGDCに文字を描画
		hPrevFont = SelectObject(BGDC, GetCurrentObject(DC, OBJ_FONT));
		SetTextColor(BGDC, GetTextColor(DC));
		SetBkColor(BGDC, GetBkColor(DC));
		SetRect(&rect, 0, 0, 0 + width, 0 + height);
		ExtTextOutW(BGDC, ts.FontDX, ts.FontDY, ETO_CLIPPED, &rect, StrW, Count, &Dx[0]);
		SelectObject(BGDC, hPrevFont);

		// BGDCに描画した文字をWindowに貼り付け
		BitBlt(DC, *X, Y, width, height, BGDC, 0, 0, SRCCOPY);
	}

	*X += width;
}


/**
 *	1行描画 Unicode
 *		Windows 95 にも ExtTextOutW() は存在するが
 *		動作が異なるようだ
 *		TODO 文字間に対応していない?
 *
 *	@param  DC				描画先DC
 *	@param  BGDC			描画先ワークDC
 *							NULLの時ワークなし(=背景描画なし)
 *							プリンタへの出力の時は常にNULL
 *	@param	StrW			出力文字 (wchar_t)
 *	@param	cells[]			出力文字のcell数
 *							1		半角文字
 *							0		結合文字, Nonspacing Mark
 *							2+		全角文字, 半角 + Spacing Mark
 *	@param	len				文字数
 *
 *	例
 *		len=2, L"AB"
 *					0		1		2
 *			StrW	'A'		'B'
 *			cells	1		1
 *
 *		len=2, U+307B U+309A (L'ほ' + L'゜')
 *					0		1		2
 *			StrW	U+307B	U+309A
 *			cells	0		2
 *
 *		 len=4
 *					吉(つちよし)  野	 家
 *					0			  1		 2
 *			StrW	0xd842 0xdfb7 0x91ce 0x5bb6
 *			cells	0	   2	  2		 2
 *
 */
void DrawStrW(HDC DC, HDC BGDC, const wchar_t *StrW, const char *cells, int len, int font_width, int font_height,
			  int Y, int *X)
{
	int Dx[TermWidthMax];
	int cell = 0;
	int i;
	vtdisp_work_t *w = &vtdisp_work;
	int sx = *X;

	if (len <= 0) {
		return;
	}

	for (i = 0; i < len; i++) {
		cell += cells[i];
		Dx[i] = cells[i] * font_width;
	}

	if (w->font_resize_enable) {
		int start_idx = 0;
		int cell_count = 0;
		int wchar_count = 0;
		int zero_count = 0;
		for (i = 0; i < len; i++) {
			if (cells[i] == 0) {
				if (cell_count != 0) {
					DrawStrWSub(DC, BGDC, &StrW[start_idx], &Dx[start_idx], wchar_count, cell_count, font_width, font_height, Y, X);
					start_idx = i;
					cell_count = 0;
					wchar_count = 0;
					zero_count = 0;
				}
				wchar_count++;
				zero_count++;
			}
			else {
				SIZE size;
				GetTextExtentPoint32W(DC, &StrW[i - zero_count], 1 + zero_count, &size);
				if (zero_count == 0 && ((size.cx == Dx[i]) || (size.cx == Dx[i] + 1))) {
					wchar_count++;
					cell_count += cells[i];
				}
				else {
					if (cell_count > 0) {
						DrawStrWSub(DC, BGDC, &StrW[start_idx], &Dx[start_idx], wchar_count, cell_count,
									font_width, font_height, Y, X);
						start_idx += wchar_count;
					}
					DrawChar(DC, BGDC, *X, Y, &StrW[i - zero_count], 1 + zero_count, cells[i]);
					*X += cells[i] * FontWidth;
					start_idx += 1 + zero_count;
					zero_count = 0;
					cell_count = 0;
					wchar_count = 0;
				}
			}
		}
		if (cell_count != 0) {
			DrawStrWSub(DC, BGDC, &StrW[start_idx], &Dx[start_idx], wchar_count, cell_count, font_width, font_height, Y,
						X);
		}
	}
	else {
		DrawStrWSub(DC, BGDC, StrW, Dx, len, cell, font_width, font_height, Y, X);
	}

	if (w->debug_drawbox_text) {
		int width = cell * font_width;
		int height = font_height;
		DrawBox(DC, sx, Y, width, height, RGB(0, 255, 0));
	}
}

/**
 *	Display a string
 *	@param   	Buff	points the string
 *	@param   	Y		vertical position in window cordinate
 *  @param[in]	*X		horizontal position
 *  @param[out]	*X		horizontal position shifted by the width of the string
 */
void DispStrA(const char *Buff, const char *WidthInfo, int Count, int Y, int* X)
{
	HDC BGDC = BGEnable ? hdcBGBuffer : NULL;
	DrawStrA(VTDC, BGDC, Buff, WidthInfo, Count, FontWidth, FontHeight, Y, X);
}

/**
 *	DispStr() の wchar_t版
 */
void DispStrW(const wchar_t *StrW, const char *WidthInfo, int Count, int Y, int* X)
{
	HDC BGDC = BGEnable ? hdcBGBuffer : NULL;
	DrawStrW(VTDC, BGDC, StrW, WidthInfo, Count, FontWidth, FontHeight, Y, X);
}

BOOL DispDeleteLines(int Count, int YEnd)
// return value:
//	 TRUE  - screen is successfully updated
//   FALSE - screen is not updated
{
  RECT R;

  if (Active && CompletelyVisible &&
      (YEnd+1-WinOrgY <= WinHeight))
  {
	R.left = 0;
	R.right = ScreenWidth;
	R.top = (CursorY-WinOrgY)*FontHeight;
	R.bottom = (YEnd+1-WinOrgY)*FontHeight;
//  ScrollWindow(HVTWin,0,-FontHeight*Count,&R,&R);
	BGScrollWindow(HVTWin,0,-FontHeight*Count,&R,&R);
	UpdateWindow(HVTWin);
	return TRUE;
  }
  else
	return FALSE;
}

BOOL DispInsertLines(int Count, int YEnd)
// return value:
//	 TRUE  - screen is successfully updated
//   FALSE - screen is not updated
{
  RECT R;

  if (Active && CompletelyVisible &&
      (CursorY >= WinOrgY))
  {
    R.left = 0;
    R.right = ScreenWidth;
    R.top = (CursorY-WinOrgY)*FontHeight;
    R.bottom = (YEnd+1-WinOrgY)*FontHeight;
//  ScrollWindow(HVTWin,0,FontHeight*Count,&R,&R);
    BGScrollWindow(HVTWin,0,FontHeight*Count,&R,&R);
	UpdateWindow(HVTWin);
    return TRUE;
  }
  else
	return FALSE;
}

BOOL IsLineVisible(int* X, int* Y)
//  Check the visibility of a line
//	called from UpdateStr()
//    *X, *Y: position of a character in the line. screen coord.
//    Return: TRUE if the line is visible.
//	*X, *Y:
//	  If the line is visible
//	    position of the character in window coord.
//	  Otherwise
//	    no change. same as input value.
{
  if ((dScroll != 0) &&
      (*Y>=SRegionTop) &&
      (*Y<=SRegionBottom))
  {
    *Y = *Y + dScroll;
    if ((*Y<SRegionTop) || (*Y>SRegionBottom))
      return FALSE;
  }

  if ((*Y<WinOrgY) ||
      (*Y>=WinOrgY+WinHeight))
    return FALSE;

  /* screen coordinate -> window coordinate */
  *X = (*X-WinOrgX)*FontWidth;
  *Y = (*Y-WinOrgY)*FontHeight;
  return TRUE;
}

//-------------- scrolling functions --------------------

void AdjustScrollBar(void) /* called by ChangeWindowSize() */
{
  LONG XRange, YRange;
  int ScrollPosX, ScrollPosY;

  if (NumOfColumns-WinWidth>0)
    XRange = NumOfColumns-WinWidth;
  else
    XRange = 0;

  if (BuffEnd-WinHeight>0)
    YRange = BuffEnd-WinHeight;
  else
    YRange = 0;

  ScrollPosX = GetScrollPos(HVTWin,SB_HORZ);
  ScrollPosY = GetScrollPos(HVTWin,SB_VERT);
  if (ScrollPosX > XRange)
    ScrollPosX = XRange;
  if (ScrollPosY > YRange)
    ScrollPosY = YRange;

  WinOrgX = ScrollPosX;
  WinOrgY = ScrollPosY-PageStart;
  NewOrgX = WinOrgX;
  NewOrgY = WinOrgY;

  DontChangeSize = TRUE;

  SetScrollRange(HVTWin,SB_HORZ,0,XRange,FALSE);

  if ((YRange == 0) && (ts.EnableScrollBuff>0))
  {
    SetScrollRange(HVTWin,SB_VERT,0,1,FALSE);
  }
  else {
    SetScrollRange(HVTWin,SB_VERT,0,YRange,FALSE);
  }

  SetScrollPos(HVTWin,SB_HORZ,ScrollPosX,TRUE);
  SetScrollPos(HVTWin,SB_VERT,ScrollPosY,TRUE);

  DontChangeSize = FALSE;
}

void DispScrollToCursor(int CurX, int CurY)
{
  if (CurX < NewOrgX)
    NewOrgX = CurX;
  else if (CurX >= NewOrgX+WinWidth)
    NewOrgX = CurX + 1 - WinWidth;

  if (CurY < NewOrgY)
    NewOrgY = CurY;
  else if (CurY >= NewOrgY+WinHeight)
    NewOrgY = CurY + 1 - WinHeight;
}

void DispScrollNLines(int Top, int Bottom, int Direction)
//  Scroll a region of the window by Direction lines
//    updates window if necessary
//  Top: top line of scroll region
//  Bottom: bottom line
//  Direction: +: forward, -: backward
{
	if (((dScroll * Direction < 0) || (dScroll * Direction > 0)) &&
		(((SRegionTop != Top) || (SRegionBottom != Bottom)))) {
		DispUpdateScroll();
	}
	SRegionTop = Top;
	SRegionBottom = Bottom;
	dScroll = dScroll + Direction;
	if (Direction > 0)
		DispCountScroll(Direction);
	else
		DispCountScroll(-Direction);
}

void DispCountScroll(int n)
{
  ScrollCount = ScrollCount + n;
  if (ScrollCount>=ts.ScrollThreshold) DispUpdateScroll();
}

void DispUpdateScroll(void)
{
  int d;
  RECT R;

  ScrollCount = 0;

  /* Update partial scroll */
  if (dScroll != 0)
  {
    d = dScroll * FontHeight;
    R.left = 0;
    R.right = ScreenWidth;
    R.top = (SRegionTop-WinOrgY)*FontHeight;
    R.bottom = (SRegionBottom+1-WinOrgY)*FontHeight;
//  ScrollWindow(HVTWin,0,-d,&R,&R);
    BGScrollWindow(HVTWin,0,-d,&R,&R);

    if ((SRegionTop==0) && (dScroll>0))
	{ // update scroll bar if BuffEnd is changed
	  if ((BuffEnd==WinHeight) &&
          (ts.EnableScrollBuff>0))
        SetScrollRange(HVTWin,SB_VERT,0,1,TRUE);
      else
        SetScrollRange(HVTWin,SB_VERT,0,BuffEnd-WinHeight,FALSE);
      SetScrollPos(HVTWin,SB_VERT,WinOrgY+PageStart,TRUE);
	}
    dScroll = 0;
  }

  /* Update normal scroll */
  if (NewOrgX < 0) NewOrgX = 0;
  if (NewOrgX>NumOfColumns-WinWidth)
    NewOrgX = NumOfColumns-WinWidth;
  if (NewOrgY < -PageStart) NewOrgY = -PageStart;
  if (NewOrgY>BuffEnd-WinHeight-PageStart)
    NewOrgY = BuffEnd-WinHeight-PageStart;

  /* 最下行でだけ自動スクロールする設定の場合
     NewOrgYが変化していなくてもバッファ行数が変化するので更新する */
  if (ts.AutoScrollOnlyInBottomLine != 0)
  {
    if ((BuffEnd==WinHeight) &&
        (ts.EnableScrollBuff>0))
      SetScrollRange(HVTWin,SB_VERT,0,1,TRUE);
    else
      SetScrollRange(HVTWin,SB_VERT,0,BuffEnd-WinHeight,FALSE);
    SetScrollPos(HVTWin,SB_VERT,NewOrgY+PageStart,TRUE);
  }

  if ((NewOrgX==WinOrgX) &&
      (NewOrgY==WinOrgY)) return;

  if (NewOrgX==WinOrgX)
  {
    d = (NewOrgY-WinOrgY) * FontHeight;
//  ScrollWindow(HVTWin,0,-d,NULL,NULL);
    BGScrollWindow(HVTWin,0,-d,NULL,NULL);
  }
  else if (NewOrgY==WinOrgY)
  {
    d = (NewOrgX-WinOrgX) * FontWidth;
//  ScrollWindow(HVTWin,-d,0,NULL,NULL);
    BGScrollWindow(HVTWin,-d,0,NULL,NULL);
  }
  else
    InvalidateRect(HVTWin,NULL,TRUE);

  /* Update scroll bars */
  if (NewOrgX!=WinOrgX)
    SetScrollPos(HVTWin,SB_HORZ,NewOrgX,TRUE);

  if (ts.AutoScrollOnlyInBottomLine == 0 && NewOrgY!=WinOrgY)
  {
    if ((BuffEnd==WinHeight) &&
        (ts.EnableScrollBuff>0))
      SetScrollRange(HVTWin,SB_VERT,0,1,TRUE);
    else
      SetScrollRange(HVTWin,SB_VERT,0,BuffEnd-WinHeight,FALSE);
    SetScrollPos(HVTWin,SB_VERT,NewOrgY+PageStart,TRUE);
  }

  WinOrgX = NewOrgX;
  WinOrgY = NewOrgY;

  if (IsCaretOn()) CaretOn();
}

void DispScrollHomePos(void)
{
  NewOrgX = 0;
  NewOrgY = 0;
  DispUpdateScroll();
}

void DispAutoScroll(POINT p)
{
  int X, Y;

  X = (p.x + FontWidth / 2) / FontWidth;
  Y = p.y / FontHeight;
  if (X<0)
    NewOrgX = WinOrgX + X;
  else if (X>=WinWidth)
    NewOrgX = NewOrgX + X - WinWidth + 1;
  if (Y<0)
    NewOrgY = WinOrgY + Y;
  else if (Y>=WinHeight)
    NewOrgY = NewOrgY + Y - WinHeight + 1;

  DispUpdateScroll();
}

void DispHScroll(int Func, int Pos)
{
  switch (Func) {
	case SCROLL_BOTTOM:
      NewOrgX = NumOfColumns-WinWidth;
      break;
	case SCROLL_LINEDOWN: NewOrgX = WinOrgX + 1; break;
	case SCROLL_LINEUP: NewOrgX = WinOrgX - 1; break;
	case SCROLL_PAGEDOWN:
      NewOrgX = WinOrgX + WinWidth - 1;
      break;
	case SCROLL_PAGEUP:
      NewOrgX = WinOrgX - WinWidth + 1;
      break;
	case SCROLL_POS: NewOrgX = Pos; break;
	case SCROLL_TOP: NewOrgX = 0; break;
  }
  DispUpdateScroll();
}

void DispVScroll(int Func, int Pos)
{
  switch (Func) {
	case SCROLL_BOTTOM:
      NewOrgY = BuffEnd-WinHeight-PageStart;
      break;
	case SCROLL_LINEDOWN: NewOrgY = WinOrgY + 1; break;
	case SCROLL_LINEUP: NewOrgY = WinOrgY - 1; break;
	case SCROLL_PAGEDOWN:
      NewOrgY = WinOrgY + WinHeight - 1;
      break;
	case SCROLL_PAGEUP:
      NewOrgY = WinOrgY - WinHeight + 1;
      break;
	case SCROLL_POS: NewOrgY = Pos-PageStart; break;
	case SCROLL_TOP: NewOrgY = -PageStart; break;
  }
  DispUpdateScroll();
}

//-------------- end of scrolling functions --------

/**
 *	フォントのCharSet(LOGFONT.charlfCharSet)から
 *	表示に妥当なCodePageを得る
 */
static int GetCodePageFromFontCharSet(BYTE char_set)
{
	static const struct {
		BYTE CharSet;	// LOGFONT.lfCharSet
		int CodePage;
	} table[] = {
		{ SHIFTJIS_CHARSET,  	932 },
		{ HANGUL_CHARSET,		949 },
		{ GB2312_CHARSET,	 	936 },
		{ CHINESEBIG5_CHARSET,	950 },
		{ RUSSIAN_CHARSET,		1251 },
	};
	int i;
	for (i = 0; i < _countof(table); i++) {
		if (table[i].CharSet == char_set) {
			return table[i].CodePage;
		}
	}
	return CP_ACP;
}

/**
 *	Restore window size by double clik on caption bar
 *
 */
void DispRestoreWinSize(void)
{
  if (ts.TermIsWin>0) return;

  if ((WinWidth==NumOfColumns) && (WinHeight==NumOfLines))
  {
    if (WinWidthOld > NumOfColumns)
      WinWidthOld = NumOfColumns;
    if (WinHeightOld > BuffEnd)
      WinHeightOld = BuffEnd;
    DispChangeWinSize(WinWidthOld,WinHeightOld);
  }
  else {
    SaveWinSize = TRUE;
    DispChangeWinSize(NumOfColumns,NumOfLines);
  }
}

/**
 *	WM_MOVE 時にコールされる
 */
void DispSetWinPos(void)
{
	int CaretX, CaretY;
	POINT Point;

	if (CanUseIME() && (ts.IMEInline > 0))
	{
		CaretX = (CursorX-WinOrgX)*FontWidth;
		CaretY = (CursorY-WinOrgY)*FontHeight;
		/* set IME conversion window pos. */
		SetConversionWindow(HVTWin,CaretX,CaretY);
	}

	Point.x = 0;
	Point.y = ScreenHeight;
	ClientToScreen(HVTWin,&Point);
	CompletelyVisible = (Point.y <= VirtualScreen.bottom);

	if(BGEnable) {
		if (BGSrc1.enable) {
			// 壁紙(Windowsのデスクトップ背景)が有効な時は
			// 全面書き直し
			InvalidateRect(HVTWin, NULL, FALSE);
		}
	}
}

/**
 *	ウィンドウの位置を移動する
 *	CSSunSequence()@vtterm.c から呼ばれる
 */
void DispMoveWindow(int x, int y)
{
	SetWindowPos(HVTWin, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	// WM_WINDOWPOSCHANGED を処理しないので
	// WM_MOVE が発生して DispSetWinPos() がコールされる
	// DispSetWinPos();
	return;
}

void DispSetActive(BOOL ActiveFlag)
{
	Active = ActiveFlag;
	if (Active) {
		if (IsCaretOn()) {
			CaretKillFocus(FALSE);
			// アクティブ時は無条件に再描画する
			UpdateCaretPosition(TRUE);
		}

		SetFocus(HVTWin);
		ActiveWin = IdVT;
	}
	else {
		if (CanUseIME()) {
			/* position & font of conv. window -> default */
			SetConversionWindow(HVTWin,-1,0);
		}
	}
}

int TCharAttrCmp(TCharAttr a, TCharAttr b)
{
  if (a.Attr == b.Attr &&
      a.Attr2 == b.Attr2 &&
      a.Fore == b.Fore &&
      a.Back == b.Back)
  {
    return 0;
  }
  else {
    return 1;
  }
}

void DispSetColor(unsigned int num, COLORREF color)
{
#if 0
	{
		HDC TmpDC = GetDC(NULL);
		color = GetNearestColor(TmpDC, color);
		ReleaseDC(NULL, TmpDC);
	}
#endif

	switch (num) {
	case CS_VT_NORMALFG:
		BGVTColor[0] = color;
		break;
	case CS_VT_NORMALBG:
		BGVTColor[1] = color;
		break;
	case CS_VT_BOLDFG:    BGVTBoldColor[0] = color; break;
	case CS_VT_BOLDBG:    BGVTBoldColor[1] = color; break;
	case CS_VT_BLINKFG:   BGVTBlinkColor[0] = color; break;
	case CS_VT_BLINKBG:   BGVTBlinkColor[1] = color; break;
	case CS_VT_REVERSEFG: BGVTReverseColor[0] = color; break;
	case CS_VT_REVERSEBG: BGVTReverseColor[1] = color; break;
	case CS_VT_URLFG:     BGURLColor[0] = color; break;
	case CS_VT_URLBG:     BGURLColor[1] = color; break;
	case CS_VT_UNDERFG:   BGVTUnderlineColor[0] = color; break;
	case CS_VT_UNDERBG:   BGVTUnderlineColor[1] = color; break;
	case CS_TEK_FG:       ts.TEKColor[0] = color; break;
	case CS_TEK_BG:       ts.TEKColor[1] = color; break;
	default:
		if (num <= 255) {
			if ((ts.ColorFlag & CF_FULLCOLOR) == 0) {
				// 8色モード
				int i256 = GetIndex256From16(num);
				ANSIColor[i256] = color;
			}
			else {
				// 16/256色モード
				ANSIColor[num] = color;
			}
		}
		else {
			return;
		}
		break;
	}

	UpdateBGBrush();

	if (num == CS_TEK_FG || num == CS_TEK_BG) {
		if (HTEKWin)
			InvalidateRect(HTEKWin, NULL, FALSE);
	}
	else {
		InvalidateRect(HVTWin,NULL,FALSE);
	}
}

void DispResetColor(unsigned int num)
{
	if (num == CS_UNSPEC) {
		return;
	}

	switch(num) {
	case CS_VT_NORMALFG:
		BGVTColor[0] = ts.VTColor[0];
		break;
	case CS_VT_NORMALBG:
		BGVTColor[1] = ts.VTColor[1];
		break;
	case CS_VT_BOLDFG:    BGVTBoldColor[0] = ts.VTBoldColor[0]; break;
	case CS_VT_BOLDBG:    BGVTBoldColor[1] = ts.VTBoldColor[1]; break;
	case CS_VT_BLINKFG:   BGVTBlinkColor[0] = ts.VTBlinkColor[0]; break;
	case CS_VT_BLINKBG:   BGVTBlinkColor[1] = ts.VTBlinkColor[1]; break;
	case CS_VT_REVERSEFG: BGVTReverseColor[0] = ts.VTReverseColor[0]; break;
	case CS_VT_REVERSEBG: BGVTReverseColor[1] = ts.VTReverseColor[1]; break;
	case CS_VT_URLFG:     BGURLColor[0] = ts.URLColor[0]; break;
	case CS_VT_URLBG:     BGURLColor[1] = ts.URLColor[1]; break;
	case CS_VT_UNDERFG:   BGVTUnderlineColor[0] = ts.VTUnderlineColor[0]; break;
	case CS_VT_UNDERBG:   BGVTUnderlineColor[1] = ts.VTUnderlineColor[1]; break;
	case CS_TEK_FG:
		break;
	case CS_TEK_BG:
		break;
	case CS_ANSICOLOR_ALL:
		InitColorTable(ts.ANSIColor);
		DispSetNearestColors(0, 255, NULL);
		break;
	case CS_SP_ALL:
		BGVTBoldColor[0] = ts.VTBoldColor[0];
		BGVTBlinkColor[0] = ts.VTBlinkColor[0];
		BGVTReverseColor[1] = ts.VTReverseColor[1];
		break;
	case CS_ALL:
		// VT color Foreground
		BGVTColor[0] = ts.VTColor[0];
		BGVTBoldColor[0] = ts.VTBoldColor[0];
		BGVTBlinkColor[0] = ts.VTBlinkColor[0];
		BGVTReverseColor[0] = ts.VTReverseColor[0];
		BGURLColor[0] = ts.URLColor[0];
		BGVTUnderlineColor[0] = ts.VTUnderlineColor[0];

		// VT color Background
		BGVTColor[1] = ts.VTColor[1];
		BGVTReverseColor[1] = ts.VTReverseColor[1];
		BGVTBoldColor[1] = ts.VTBoldColor[1];
		BGVTBlinkColor[1] = ts.VTBlinkColor[1];
		BGURLColor[1] = ts.URLColor[1];
		BGVTUnderlineColor[1] = ts.VTUnderlineColor[1];

		// ANSI Color / xterm 256 color
		InitColorTable(ts.ANSIColor);
		DispSetNearestColors(0, 255, NULL);
		break;
	default:
		if (num <= 15) {
			if ((ts.ColorFlag & CF_FULLCOLOR) == 0) {
				// 8色モード
				int i256 = GetIndex256From16(num);
				ANSIColor[i256] = ts.ANSIColor[num];
			}
			else {
				int i16 = GetIndex16From256(num);
				ANSIColor[num] = ts.ANSIColor[i16];
				DispSetNearestColors(num, num, NULL);
			}
		}
		else if (num <= 255) {
			ANSIColor[num] = RGB(DefaultColorTable[num][0], DefaultColorTable[num][1], DefaultColorTable[num][2]);
			DispSetNearestColors(num, num, NULL);
		}
	}

	UpdateBGBrush();

	if (num == CS_TEK_FG || num == CS_TEK_BG) {
		if (HTEKWin)
			InvalidateRect(HTEKWin, NULL, FALSE);
	}
	else {
		InvalidateRect(HVTWin,NULL,FALSE);
	}
}

COLORREF DispGetColor(unsigned int num)
{
	COLORREF color;

	switch (num) {
	case CS_VT_NORMALFG:  color = ts.VTColor[0]; break;
	case CS_VT_NORMALBG:  color = ts.VTColor[1]; break;
	case CS_VT_BOLDFG:    color = ts.VTBoldColor[0]; break;
	case CS_VT_BOLDBG:    color = ts.VTBoldColor[1]; break;
	case CS_VT_BLINKFG:   color = ts.VTBlinkColor[0]; break;
	case CS_VT_BLINKBG:   color = ts.VTBlinkColor[1]; break;
	case CS_VT_REVERSEFG: color = ts.VTReverseColor[0]; break;
	case CS_VT_REVERSEBG: color = ts.VTReverseColor[1]; break;
	case CS_VT_URLFG:     color = ts.URLColor[0]; break;
	case CS_VT_URLBG:     color = ts.URLColor[1]; break;
	case CS_VT_UNDERFG:   color = ts.VTUnderlineColor[0]; break;
	case CS_VT_UNDERBG:   color = ts.VTUnderlineColor[1]; break;
	case CS_TEK_FG:       color = ts.TEKColor[0]; break;
	case CS_TEK_BG:       color = ts.TEKColor[1]; break;
	default:
		if (num <= 255) {
			if ((ts.ColorFlag & CF_FULLCOLOR) == 0) {
				// 8色モード
				int i256 = GetIndex256From16(num);
				color = ANSIColor[i256];
			}
			else {
				// 16/256色モード
				color = ANSIColor[num];
			}
		}
		else {
			color = ANSIColor[0];
		}
		break;
	}

	return color;
}

void DispSetCurCharAttr(const TCharAttr *Attr)
{
	CurCharAttr = *Attr;
	UpdateBGBrush();
}

static void UpdateBGBrush(void)
{
	COLORREF bg_rgb;
	vtdisp_work_t *w = &vtdisp_work;

	if (Background != NULL) DeleteObject(Background);

	if ((ts.ColorFlag & CF_REVERSEVIDEO) == 0) {
		if ((CurCharAttr.Attr2 & Attr2Back) != 0) {
			const WORD AttrFlag = ((ts.ColorFlag & CF_BLINKCOLOR) && (CurCharAttr.Attr & AttrBlink)) ? AttrBlink : 0;
			const int index = Get16ColorIndex(CurCharAttr.Back, ts.ColorFlag & CF_PCBOLD16, AttrFlag & AttrBlink);
			bg_rgb = ANSIColor[index];
		}
		else {
			bg_rgb = BGVTColor[1];
		}
	}
	else {
		if ((CurCharAttr.Attr2 & Attr2Fore) != 0) {
			const WORD AttrFlag = ((ts.ColorFlag & CF_BOLDCOLOR) && (CurCharAttr.Attr & AttrBold)) ? AttrBold : 0;
			const int index = Get16ColorIndex(CurCharAttr.Fore, ts.ColorFlag & CF_PCBOLD16, AttrFlag & AttrBold);
			bg_rgb = ANSIColor[index];
		}
		else {
			bg_rgb = BGVTColor[0];
		}
	}

	w->DCBackColor = bg_rgb;
	Background = CreateSolidBrush(bg_rgb);
}

void DispShowWindow(int mode)
{
	switch (mode) {
	case WINDOW_MINIMIZE:
		ShowWindow(HVTWin, SW_MINIMIZE);
		break;
	case WINDOW_MAXIMIZE:
		ShowWindow(HVTWin, SW_MAXIMIZE);
		break;
	case WINDOW_RESTORE:
		ShowWindow(HVTWin, SW_RESTORE);
		break;
	case WINDOW_RAISE: {
		//何も起きないことあり
		//  SetWindowPos(HVTWin, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
//#define RAISE_AND_GET_FORCUS
#if defined(RAISE_AND_GET_FORCUS)
		//フォーカスを奪う
		SetForegroundWindow(HVTWin);
#else
		//フォーカスは奪わず最上面に来る
		BringWindowToTop(HVTWin);
		if (GetForegroundWindow() != HVTWin) {
			FlashWindow(HVTWin, TRUE);
		}
#endif
	}
		break;
	case WINDOW_LOWER:
		SetWindowPos(HVTWin, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		break;
	case WINDOW_REFRESH:
		InvalidateRect(HVTWin, NULL, FALSE);
		break;
	case WINDOW_TOGGLE_MAXIMIZE:
		if (IsZoomed(HVTWin)) {
			ShowWindow(HVTWin, SW_RESTORE);
		}
		else {
			ShowWindow(HVTWin, SW_MAXIMIZE);
		}
		break;
	}
}

void DispResizeWin(int w, int h) {
	RECT r;

	if (w <= 0 || h <= 0) {
		GetWindowRect(HVTWin,&r);
		if (w <= 0) {
			w = r.right - r.left;
		}
		if (h <= 0) {
			h = r.bottom - r.top;
		}
	}
	SetWindowPos(HVTWin, 0, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	AdjustSize = FALSE;
}

BOOL DispWindowIconified(void) {
	return IsIconic(HVTWin);
}

void DispGetWindowPos(int *x, int *y, BOOL client) {
	WINDOWPLACEMENT wndpl;
	POINT point;

	if (client) {
		point.x = point.y = 0;
		ClientToScreen(HVTWin, &point);
		*x = point.x;
		*y = point.y;
	}
	else {
		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(HVTWin, &wndpl);

		switch (wndpl.showCmd) {
		  case SW_SHOWMAXIMIZED:
			*x = wndpl.ptMaxPosition.x;
			*y = wndpl.ptMaxPosition.y;
			break;
		  default:
			*x = wndpl.rcNormalPosition.left;
			*y = wndpl.rcNormalPosition.top;
		}
	}

	return;
}

void DispGetWindowSize(int *width, int *height, BOOL client) {
	RECT r;

	if (client) {
		GetClientRect(HVTWin, &r);
	}
	else {
		GetWindowRect(HVTWin, &r);
	}
	*width = r.right - r.left;
	*height = r.bottom - r.top;

	return;
}

void DispGetRootWinSize(int *x, int *y, BOOL inPixels)
{
	RECT desktop, win, client;

	GetWindowRect(HVTWin, &win);
	GetClientRect(HVTWin, &client);

	GetDesktopRect(HVTWin, &desktop);

	if (inPixels) {
		*x = desktop.right - desktop.left;
		*y = desktop.bottom - desktop.top;
	}
	else {
		*x = (desktop.right - desktop.left - (win.right - win.left - client.right)) / FontWidth;
		*y = (desktop.bottom - desktop.top - (win.bottom - win.top - client.bottom)) / FontHeight;
	}
}

int DispFindClosestColor(int red, int green, int blue)
{
	int i, color, diff_r, diff_g, diff_b, diff, min;

	min = 0xfffffff;
	color = 0;

	if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
		return -1;

	for (i=0; i<256; i++) {
		diff_r = red - GetRValue(ANSIColor[i]);
		diff_g = green - GetGValue(ANSIColor[i]);
		diff_b = blue - GetBValue(ANSIColor[i]);
		diff = diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;

		if (diff < min) {
			min = diff;
			color = i;
		}
	}

	if ((ts.ColorFlag & CF_FULLCOLOR) != 0 && color < 16 && (color & 7) != 0) {
		color ^= 8;
	}
	return color;
}

void ThemeGetColor(TColorTheme *data)
{
	int i;

	wcscpy_s(data->name, _countof(data->name), L"Tera Term color theme");
	data->vt.change = TRUE;
	data->vt.enable = TRUE;
	data->vt.fg = BGVTColor[0];
	data->vt.bg = BGVTColor[1];
	data->bold.change = TRUE;
	data->bold.enable = TRUE;
	data->bold.fg = BGVTBoldColor[0];
	data->bold.bg = BGVTBoldColor[1];
	data->underline.change = TRUE;
	data->underline.enable = TRUE;
	data->underline.fg = BGVTUnderlineColor[0];
	data->underline.bg = BGVTUnderlineColor[1];
	data->blink.change = TRUE;
	data->blink.enable = TRUE;
	data->blink.fg = BGVTBlinkColor[0];
	data->blink.bg = BGVTBlinkColor[1];
	data->reverse.change = TRUE;
	data->reverse.enable = TRUE;
	data->reverse.fg = BGVTReverseColor[0];
	data->reverse.bg = BGVTReverseColor[1];
	data->url.change = TRUE;
	data->url.enable = TRUE;
	data->url.fg = BGURLColor[0];
	data->url.bg = BGURLColor[1];

	// ANSI color
	data->ansicolor.change = TRUE;
	for (i = 0; i < 16; i++) {
		data->ansicolor.color[i] = ANSIColor[i];
	}
}

void ThemeSetColor(const TColorTheme *data)
{
	int i;

	BGVTColor[0] = data->vt.fg;
	BGVTColor[1] = data->vt.bg;
	BGVTBoldColor[0] = data->bold.fg;
	BGVTBoldColor[1] = data->bold.bg;
	BGVTUnderlineColor[0] = data->underline.fg;
	BGVTUnderlineColor[1] = data->underline.bg;
	BGVTBlinkColor[0] = data->blink.fg;
	BGVTBlinkColor[1] = data->blink.bg;
	BGVTReverseColor[0] = data->reverse.fg;
	BGVTReverseColor[1] = data->reverse.bg;
	BGURLColor[0] = data->url.fg;
	BGURLColor[1] = data->url.bg;
	for (i = 0; i < 16; i++) {
		ANSIColor[i] = data->ansicolor.color[i];
	}
}

/**
 * デフォルト値で初期化する
 */
void ThemeGetBGDefault(BGTheme *bg_theme)
{
	bg_theme->BGDest.enable = FALSE;
	bg_theme->BGDest.type = BG_PICTURE;
	bg_theme->BGDest.pattern = BG_STRETCH;
	bg_theme->BGDest.color = RGB(0, 0, 0);
	bg_theme->BGDest.alpha = 255;
	bg_theme->BGDest.antiAlias = TRUE;
	bg_theme->BGDest.file[0] = 0;

	bg_theme->BGSrc1.enable = FALSE;
	bg_theme->BGSrc1.type = BG_WALLPAPER;
	bg_theme->BGSrc1.pattern = BG_STRETCH;
	bg_theme->BGSrc1.color = RGB(255, 255, 255);
	bg_theme->BGSrc1.antiAlias = TRUE;
	bg_theme->BGSrc1.alpha = 0;
	bg_theme->BGSrc1.file[0] = 0;

	bg_theme->BGSrc2.enable = FALSE;
	bg_theme->BGSrc2.type = BG_COLOR;
	bg_theme->BGSrc2.pattern = BG_STRETCH;
	bg_theme->BGSrc2.color = RGB(0, 0, 0);
	bg_theme->BGSrc2.antiAlias = TRUE;
	bg_theme->BGSrc2.alpha = 0;
	bg_theme->BGSrc2.file[0] = 0;

	bg_theme->BGReverseTextAlpha = 255;
	bg_theme->TextBackAlpha = 255;
	bg_theme->BackAlpha = 255;
}

#if 0
static void GetDefaultColor(TColorSetting *tc, const COLORREF *color, int field)
{
	tc->change = TRUE;
	tc->enable = field ? TRUE : FALSE;
	tc->fg = color[0];
	tc->bg = color[1];

	return;
}
#endif

/**
 *	デフォルト色をセットする
 */
void ThemeGetColorDefaultTS(const TTTSet *pts, TColorTheme *color_theme)
{
	int i;

	color_theme->name[0] = 0;

	color_theme->vt.fg = pts->VTColor[0];
	color_theme->vt.bg = pts->VTColor[1];

	color_theme->bold.fg = pts->VTBoldColor[0];
	color_theme->bold.bg = pts->VTBoldColor[1];

	color_theme->blink.fg = pts->VTBlinkColor[0];
	color_theme->blink.bg = pts->VTBlinkColor[1];

	color_theme->reverse.fg = pts->VTReverseColor[0];
	color_theme->reverse.bg = pts->VTReverseColor[1];

	color_theme->url.fg = pts->URLColor[0];
	color_theme->url.bg = pts->URLColor[1];

	color_theme->underline.fg = pts->VTUnderlineColor[0];
	color_theme->underline.bg = pts->VTUnderlineColor[1];

	for (i = 0 ; i < 16 ; i++) {
		int i256 = GetIndex256From16(i);
		color_theme->ansicolor.color[i256] = pts->ANSIColor[i];
	}

#if 0
	// デフォルト
	const int ColorFlag = ts.ColorFlag;
	GetDefaultColor(&(color_theme->vt), ts.VTColor, !FALSE);
	GetDefaultColor(&(color_theme->bold), ts.VTBoldColor, ColorFlag & CF_BOLDCOLOR);
	GetDefaultColor(&(color_theme->blink), ts.VTBlinkColor, ColorFlag & CF_BLINKCOLOR);
	GetDefaultColor(&(color_theme->reverse), ts.VTReverseColor, ColorFlag & CF_REVERSECOLOR);
	GetDefaultColor(&(color_theme->url), ts.URLColor, ColorFlag & CF_URLCOLOR);

	color_theme->ansicolor.change = 1;
	color_theme->ansicolor.enable = (ts.ColorFlag & CF_ANSICOLOR) != 0;
#endif
}

/**
 *	BGテーマをセットする
 */
void ThemeSetBG(const BGTheme *bg_theme)
{
	if (BGDest.fileW != NULL) {
		free(BGDest.fileW);
	}
	BGDest.fileW = _wcsdup(bg_theme->BGDest.file);
	BGDest.type = bg_theme->BGDest.type;
	BGDest.color = bg_theme->BGDest.color;
	BGDest.pattern = bg_theme->BGDest.pattern;
	BGDest.enable = bg_theme->BGDest.enable;
	BGDest.alpha = bg_theme->BGDest.alpha;

	BGSrc1.type = bg_theme->BGSrc1.type;
	BGSrc1.alpha = bg_theme->BGSrc1.alpha;
	BGSrc1.enable = bg_theme->BGSrc1.enable;

	BGSrc2.type = bg_theme->BGSrc2.type;
	BGSrc2.alpha = bg_theme->BGSrc2.alpha;
	BGSrc2.color = bg_theme->BGSrc2.color;
	BGSrc2.enable = bg_theme->BGSrc2.enable;

	BGReverseTextAlpha = bg_theme->BGReverseTextAlpha;
	{
		vtdisp_work_t *w = &vtdisp_work;
		w->alpha_vtback = bg_theme->TextBackAlpha;
		w->alpha_back = bg_theme->BackAlpha;
	}

	DecideBGEnable();
}

void ThemeGetBG(BGTheme *bg_theme)
{
	if (BGDest.fileW == NULL) {
		bg_theme->BGDest.file[0] = 0;
	}
	else {
		wcscpy_s(bg_theme->BGDest.file, _countof(bg_theme->BGDest.file), BGDest.fileW);
	}
	bg_theme->BGDest.type = BG_PICTURE;
	bg_theme->BGDest.color = BGDest.color;
	bg_theme->BGDest.pattern = BGDest.pattern;
	bg_theme->BGDest.enable = BGDest.enable;
	bg_theme->BGDest.alpha = BGDest.alpha;

	bg_theme->BGSrc1.type = BG_WALLPAPER;
	bg_theme->BGSrc1.alpha = BGSrc1.alpha;
	bg_theme->BGSrc1.enable = BGSrc1.enable;

	bg_theme->BGSrc2.type = BG_COLOR;
	bg_theme->BGSrc2.alpha = BGSrc2.alpha;
	bg_theme->BGSrc2.color = BGSrc2.color;
	bg_theme->BGSrc2.enable = BGSrc2.enable;

	bg_theme->BGReverseTextAlpha = BGReverseTextAlpha;
	{
		vtdisp_work_t *w = &vtdisp_work;
		bg_theme->TextBackAlpha = w->alpha_vtback;
		bg_theme->BackAlpha = w->alpha_back;
	}
}

/**
 *	デフォルトの色を取得
 */
void ThemeGetColorDefault(TColorTheme *color_theme)
{
	ThemeGetColorDefaultTS(&ts, color_theme);
}

void ThemeEnable(BOOL enable)
{
	vtdisp_work_t *w = &vtdisp_work;
	if (enable) {
		DecideBGEnable();
		w->bg_enable = BGEnable;
	}
	else {
		w->bg_enable = FALSE;
		BGEnable = FALSE;
	}
}

BOOL ThemeIsEnabled(void)
{
	vtdisp_work_t *w = &vtdisp_work;
	return w->bg_enable;
}

void DispEnableResizedFont(BOOL enable)
{
	vtdisp_work_t *w = &vtdisp_work;
	w->font_resize_enable = enable;
	FontReSizeEnableInit = enable;
}

BOOL DispIsResizedFont()
{
	vtdisp_work_t *w = &vtdisp_work;
	return w->font_resize_enable;
}
