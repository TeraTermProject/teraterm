/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, Printing routines */
#include "stdafx.h"
#include "teraterm.h"
#include "tttypes.h"
#include <commdlg.h>
#include <stdio.h>

#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttlib.h"

#include "tt_res.h"
#include "prnabort.h"

#include "teraprn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static PRINTDLG PrnDlg;

static HDC PrintDC;
static LOGFONT Prnlf;
static HFONT PrnFont[AttrFontMask+1];
static int PrnFW, PrnFH;
static RECT Margin;
static COLORREF White, Black;
static int PrnX, PrnY;
static int PrnDx[256];
static TCharAttr PrnAttr;

static BOOL Printing = FALSE;
static BOOL PrintAbortFlag = FALSE;

/* pass-thru printing */
static char PrnFName[MAXPATHLEN];
static int HPrnFile = 0;
static char PrnBuff[TermWidthMax];
static int PrnBuffCount = 0;

static CPrnAbortDlg *PrnAbortDlg;
static HWND HPrnAbortDlg;

/* Print Abortion Call Back Function */
BOOL CALLBACK PrnAbortProc(HDC PDC, int Code)
{
  MSG m;

  while ((! PrintAbortFlag) && PeekMessage(&m, 0,0,0, PM_REMOVE))
    if ((HPrnAbortDlg==NULL) || (! IsDialogMessage(HPrnAbortDlg, &m)))
    {
      TranslateMessage(&m);
      DispatchMessage(&m);
    }

  if (PrintAbortFlag)
  {
    HPrnAbortDlg = NULL;
    PrnAbortDlg = NULL;
    return FALSE;
  }
  else
    return TRUE;
}

extern "C" {
HDC PrnBox(HWND HWin, PBOOL Sel)
{
  /* initialize PrnDlg record */
  memset(&PrnDlg, 0, sizeof(PRINTDLG));
  PrnDlg.lStructSize = sizeof(PRINTDLG);
  PrnDlg.hwndOwner = HWin;
  PrnDlg.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_SHOWHELP;
  if (! *Sel)
    PrnDlg.Flags = PrnDlg.Flags | PD_NOSELECTION;
  PrnDlg.nCopies = 1;

  /* 'Print' dialog box */
  if (! PrintDlg(&PrnDlg)) return NULL; /* if 'Cancel' button clicked, exit */
  if (PrnDlg.hDC == NULL) return NULL;
  PrintDC = PrnDlg.hDC;
  *Sel = (PrnDlg.Flags & PD_SELECTION) != 0;
  return PrintDC;
}
}

