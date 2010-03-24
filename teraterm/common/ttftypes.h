/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* Constants, types for file transfer */

/* GetSetupFname function id */
#define GSF_SAVE	0 // Save setup
#define GSF_RESTORE	1 // Restore setup
#define GSF_LOADKEY	2 // Load key map

/* GetTransFname function id */
#define GTF_SEND 0 /* Send file */
#define GTF_LOG  1 /* Log */
#define GTF_BP   2 /* B-Plus Send */

/* GetMultiFname function id */
#define GMF_KERMIT 0 /* Kermit Send */
#define GMF_Z  1     /* ZMODEM Send */
#define GMF_QV 2     /* Quick-VAN Send */
#define GMF_Y  3     /* YMODEM Send */

#define FnStrMemSize 4096

#define PROTO_KMT 1
#define PROTO_XM  2
#define PROTO_ZM  3
#define PROTO_BP  4
#define PROTO_QV  5
#define PROTO_YM  6

#define OpLog      1
#define OpSendFile 2
#define OpKmtRcv   3
#define OpKmtGet   4
#define OpKmtSend  5
#define OpKmtFin   6
#define OpXRcv     7
#define OpXSend    8
#define OpZRcv     9
#define OpZSend    10
#define OpBPRcv    11
#define OpBPSend   12
#define OpQVRcv    13
#define OpQVSend   14
#define OpYRcv     15
#define OpYSend    16

#define TitLog      "Log"
#define TitSendFile "Send file"
#define TitKmtRcv   "Kermit Receive"
#define TitKmtGet   "Kermit Get"
#define TitKmtSend  "Kermit Send"
#define TitKmtFin   "Kermit Finish"
#define TitXRcv     "XMODEM Receive"
#define TitXSend    "XMODEM Send"
#define TitYRcv     "YMODEM Receive"
#define TitYSend    "YMODEM Send"
#define TitZRcv     "ZMODEM Receive"
#define TitZSend    "ZMODEM Send"
#define TitBPRcv    "B-Plus Receive"
#define TitBPSend   "B-Plus Send"
#define TitQVRcv    "Quick-VAN Receive"
#define TitQVSend   "Quick-VAN Send"

typedef struct {
  HWND HMainWin;
  HWND HWin;
  WORD OpId;
  char DlgCaption[40];

  char FullName[MAXPATHLEN];
  int DirLen;

  int NumFname, FNCount;
  HANDLE FnStrMemHandle;
  PCHAR FnStrMem;
  int FnPtr;

  BOOL FileOpen;
  int FileHandle;
  LONG FileSize, ByteCount;
  BOOL OverWrite;

  BOOL LogFlag;
  int LogFile;
  WORD LogState;
  WORD LogCount;

  BOOL Success;
  BOOL NoMsg;

  char LogDefaultPath[MAX_PATH];
  BOOL HideDialog;

  BYTE LogLineBuf[16];
  int FlushLogLineBuf;

  int ProgStat;
} TFileVar;
typedef TFileVar far *PFileVar;

typedef struct {
  BYTE MAXL,TIME,NPAD,PADC,EOL,QCTL,QBIN,CHKT,REPT;
} KermitParam;

typedef struct {
  BYTE PktIn[96], PktOut[96];
  int PktInPtr;
  int PktInLen, PktInCount;
  int PktNum, PktNumOffset;
  int PktReadMode;
  int KmtMode, KmtState;
  BOOL Quote8, RepeatFlag;
  char ByteStr[6];
  BOOL NextByteFlag;
  int RepeatCount;
  BYTE NextSeq;
  BYTE NextByte;
  KermitParam KmtMy, KmtYour;
} TKmtVar;
typedef TKmtVar far *PKmtVar;

  /* Kermit states */
#define WaitMark  0
#define WaitLen   1
#define WaitCheck 2

#define Unknown 0
#define SendInit 1
#define SendFile 2
#define SendData 3
#define SendEOF 4
#define SendEOT 5

#define ReceiveInit 6
#define ReceiveFile 7
#define ReceiveData 8

#define ServerInit 9
#define GetInit 10
#define Finish 11

/* XMODEM */
typedef struct {
  BYTE PktIn[1030], PktOut[1030];
  int PktBufCount, PktBufPtr;
  BYTE PktNum, PktNumSent;
  int PktNumOffset;
  int PktReadMode;
  WORD XMode, XOpt, TextFlag;
  WORD NAKMode;
  int NAKCount;
  WORD DataLen, CheckLen;
  BOOL CRRecv;
  int TOutShort;
  int TOutLong;
} TXVar;
typedef TXVar far *PXVar;

  /* XMODEM states */
#define XpktSOH 1
#define XpktBLK 2
#define XpktBLK2 3
#define XpktDATA 4

#define XnakNAK 1
#define XnakC 2


