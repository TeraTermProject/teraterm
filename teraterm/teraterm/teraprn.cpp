/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* TERATERM.EXE, Printing routines */
#include "teraterm.h"
#include "tttypes.h"
#include <commdlg.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "codeconv.h"
#include "vtdisp.h"

#include "tt_res.h"
#include "tmfc.h"
#include "prnabort.h"

#include "teraprn.h"

#ifdef _DEBUG
#if defined(_MSC_VER)
#define new		::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

static HDC PrintDC;
static HFONT PrnFont[AttrFontMask+1];
static int PrnFW, PrnFH;
static RECT Margin;
static COLORREF White, Black;
static int PrnX, PrnY;
static TCharAttr PrnAttr;

static BOOL Printing = FALSE;
static BOOL PrintAbortFlag = FALSE;

static CPrnAbortDlg *PrnAbortDlg;
static HWND HPrnAbortDlg;

static void PrnSetAttr(TCharAttr Attr);

/* Print Abortion Call Back Function */
static BOOL CALLBACK PrnAbortProc(HDC PDC, int Code)
{
	MSG m;

	while ((! PrintAbortFlag) && PeekMessage(&m, 0,0,0, PM_REMOVE))
		if ((HPrnAbortDlg==NULL) || (! IsDialogMessage(HPrnAbortDlg, &m))) {
			TranslateMessage(&m);
			DispatchMessage(&m);
		}

	if (PrintAbortFlag) {
		HPrnAbortDlg = NULL;
		PrnAbortDlg = NULL;
		return FALSE;
	}
	else {
		return TRUE;
	}
}

static UINT_PTR CALLBACK PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

HDC PrnBox(HWND HWin, PBOOL Sel)
{
	/* initialize PrnDlg record */
	PRINTDLGW PrnDlg = {};
	PrnDlg.lStructSize = sizeof(PrnDlg);
	PrnDlg.hwndOwner = HWin;
	PrnDlg.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_SHOWHELP | PD_ENABLEPRINTHOOK;
	if (! *Sel) {
		PrnDlg.Flags = PrnDlg.Flags | PD_NOSELECTION;	/* when there is nothing select, gray out the "Selection" radio button */
	} else {
		PrnDlg.Flags = PrnDlg.Flags | PD_SELECTION;	/* when there is something select, select the "Selection" radio button by default */
	}
	PrnDlg.nCopies = 1;
	/*
	 * Windows NT系において、印刷ダイアログにヘルプボタンを表示するため、
	 * プロシージャをフックする。
	 */
	PrnDlg.lpfnPrintHook = PrintHookProc;

	/* 'Print' dialog box */
	if (! PrintDlgW(&PrnDlg)) {
		return NULL; /* if 'Cancel' button clicked, exit */
	}
	if (PrnDlg.hDC == NULL) {
		return NULL;
	}
	*Sel = (PrnDlg.Flags & PD_SELECTION) != 0;
	return PrnDlg.hDC;
}

BOOL PrnStart(LPSTR DocumentName)
{
	DOCINFOA Doc;
	char DocName[50];
	HWND hParent;

	Printing = FALSE;
	PrintAbortFlag = FALSE;

	PrnAbortDlg = new CPrnAbortDlg();
	if (PrnAbortDlg==NULL) {
		return FALSE;
	}
	if (ActiveWin==IdVT) {
		hParent = HVTWin;
	}
	else {
		hParent = HTEKWin;
	}
	PrnAbortDlg->Create(hInst,hParent,&PrintAbortFlag,&ts);
	HPrnAbortDlg = PrnAbortDlg->GetSafeHwnd();

	SetAbortProc(PrintDC,PrnAbortProc);

	Doc.cbSize = sizeof(Doc);
	strncpy_s(DocName,sizeof(DocName),DocumentName,_TRUNCATE);
	Doc.lpszDocName = DocName;
	Doc.lpszOutput = NULL;
	Doc.lpszDatatype = NULL;
	Doc.fwType = 0;
	if (StartDocA(PrintDC, &Doc) > 0) {
		Printing = TRUE;
	}
	else {
		if (PrnAbortDlg != NULL) {
			PrnAbortDlg->DestroyWindow();
			PrnAbortDlg = NULL;
			HPrnAbortDlg = NULL;
		}
	}
	return Printing;
}