extern "C" {
BOOL PrnStart(LPSTR DocumentName)
{
  DOCINFO Doc;
  char DocName[50];
  CWnd* pParent;
  char uimsg[MAX_UIMSG];

  Printing = FALSE;
  PrintAbortFlag = FALSE;

  PrnAbortDlg = new CPrnAbortDlg();
  if (PrnAbortDlg==NULL) return FALSE;
  if (ActiveWin==IdVT)
    pParent = (CWnd*)pVTWin;
  else
    pParent = (CWnd*)pTEKWin;
  PrnAbortDlg->Create(pParent,&PrintAbortFlag,&ts);
  HPrnAbortDlg = PrnAbortDlg->GetSafeHwnd();

  GetDlgItemText(HPrnAbortDlg, IDC_PRNABORT_PRINTING, uimsg, sizeof(uimsg));
  get_lang_msg("DLG_PRNABORT_PRINTING", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  SetDlgItemText(HPrnAbortDlg, IDC_PRNABORT_PRINTING, ts.UIMsg);
  GetDlgItemText(HPrnAbortDlg, IDCANCEL, uimsg, sizeof(uimsg));
  get_lang_msg("BTN_CANCEL", ts.UIMsg, sizeof(ts.UIMsg), uimsg, ts.UILanguageFile);
  SetDlgItemText(HPrnAbortDlg, IDCANCEL, ts.UIMsg);

  SetAbortProc(PrintDC,PrnAbortProc);

  Doc.cbSize = sizeof(DOCINFO);
  strncpy_s(DocName,sizeof(DocName),DocumentName,_TRUNCATE);
  Doc.lpszDocName = DocName;
  Doc.lpszOutput = NULL;
  Doc.lpszDatatype = NULL;
  Doc.fwType = 0;
  if (StartDoc(PrintDC, &Doc) > 0)
    Printing = TRUE;
  else
    if (PrnAbortDlg != NULL)
    {
      PrnAbortDlg->DestroyWindow();
      PrnAbortDlg = NULL;
      HPrnAbortDlg = NULL;
    }
  return Printing;
}
}

extern "C" {
void PrnStop()
{
  if (Printing)
  {
    EndDoc(PrintDC);
    DeleteDC(PrintDC);
    Printing = FALSE;
  }
  if (PrnAbortDlg != NULL)
  {
    PrnAbortDlg->DestroyWindow();
    PrnAbortDlg = NULL;
    HPrnAbortDlg = NULL;
  }
}
}

extern "C" {
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
  int i;
  TCharAttr TempAttr = {
    AttrDefault,
    AttrDefault,
    AttrDefaultFG,
    AttrDefaultBG
  };

  Sel = (PrnFlag & IdPrnSelectedText)!=0;
  if (PrnBox(HVTWin,&Sel)==NULL) return (IdPrnCancel);

  if (PrintDC==0) return (IdPrnCancel);

  /* start printing */
  if (! PrnStart(ts.Title)) return (IdPrnCancel);

  /* initialization */
  StartPage(PrintDC);

  /* pixels per inch */
  if ((ts.VTPPI.x>0) && (ts.VTPPI.y>0))
    PPI = ts.VTPPI;
  else {
    PPI.x = GetDeviceCaps(PrintDC,LOGPIXELSX);
    PPI.y = GetDeviceCaps(PrintDC,LOGPIXELSY);
  }

  Margin.left = /* left margin */
    (int)((float)ts.PrnMargin[0] / 100.0 * (float)PPI.x);
  Margin.right = /* right margin */
    GetDeviceCaps(PrintDC,HORZRES) -
    (int)((float)ts.PrnMargin[1] / 100.0 * (float)PPI.x);
  Margin.top = /* top margin */
    (int)((float)ts.PrnMargin[2] / 100.0 * (float)PPI.y);
  Margin.bottom = /* bottom margin */
    GetDeviceCaps(PrintDC,VERTRES) -
    (int)((float)ts.PrnMargin[3] / 100.0 * (float)PPI.y);

  /* create test font */
  memset(&Prnlf, 0, sizeof(LOGFONT));

  if (ts.PrnFont[0]==0)
  {
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
  Prnlf.lfOutPrecision	= OUT_CHARACTER_PRECIS;
  Prnlf.lfClipPrecision = CLIP_CHARACTER_PRECIS;
  Prnlf.lfQuality	= DEFAULT_QUALITY;
  Prnlf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

  PrnFont[0] = CreateFontIndirect(&Prnlf);

  DC = GetDC(HVTWin);
  SelectObject(DC, PrnFont[0]);
  GetTextMetrics(DC, &Metrics);
  PPI2.x = GetDeviceCaps(DC,LOGPIXELSX);
  PPI2.y = GetDeviceCaps(DC,LOGPIXELSY);
  ReleaseDC(HVTWin,DC);
  DeleteObject(PrnFont[0]); /* Delete test font */

  /* Adjust font size */
  Prnlf.lfHeight =
    (int)((float)Metrics.tmHeight * (float)PPI.y / (float)PPI2.y);
  Prnlf.lfWidth =
    (int)((float)Metrics.tmAveCharWidth * (float)PPI.x / (float)PPI2.x);

  /* Create New Fonts */

  /* Normal Font */
  Prnlf.lfWeight = FW_NORMAL;
  Prnlf.lfUnderline = 0;
  PrnFont[0] = CreateFontIndirect(&Prnlf);
  SelectObject(PrintDC,PrnFont[0]);
  GetTextMetrics(PrintDC, &Metrics);
  PrnFW = Metrics.tmAveCharWidth;
  PrnFH = Metrics.tmHeight;
  /* Under line */
  Prnlf.lfUnderline = 1;
  PrnFont[AttrUnder] = CreateFontIndirect(&Prnlf);

  if (ts.EnableBold > 0)
  {
    /* Bold */
    Prnlf.lfUnderline = 0;
    Prnlf.lfWeight = FW_BOLD;
    PrnFont[AttrBold] = CreateFontIndirect(&Prnlf);
    /* Bold + Underline */
    Prnlf.lfUnderline = 1;
    PrnFont[AttrBold | AttrUnder] = CreateFontIndirect(&Prnlf);
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
  PrnFont[AttrSpecial] = CreateFontIndirect(&Prnlf);
  PrnFont[AttrSpecial | AttrBold] = PrnFont[AttrSpecial];
  PrnFont[AttrSpecial | AttrUnder] = PrnFont[AttrSpecial];
  PrnFont[AttrSpecial | AttrBold | AttrUnder] = PrnFont[AttrSpecial];

  Black = RGB(0,0,0);
  White = RGB(255,255,255);
  for (i = 0 ; i <= 255 ; i++)
    PrnDx[i] = PrnFW;
  PrnSetAttr(TempAttr);

  PrnY = Margin.top;
  PrnX = Margin.left;

  if (PrnFlag == IdPrnScrollRegion)
    return (IdPrnScrollRegion);
  if (PrnFlag == IdPrnFile)
    return (IdPrnFile);
  if (Sel) return (IdPrnSelectedText);
      else return (IdPrnScreen);

}
}
extern "C" {
void PrnSetAttr(TCharAttr Attr)
//  Set text attribute of printing
//
{
  PrnAttr = Attr;
  SelectObject(PrintDC, PrnFont[Attr.Attr & AttrFontMask]);

  if ((Attr.Attr & AttrReverse) != 0)
  {
    SetTextColor(PrintDC,White);
    SetBkColor(  PrintDC,Black);
  }
  else {
    SetTextColor(PrintDC,Black);
    SetBkColor(  PrintDC,White);
  }
}
}

extern "C" {
void PrnOutText(PCHAR Buff, int Count)
//  Print out text
//    Buff: points text buffer
//    Count: number of characters to be printed
{
  int i;
  RECT RText;
  PCHAR Ptr, Ptr1, Ptr2;
  char Buff2[256];

  if (Count<=0) return;
  if (Count>(sizeof(Buff2)-1)) Count=sizeof(Buff2)-1;
  memcpy(Buff2,Buff,Count);
  Buff2[Count] = 0;
  Ptr = Buff2;

  if (ts.Language==IdRussian)
  {
    if (ts.PrnFont[0]==0)
      RussConvStr(ts.RussClient,ts.RussFont,Buff2,Count);
    else
      RussConvStr(ts.RussClient,ts.RussPrint,Buff2,Count);
  }

  do {
    if (PrnX+PrnFW > Margin.right)
    {
      /* new line */
      PrnX = Margin.left;
      PrnY = PrnY + PrnFH;
    }
    if (PrnY+PrnFH > Margin.bottom)
    {
      /* next page */
      EndPage(PrintDC);
      StartPage(PrintDC);
      PrnSetAttr(PrnAttr);
      PrnY = Margin.top;
    }

    i = (Margin.right-PrnX) / PrnFW;
    if (i==0) i=1;
    if (i>Count) i=Count;

    if (i<Count)
    {
      Ptr2 = Ptr;
      do {
	Ptr1 = Ptr2;
	Ptr2 = CharNext(Ptr1);
      } while ((Ptr2!=NULL) && ((Ptr2-Ptr)<=i));
      i = Ptr1-Ptr;
      if (i<=0) i=1;
    }

    RText.left = PrnX;
    RText.right = PrnX + i*PrnFW;
    RText.top = PrnY;
    RText.bottom = PrnY+PrnFH;
    ExtTextOut(PrintDC,PrnX,PrnY,6,&RText,Ptr,
	       i,&PrnDx[0]);
    PrnX = RText.right;
    Count=Count-i;
    Ptr = Ptr + i;
  } while (Count>0);

}
}

extern "C" {
void PrnNewLine()
//  Moves to the next line in printing
{
  PrnX = Margin.left;
  PrnY = PrnY + PrnFH;
}
}

extern "C" {
void VTPrintEnd()
{
  int i, j;

  EndPage(PrintDC);

  for (i = 0 ; i <= AttrFontMask ; i++)
  {
    for (j = i+1 ; j <= AttrFontMask ; j++)
      if (PrnFont[j]==PrnFont[i])
	PrnFont[j] = NULL;
    if (PrnFont[i] != NULL) DeleteObject(PrnFont[i]);
  }

  PrnStop();
  return;
}
}

/* printer emulation routines */
extern "C" {
void OpenPrnFile()
{
  char Temp[MAXPATHLEN];

  KillTimer(HVTWin,IdPrnStartTimer);
  if (HPrnFile > 0) return;
  if (PrnFName[0] == 0)
  {
    GetTempPath(sizeof(Temp),Temp);
    if (GetTempFileName(Temp,"tmp",0,PrnFName)==0) return;
    HPrnFile = _lcreat(PrnFName,0);
  }
  else {
    HPrnFile = _lopen(PrnFName,OF_WRITE);
    if (HPrnFile <= 0)
      HPrnFile = _lcreat(PrnFName,0);      
  }
  if (HPrnFile > 0)
    _llseek(HPrnFile,0,2);
}
}

void PrintFile()
{
  char Buff[256];
  BOOL CRFlag = FALSE;
  int c, i;
  BYTE b;

  if (VTPrintInit(IdPrnFile)==IdPrnFile)
  {
    HPrnFile = _lopen(PrnFName,OF_READ);
    if (HPrnFile>0)
    {
      do {
	i = 0;
	do {
	  c = _lread(HPrnFile,&b,1);
	  if (c==1)
	  {
	    switch (b) {
	      case HT:
		memset(&(Buff[i]),0x20,8);
		i = i + 8;
		CRFlag = FALSE;
		break;
	      case LF:
		CRFlag = ! CRFlag;
		break;
	      case FF:
	      case CR:
		CRFlag = TRUE;
		break;
	      default:
		if (b >= 0x20)
		{
		  Buff[i] = b;
		  i++;
		}
		CRFlag = FALSE;
		break;
	    }
	  }
	  if (i>=(sizeof(Buff)-7)) CRFlag=TRUE;
	} while ((c>0) && (! CRFlag));
	if (i>0) PrnOutText(Buff, i);
	if (CRFlag)
	{
	  PrnX = Margin.left;
	  if ((b==FF) && (ts.PrnConvFF==0)) // new page
	    PrnY = Margin.bottom;
	  else // new line
	    PrnY = PrnY + PrnFH;
	}
	CRFlag = (b==CR);
      }  while (c>0);
      _lclose(HPrnFile);
    }
    HPrnFile = 0;
    VTPrintEnd();
  }
  remove(PrnFName);
  PrnFName[0] = 0;
}

void PrintFileDirect()
{
  CWnd* pParent;

  PrnAbortDlg = new CPrnAbortDlg();
  if (PrnAbortDlg==NULL)
  {
    remove(PrnFName);
    PrnFName[0] = 0;
    return;
  }
  if (ActiveWin==IdVT)
    pParent = (CWnd*)pVTWin;
  else
    pParent = (CWnd*)pTEKWin;
  PrnAbortDlg->Create(pParent,&PrintAbortFlag,&ts);
  HPrnAbortDlg = PrnAbortDlg->GetSafeHwnd();

  HPrnFile = _lopen(PrnFName,OF_READ);
  PrintAbortFlag = (HPrnFile<=HFILE_ERROR) || ! PrnOpen(ts.PrnDev);
  PrnBuffCount = 0;
  SetTimer(HVTWin,IdPrnProcTimer,0,NULL);
}

void PrnFileDirectProc()
{
  int c;

  if (HPrnFile==0) return;
  if (PrintAbortFlag)
  {
    HPrnAbortDlg = NULL;
    PrnAbortDlg = NULL;
    PrnCancel();
  }
  if (!PrintAbortFlag && (HPrnFile>0))
  {
    do {
      if (PrnBuffCount==0)
      {
        PrnBuffCount = _lread(HPrnFile,PrnBuff,1);
	if (ts.Language==IdRussian)
	  RussConvStr(ts.RussClient,ts.RussPrint,PrnBuff,PrnBuffCount);
      }

      if (PrnBuffCount==1)
      {
	c = PrnWrite(PrnBuff,1);
	if (c==0)
	{
	  SetTimer(HVTWin,IdPrnProcTimer,10,NULL);
	  return;
	}
	PrnBuffCount = 0;
      }
      else
	c = 0;
    } while (c>0);
  }
  if (HPrnFile > 0)
    _lclose(HPrnFile);
  HPrnFile = 0;
  PrnClose();
  remove(PrnFName);
  PrnFName[0] = 0;
  if (PrnAbortDlg!=NULL)
  {
    PrnAbortDlg->DestroyWindow();
    PrnAbortDlg = NULL;
    HPrnAbortDlg = NULL;
  }
}

void PrnFileStart()
{
  if (HPrnFile>0) return;
  if (PrnFName[0]==0) return;
  if (ts.PrnDev[0]!=0)
    PrintFileDirect(); // send file directry to printer port
  else  // print file by using Windows API
    PrintFile();
}

extern "C" {
void ClosePrnFile()
{
  PrnBuffCount = 0;
  if (HPrnFile > 0)
    _lclose(HPrnFile);
  HPrnFile = 0;
  SetTimer(HVTWin,IdPrnStartTimer,ts.PassThruDelay*1000,NULL);
}
}

extern "C" {
void WriteToPrnFile(BYTE b, BOOL Write)
//  (b,Write) =
//    (0,FALSE): clear buffer
//    (0,TRUE):  write buffer to file
//    (b,FALSE): put b in buff
//    (b,TRUE):  put b in buff and
//		 write buffer to file
{
  if ((b>0) && (PrnBuffCount<sizeof(PrnBuff)))
    PrnBuff[PrnBuffCount++] = b;

  if (Write)
  {
    _lwrite(HPrnFile,PrnBuff,PrnBuffCount);
    PrnBuffCount = 0;
  }
  if ((b==0) && ! Write) PrnBuffCount = 0;
}
}