/* YMODEM */
typedef struct {
  BYTE PktIn[1030], PktOut[1030];
  int PktBufCount, PktBufPtr;
  BYTE PktNum, PktNumSent;
  int PktNumOffset;
  int PktReadMode;
  WORD YMode, YOpt, TextFlag;
  WORD NAKMode;
  int NAKCount;
  WORD DataLen, CheckLen;
  BOOL CRRecv;
  int TOutShort;
  int TOutLong;
  int SendFileInfo;
  int SendEot;
  int LastSendEot;
} TYVar;
typedef TYVar far *PYVar;

  /* YMODEM states */
#define YpktSOH 0x01
#define YpktSTX 0x02
#define YpktEOT 0x04
#define YpktACK 0x06
#define YpktNAK 0x15
#define YpktCAN 0x18

#define YnakC 1
#define YnakG 2


/* ZMODEM */
typedef struct {
  BYTE RxHdr[4], TxHdr[4];
  BYTE RxType, TERM;
  BYTE PktIn[1032], PktOut[1032];
  int PktInPtr, PktOutPtr;
  int PktInCount, PktOutCount;
  int PktInLen;
  BOOL BinFlag;
  BOOL Sending;
  int ZMode, ZState, ZPktState;
  int MaxDataLen, TimeOut, CanCount;
  BOOL CtlEsc, CRC32, HexLo, Quoted, CRRecv;
  WORD CRC;
  LONG CRC3, Pos, LastPos, WinSize;
  BYTE LastSent;
} TZVar;
typedef TZVar far *PZVar;

#define Z_RecvInit 1
#define Z_RecvInit2 2
#define Z_RecvData 3
#define Z_RecvFIN  4
#define Z_SendInit 5
#define Z_SendInitHdr 6
#define Z_SendInitDat 7
#define Z_SendFileHdr 8
#define Z_SendFileDat 9
#define Z_SendDataHdr 10
#define Z_SendDataDat 11
#define Z_SendDataDat2 12
#define Z_SendEOF  13
#define Z_SendFIN  14
#define Z_Cancel   15
#define Z_End      16

#define Z_PktGetPAD 1
#define Z_PktGetDLE 2
#define Z_PktHdrFrm 3
#define Z_PktGetBin 4
#define Z_PktGetHex 5
#define Z_PktGetHexEOL 6
#define Z_PktGetData 7
#define Z_PktGetCRC 8

/* B Plus */
typedef struct {
  BYTE WS;
  BYTE WR;
  BYTE B_S;
  BYTE CM;
  BYTE DQ;
  BYTE TL;
  BYTE Q[8];
  BYTE DR;
  BYTE UR;
  BYTE FI;
} TBPParam;


typedef struct {
  BYTE PktIn[2066], PktOut[2066];
  int PktInCount, CheckCount;
  int PktOutLen, PktOutCount, PktOutPtr;
  LONG Check, CheckCalc;
  BYTE PktNum, PktNumSent;
  int PktNumOffset;
  int PktSize;
  WORD BPMode, BPState, BPPktState;
  BOOL GetPacket, EnqSent;
  BYTE MaxBS, CM;
  BOOL Quoted;
  int TimeOut;
  BOOL CtlEsc;
  BYTE Q[8];
} TBPVar;
typedef TBPVar far *PBPVar;

  /* B Plus states */
#define BP_Init      1
#define BP_RecvFile  2
#define BP_RecvData  3
#define BP_SendFile  4
#define BP_SendData  5
#define BP_SendClose 6
#define BP_Failure   7
#define BP_Close     8
#define BP_AutoFile  9

  /* B Plus packet states */
#define BP_PktGetDLE   1
#define BP_PktDLESeen  2
#define BP_PktGetData  3
#define BP_PktGetCheck 4
#define BP_PktSending  5

/* Quick-VAN */
typedef struct {
  BYTE PktIn[142], PktOut[142];
  int PktInCount, PktInPtr;
  int PktOutCount, PktOutPtr, PktOutLen;
  WORD Ver, WinSize;
  WORD QVMode, QVState, PktState;
  WORD AValue;
  WORD SeqNum;
  WORD FileNum;
  int RetryCount;
  BOOL CanFlag;
  WORD Year,Month,Day,Hour,Min,Sec;
  WORD SeqSent, WinEnd, FileEnd;
  BOOL EnqFlag;
  BYTE CheckSum;
} TQVVar;
typedef TQVVar far *PQVVar;

  /* Quick-VAN states */
#define QV_RecvInit1 1
#define QV_RecvInit2 2
#define QV_RecvData 3
#define QV_RecvDataRetry 4
#define QV_RecvNext 5
#define QV_RecvEOT 6
#define QV_Cancel 7
#define QV_Close 8

#define QV_SendInit1 11
#define QV_SendInit2 12
#define QV_SendInit3 13
#define QV_SendData 14
#define QV_SendDataRetry 15
#define QV_SendNext 16
#define QV_SendEnd 17

#define QVpktSOH 1
#define QVpktBLK 2
#define QVpktBLK2 3
#define QVpktDATA 4

#define QVpktSTX 5
#define QVpktCR 6
