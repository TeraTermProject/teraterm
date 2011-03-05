/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* Constants and types for TEK window */

#define ViewSizeX 4096
#define ViewSizeY 3120

#define CurWidth 2

#define IdAlphaMode 0
#define IdVectorMode 1
#define IdMarkerMode 2
#define IdPlotMode 3
#define IdUnknownMode 4

#define ModeFirst 0
#define ModeEscape 1
#define ModeCS 2
#define ModeSelectCode 3
#define Mode2OC 4
#define ModeGT 5

//"LF"
#define IdMove 0x4C46
// "LG"
#define IdDraw 0x4C47
// "LH"
#define IdDrawMarker 0x4C48
 // "LT"
#define IdGraphText 0x4C54
 // "LV"
#define IdSetDialogVisibility 0x4C56
 // "MC"
#define IdSetGraphTextSize 0x4D43
 // "MG"
#define IdSetWriteMode 0x4D47
 // "ML"
#define IdSetLineIndex 0x4D4C
 // "MM"
#define IdSetMarkerType 0x4D4D
 // "MN"
#define IdSetCharPath 0x4D4E
 // "MQ"
#define IdSetPrecision 0x4D51
 // "MR"
#define IdSetRotation 0x4D52
 // "MT"
#define IdSetTextIndex 0x4D54
 // "MV"
#define IdSetLineStyle 0x4D56

#define NParamMax 16
#define NParam2OCMax 16

typedef struct {
  HWND HWin;

  BOOL Drawing;
  int ParseMode;
  int SelectCodeFlag;

  LOGFONT TEKlf;
  HFONT TEKFont[4];
  HFONT OldMemFont;
  BOOL AdjustSize, ScaleFont;
  int ScreenWidth, ScreenHeight;
  int FontWidth, FontHeight;
  int FW[4], FH[4];
  int CaretX, CaretY;
  int CaretOffset;
  int TextSize;
  int DispMode;
  HDC MemDC;
  HBITMAP HBits, OldMemBmp;
  BOOL Active, Minimized, MoveFlag;
  COLORREF TextColor, PenColor;
  COLORREF MemForeColor, MemBackColor, MemTextColor, MemPenColor;
  HBRUSH BackGround, MemBackGround;
  HPEN Pen, MemPen, OldMemPen;
  int ps;
  int ChangeEmu;
  int CaretStatus;

  BOOL ButtonDown, Select, RubberBand;
  POINT SelectStart, SelectEnd;

  BOOL GIN, CrossHair;
  int IgnoreCount;
  int GINX, GINY;

  /* flags for Drawing */
  BOOL LoXReceive;
  int LoCount;
  BYTE LoA, LoB;

  /* variables for 2OC mode */
  int OpCount, PrmCount, PrmCountMax;
  WORD Op2OC;
  WORD Prm2OC[NParam2OCMax+1];

  /* plot mode */
  BOOL JustAfterRS, PenDown;
  int PlotX, PlotY;

  // variables for control sequences
  int CSCount;
  BYTE CSBuff[256];
  int NParam;
  int Param[NParamMax+1];

  // variables for graphtext
  int GTWidth, GTHeight, GTSpacing;
  int GTCount, GTLen, GTAngle;
  char GTBuff[80];

  // variables for marker
  int MarkerType, MarkerW, MarkerH;
  HFONT MarkerFont;
  BOOL MarkerFlag;

  BYTE HiY, Extra, LoY, HiX, LoX;   
} TTEKVar;
typedef TTEKVar far *PTEKVar;