void PrnStop()
{
	if (Printing) {
		EndDoc(PrintDC);
		DeleteDC(PrintDC);
		Printing = FALSE;
	}
	if (PrnAbortDlg != NULL) {
		PrnAbortDlg->DestroyWindow();
		PrnAbortDlg = NULL;
		HPrnAbortDlg = NULL;
	}
}

int VTPrintInit(int PrnFlag)
// Initialize printing of VT window
//   PrnFlag: specifies object to be printed
//	= IdPrnScreen		Current screen
//	= IdPrnSelectedText	Selected text
//	= IdPrnScrollRegion	Scroll region
//	= IdPrnFile		Spooled file (printer sequence)
//   Return: print object ID specified by user
//	= IdPrnCancel		(user clicks "Cancel" button)
//	= IdPrnScreen		(user don't select "print selection" option)
//	= IdPrnSelectedText	(user selects "print selection")
//	= IdPrnScrollRegion	(always when PrnFlag=IdPrnScrollRegion)
//	= IdPrnFile		(always when PrnFlag=IdPrnFile)
{
	BOOL Sel;
	TEXTMETRIC Metrics;
	POINT PPI, PPI2;
	HDC DC;
	TCharAttr TempAttr = DefCharAttr;
	LOGFONTA Prnlf;

	Sel = (PrnFlag & IdPrnSelectedText)!=0;
	PrintDC = PrnBox(HVTWin,&Sel);
	if (PrintDC == NULL) {
		return (IdPrnCancel);
	}

	/* start printing */
	if (! PrnStart(ts.Title)) {
		return (IdPrnCancel);
	}

	/* initialization */
	StartPage(PrintDC);

	/* pixels per inch */
	if ((ts.VTPPI.x>0) && (ts.VTPPI.y>0)) {
		PPI = ts.VTPPI;
	}
	else {
		PPI.x = GetDeviceCaps(PrintDC,LOGPIXELSX);
		PPI.y = GetDeviceCaps(PrintDC,LOGPIXELSY);
	}

	/* left margin */
	Margin.left = (int)((float)ts.PrnMargin[0] / 100.0 * (float)PPI.x);
	/* right margin */
	Margin.right = GetDeviceCaps(PrintDC,HORZRES) -
	               (int)((float)ts.PrnMargin[1] / 100.0 * (float)PPI.x);
	/* top margin */
	Margin.top = (int)((float)ts.PrnMargin[2] / 100.0 * (float)PPI.y);
	/* bottom margin */
	Margin.bottom =  GetDeviceCaps(PrintDC,VERTRES) -
	                 (int)((float)ts.PrnMargin[3] / 100.0 * (float)PPI.y);

	/* create test font */
	memset(&Prnlf, 0, sizeof(Prnlf));

	if (ts.PrnFont[0]==0) {
		Prnlf.lfHeight = ts.VTFontSize.y;
		Prnlf.lfWidth = ts.VTFontSize.x;
		Prnlf.lfCharSet = ts.VTFontCharSet;
		strncpy_s(Prnlf.lfFaceName, sizeof(Prnlf.lfFaceName), ts.VTFont, _TRUNCATE);
	}
	else {
		Prnlf.lfHeight = ts.PrnFontSize.y;
		Prnlf.lfWidth = ts.PrnFontSize.x;
		Prnlf.lfCharSet = ts.PrnFontCharSet;
		strncpy_s(Prnlf.lfFaceName, sizeof(Prnlf.lfFaceName), ts.PrnFont, _TRUNCATE);
	}
	Prnlf.lfWeight = FW_NORMAL;
	Prnlf.lfItalic = 0;
	Prnlf.lfUnderline = 0;
	Prnlf.lfStrikeOut = 0;
	Prnlf.lfOutPrecision = OUT_CHARACTER_PRECIS;
	Prnlf.lfClipPrecision = CLIP_CHARACTER_PRECIS;
	Prnlf.lfQuality = DEFAULT_QUALITY;
	Prnlf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

	PrnFont[0] = CreateFontIndirectA(&Prnlf);

	DC = GetDC(HVTWin);
	SelectObject(DC, PrnFont[0]);
	GetTextMetrics(DC, &Metrics);
	PPI2.x = GetDeviceCaps(DC,LOGPIXELSX);
	PPI2.y = GetDeviceCaps(DC,LOGPIXELSY);
	ReleaseDC(HVTWin,DC);
	DeleteObject(PrnFont[0]); /* Delete test font */

	/* Adjust font size */
	Prnlf.lfHeight = (int)((float)Metrics.tmHeight * (float)PPI.y / (float)PPI2.y);
	Prnlf.lfWidth = (int)((float)Metrics.tmAveCharWidth * (float)PPI.x / (float)PPI2.x);

	/* Create New Fonts */

	/* Normal Font */
	Prnlf.lfWeight = FW_NORMAL;
	Prnlf.lfUnderline = 0;
	PrnFont[0] = CreateFontIndirectA(&Prnlf);
	SelectObject(PrintDC,PrnFont[0]);
	GetTextMetrics(PrintDC, &Metrics);
	PrnFW = Metrics.tmAveCharWidth;
	PrnFH = Metrics.tmHeight;
	/* Under line */
	Prnlf.lfUnderline = 1;
	PrnFont[AttrUnder] = CreateFontIndirectA(&Prnlf);

	if (ts.FontFlag & FF_BOLD) {
		/* Bold */
		Prnlf.lfUnderline = 0;
		Prnlf.lfWeight = FW_BOLD;
		PrnFont[AttrBold] = CreateFontIndirectA(&Prnlf);
		/* Bold + Underline */
		Prnlf.lfUnderline = 1;
		PrnFont[AttrBold | AttrUnder] = CreateFontIndirectA(&Prnlf);
	}
	else {
		PrnFont[AttrBold] = PrnFont[AttrDefault];
		PrnFont[AttrBold | AttrUnder] = PrnFont[AttrUnder];
	}
	/* Special font */
	Prnlf.lfWeight = FW_NORMAL;
	Prnlf.lfUnderline = 0;
	Prnlf.lfWidth = PrnFW; /* adjust width */
	Prnlf.lfHeight = PrnFH;
	Prnlf.lfCharSet = SYMBOL_CHARSET;

	strncpy_s(Prnlf.lfFaceName, sizeof(Prnlf.lfFaceName),"Tera Special", _TRUNCATE);
	PrnFont[AttrSpecial] = CreateFontIndirectA(&Prnlf);
	PrnFont[AttrSpecial | AttrBold] = PrnFont[AttrSpecial];
	PrnFont[AttrSpecial | AttrUnder] = PrnFont[AttrSpecial];
	PrnFont[AttrSpecial | AttrBold | AttrUnder] = PrnFont[AttrSpecial];

	Black = RGB(0,0,0);
	White = RGB(255,255,255);
	PrnSetAttr(TempAttr);

	PrnY = Margin.top;
	PrnX = Margin.left;

	if (PrnFlag == IdPrnScrollRegion) {
		return (IdPrnScrollRegion);
	}
	if (PrnFlag == IdPrnFile) {
		return (IdPrnFile);
	}
	if (Sel) {
		return (IdPrnSelectedText);
	}
	else {
		return (IdPrnScreen);
	}
}

static void PrnSetAttr(TCharAttr Attr)
//  Set text attribute of printing
//
{
	PrnAttr = Attr;
	SelectObject(PrintDC, PrnFont[Attr.Attr & AttrFontMask]);

	if ((Attr.Attr & AttrReverse) != 0) {
		SetTextColor(PrintDC,White);
		SetBkColor(  PrintDC,Black);
	}
	else {
		SetTextColor(PrintDC,Black);
		SetBkColor(  PrintDC,White);
	}
}

void PrnSetupDC(TCharAttr Attr, BOOL reverse)
{
	(void)reverse;
	PrnSetAttr(Attr);
}

/**
 *  Print out text
 *    Buff: points text buffer
 *    Count: number of characters to be printed
 */
void PrnOutText(const char *StrA, int Count, void *data)
{
	if (PrnX+PrnFW > Margin.right) {
		/* new line */
		PrnX = Margin.left;
		PrnY = PrnY + PrnFH;
	}
	if (PrnY+PrnFH > Margin.bottom) {
		/* next page */
		EndPage(PrintDC);
		StartPage(PrintDC);
		PrnSetAttr(PrnAttr);
		PrnY = Margin.top;
	}

	DrawStrA(PrintDC, NULL, StrA, Count, PrnFW, PrnFH, PrnY, &PrnX);
}

void PrnOutTextW(const wchar_t *StrW, const char *WidthInfo, int Count, void *data)
{
	if (PrnX+PrnFW > Margin.right) {
		/* new line */
		PrnX = Margin.left;
		PrnY = PrnY + PrnFH;
	}
	if (PrnY+PrnFH > Margin.bottom) {
		/* next page */
		EndPage(PrintDC);
		StartPage(PrintDC);
		PrnSetAttr(PrnAttr);
		PrnY = Margin.top;
	}

	DrawStrW(PrintDC, NULL, StrW, WidthInfo, Count, PrnFW, PrnFH, PrnY, &PrnX);
}

void PrnNewLine()
//  Moves to the next line in printing
{
	PrnX = Margin.left;
	PrnY = PrnY + PrnFH;
}

void VTPrintEnd()
{
	int i, j;

	EndPage(PrintDC);

	for (i = 0 ; i <= AttrFontMask ; i++) {
		for (j = i+1 ; j <= AttrFontMask ; j++) {
			if (PrnFont[j]==PrnFont[i]) {
				PrnFont[j] = NULL;
			}
		}
		if (PrnFont[i] != NULL) {
			DeleteObject(PrnFont[i]);
		}
	}

	PrnStop();
	return;
}

/* pass-thru printing */
typedef struct PrintFileTag {
	wchar_t *PrnFName;
	HANDLE HPrnFile;
	unsigned int PrnBuff[TermWidthMax*2];
	int PrnBuffCount;
	void (*FinishCallback)(PrintFile *handle);
} PrintFile;

/* printer emulation routines */
PrintFile *OpenPrnFile(void)
{
	PrintFile *p = (PrintFile *)calloc(sizeof(PrintFile),1 );
	if (p == NULL) {
		return NULL;
	}

	KillTimer(HVTWin, IdPrnStartTimer);

	wchar_t TempPath[MAX_PATH];
	GetTempPathW(_countof(TempPath), TempPath);
	wchar_t Temp[MAX_PATH];
	if (GetTempFileNameW(TempPath, L"tmp", 0, Temp) == 0) {
		free(p);
		return NULL;
	}
	p->PrnFName = _wcsdup(Temp);

	HANDLE h = CreateFileW(p->PrnFName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		free(p);
		return NULL;
	}
	SetFilePointer(h, 0, NULL, FILE_END);
	p->HPrnFile = h;

	return p;
}

static void DeletePrintFile(PrintFile *handle)
{
	if (handle->PrnFName == NULL) {
		return;
	}
	DeleteFileW(handle->PrnFName);
	free(handle->PrnFName);
	handle->PrnFName = NULL;
}

void PrnFinish(PrintFile *handle)
{
	DeletePrintFile(handle);
	free(handle);
}

/**
 *	印字用に保存していたファイルから印字する
 */
static void PrintFile_(PrintFile *handle)
{
	if (VTPrintInit(IdPrnFile)==IdPrnFile) {
		HANDLE HPrnFile = CreateFileW(handle->PrnFName,
									  GENERIC_READ, FILE_SHARE_READ, NULL,
									  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (HPrnFile != INVALID_HANDLE_VALUE) {
			char BuffA[TermWidthMax];
			size_t len_a;
			wchar_t BuffW[TermWidthMax];
			size_t len_w;
			BOOL CRFlag = FALSE;
			int c;
			unsigned int u32;
			do {
				len_a = 0;
				len_w = 0;
				do {
					DWORD NumberOfBytesRead;
					BOOL r = ReadFile(HPrnFile, &u32, sizeof(u32), &NumberOfBytesRead, NULL);
					if (r == TRUE && NumberOfBytesRead != 0) {
						// 印字継続
						c = 1;
					}
					else {
						c = 0;
					}
					if (c == 1) {
						switch (u32) {
							case LF:
								CRFlag = ! CRFlag;
								break;
							case FF:
							case CR:
								CRFlag = TRUE;
								break;
#if 0
							case HT:
								memset(&(Buff[i]),0x20,8);
								i = i + 8;
								CRFlag = FALSE;
								break;
#endif
							default:
								if (u32 >= 0x20) {
									int codepage = CP_ACP;	// 印刷用コードページ
									size_t out_len = UTF32ToUTF16(u32, &BuffW[len_w], _countof(BuffW) - len_w);
									len_w += out_len;
									out_len = UTF32ToMBCP(u32, codepage, &BuffA[len_a], _countof(BuffA) - len_a);
									len_a += out_len;
								}
								CRFlag = FALSE;
								break;
						}
					}
					if (len_w >= (_countof(BuffW)-7)) {
						CRFlag=TRUE;
					}
				} while ((c>0) && (! CRFlag));
				if (len_a >0) {
					PrnOutText(BuffA, len_a, NULL);
					//PrnOutTextW(BuffW, NULL, len_w, NULL);
				}
				if (CRFlag) {
					PrnX = Margin.left;
					if ((u32==FF) && (ts.PrnConvFF==0)) { // new page
						PrnY = Margin.bottom;
					}
					else { // new line
						PrnY = PrnY + PrnFH;
					}
				}
				CRFlag = (u32==CR);
			} while (c>0);
			CloseHandle(HPrnFile);
		}
		HPrnFile = INVALID_HANDLE_VALUE;
		VTPrintEnd();
	}
	handle->FinishCallback(handle);
}

static void PrintFileDirect(PrintFile *handle)
{
	HWND hParent;

	PrnAbortDlg = new CPrnAbortDlg();
	if (PrnAbortDlg==NULL) {
		DeletePrintFile(handle);
		return;
	}
	if (ActiveWin==IdVT) {
		hParent = HVTWin;
	}
	else {
		hParent = HTEKWin;
	}
	PrnAbortDlg->Create(hInst,hParent,&PrintAbortFlag,&ts);
	HPrnAbortDlg = PrnAbortDlg->GetSafeHwnd();

	handle->HPrnFile = CreateFileW(handle->PrnFName,
								   GENERIC_READ, FILE_SHARE_READ, NULL,
								   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	PrintAbortFlag = (handle->HPrnFile == INVALID_HANDLE_VALUE) || ! PrnOpen(ts.PrnDev);
	handle->PrnBuffCount = 0;
	SetTimer(HVTWin,IdPrnProcTimer,0,NULL);
}

void PrnFileDirectProc(PrintFile *handle)
{
	HANDLE HPrnFile = handle->HPrnFile;
	if (HPrnFile==INVALID_HANDLE_VALUE) {
		return;
	}
	if (PrintAbortFlag) {
		HPrnAbortDlg = NULL;
		PrnAbortDlg = NULL;
		PrnCancel();
	}
	if (!PrintAbortFlag && (HPrnFile != INVALID_HANDLE_VALUE)) {
		int c;
		do {
			if (handle->PrnBuffCount==0) {
				handle->PrnBuffCount = 0;
				DWORD NumberOfBytesRead;
				BOOL r = ReadFile(HPrnFile, handle->PrnBuff, sizeof(handle->PrnBuff[0]), &NumberOfBytesRead, NULL);
				if (r) {
					handle->PrnBuffCount = NumberOfBytesRead;
				}
			}

			if (handle->PrnBuffCount != 0) {
				// UTF-32
				unsigned int u32 = handle->PrnBuff[0];
				int codepage = CP_ACP;	// 印刷用コードページ
				char str[5];
				size_t out_len = UTF32ToMBCP(u32, codepage, str, _countof(str));
				c = PrnWrite(str, out_len);
				if (c==0) {
					SetTimer(HVTWin,IdPrnProcTimer,10,NULL);
					return;
				}
				handle->PrnBuffCount = 0;
			}
			else {
				c = 0;
			}
		} while (c>0);
	}
	PrnClose();

	if (PrnAbortDlg!=NULL) {
		PrnAbortDlg->DestroyWindow();
		PrnAbortDlg = NULL;
		HPrnAbortDlg = NULL;
	}

	handle->FinishCallback(handle);
}

/**
 * タイマー時間が経過、印字を開始する
 *		ClosePrnFile() の SetTimer(IdPrnStartTimer) がトリガ
 */
void PrnFileStart(PrintFile *handle)
{
	if (handle->HPrnFile != INVALID_HANDLE_VALUE) {
		return;
	}
	if (handle->PrnFName == NULL) {
		return;
	}
	if (ts.PrnDev[0]!=0) {
		PrintFileDirect(handle); // send file directry to printer port
	}
	else { // print file by using Windows API
		PrintFile_(handle);
	}
}

/**
 * プリント用ファイルの書き込みを終了
 * プリントを開始タイマーをセットする
 */
void ClosePrnFile(PrintFile *handle, void (*finish_callback)(PrintFile *handle))
{
	PrintFile *p = handle;
	p->PrnBuffCount = 0;
	p->FinishCallback = finish_callback;
	if (p->HPrnFile != INVALID_HANDLE_VALUE) {
		CloseHandle(p->HPrnFile);
		p->HPrnFile = INVALID_HANDLE_VALUE;
	}
	SetTimer(HVTWin,IdPrnStartTimer,ts.PassThruDelay*1000,NULL);
}

/**
 *  (b,Write) =
 *    (0,FALSE): clear buffer
 *    (0,TRUE):  write buffer to file
 *    (b,FALSE): put b in buff
 *    (b,TRUE):  put b in buff and
 *		 write buffer to file
 */
void WriteToPrnFileUTF32(PrintFile *handle, unsigned int u32, BOOL Write)
{
	PrintFile *p = handle;
	if ((u32 > 0) && p->PrnBuffCount < _countof(p->PrnBuff)) {
		p->PrnBuff[p->PrnBuffCount++] = u32;
	}

	if (Write) {
		DWORD NumberOfBytesWritten;
		WriteFile(p->HPrnFile, p->PrnBuff, sizeof(p->PrnBuff[0]) * p->PrnBuffCount, &NumberOfBytesWritten, NULL);
		p->PrnBuffCount = 0;
	}
	if ((u32 == 0) && !Write) {
		p->PrnBuffCount = 0;
	}
}

void WriteToPrnFile(PrintFile *handle, BYTE b, BOOL Write)
{
	WriteToPrnFileUTF32(handle, b, Write);
}
