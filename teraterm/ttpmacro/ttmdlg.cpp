/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, dialog boxes */

#include "stdafx.h"
#include "teraterm.h"
#include <direct.h>
#include "ttm_res.h"
#include "tttypes.h"
#include "ttlib.h"
#include <commdlg.h>
#include "errdlg.h"
#include "inpdlg.h"
#include "msgdlg.h"
#include "statdlg.h"
#include "ttmlib.h"

#define MaxStrLen 256

extern "C" {
char HomeDir[MAXPATHLEN];
char FileName[MAXPATHLEN];
char TopicName[11];
char ShortName[MAXPATHLEN];
char Param2[MAXPATHLEN];
char Param3[MAXPATHLEN];
char Param4[MAXPATHLEN];
char Param5[MAXPATHLEN];
char Param6[MAXPATHLEN];
char Param7[MAXPATHLEN];
char Param8[MAXPATHLEN];
char Param9[MAXPATHLEN];
BOOL SleepFlag;
}

static int DlgPosX = -1000;
static int DlgPosY = 0;

static PStatDlg StatDlg = NULL;

extern "C" {
BOOL NextParam(PCHAR Param, int *i, PCHAR Buff, int BuffSize)
{
	int j;
	char c, q;
	BOOL Quoted;

	if ( *i >= (int)strlen(Param)) {
		return FALSE;
	}
	j = 0;

	while (Param[*i]==' ') {
		(*i)++;
	}

	c = Param[*i];
	Quoted = ((c=='"') || (c=='\''));
	q = 0;
	if (Quoted) {
		q = c; 
	   (*i)++;
		c = Param[*i];
	}
	(*i)++;
	while ((c!=0) && (c!=q) && (Quoted || (c!=' ')) &&
	       (Quoted || (c!=';')) && (j<BuffSize-1)) {
		Buff[j] = c;
		j++;
		c = Param[*i];
		(*i)++;
	}
	if (! Quoted && (c==';')) {
		(*i)--;
	}

	Buff[j] = 0;
	return (strlen(Buff)>0);
}
}

extern "C" {
void ParseParam(PBOOL IOption, PBOOL VOption)
{
	int i, j, k;
	char *Param;
	char Temp[MAXPATHLEN];

	// Get home directory
	if (GetModuleFileName(AfxGetInstanceHandle(),FileName,sizeof(FileName)) == 0) {
		return;
	}
	ExtractDirName(FileName,HomeDir);
	_chdir(HomeDir);

	// Get command line parameters
	FileName[0] = 0;
	TopicName[0] = 0;
	Param2[0] = 0;
	Param3[0] = 0;
	Param4[0] = 0;
	Param5[0] = 0;
	Param6[0] = 0;
	Param7[0] = 0;
	Param8[0] = 0;
	Param9[0] = 0;
	SleepFlag = FALSE;
	*IOption = FALSE;
	*VOption = FALSE;
	// 256バイト以上のコマンドラインパラメータ指定があると、BOF(Buffer Over Flow)で
	// 落ちるバグを修正。(2007.5.25 yutaka)
	Param = GetCommandLine();
	i = 0;
	// the first term shuld be executable filename of TTMACRO
	NextParam(Param, &i, Temp, sizeof(Temp));
	j = 0;

	while (NextParam(Param, &i, Temp, sizeof(Temp))) {
		if (_strnicmp(Temp,"/D=",3)==0) { // DDE option
			strncpy_s(TopicName, sizeof(TopicName), &Temp[3], _TRUNCATE);  // BOF対策
		}
		else if (_strnicmp(Temp,"/I",2)==0) {
			*IOption = TRUE;
		}
		else if (_strnicmp(Temp,"/S",2)==0) {
			SleepFlag = TRUE;
		}
		else if (_strnicmp(Temp,"/V",2)==0) {
			*VOption = TRUE;
		}
		else {
			j++;
			if (j==1) {
				strncpy_s(FileName, sizeof(FileName),Temp, _TRUNCATE);
			}
			else if (j==2) {
				strncpy_s(Param2, sizeof(Param2),Temp, _TRUNCATE);
			}
			else if (j==3) {
				strncpy_s(Param3, sizeof(Param3),Temp, _TRUNCATE);
			}
			else if (j==4) {
				strncpy_s(Param4, sizeof(Param4),Temp, _TRUNCATE);
			}
			else if (j==5) {
				strncpy_s(Param5, sizeof(Param5),Temp, _TRUNCATE);
			}
			else if (j==6) {
				strncpy_s(Param6, sizeof(Param6),Temp, _TRUNCATE);
			}
			else if (j==7) {
				strncpy_s(Param7, sizeof(Param7),Temp, _TRUNCATE);
			}
			else if (j==8) {
				strncpy_s(Param8, sizeof(Param8),Temp, _TRUNCATE);
			}
			else if (j==9) {
				strncpy_s(Param9, sizeof(Param9),Temp, _TRUNCATE);
			}
		}
	}

	if (FileName[0]=='*') {
		FileName[0] = 0;
	}
	else if (FileName[0]!=0) {
		if (GetFileNamePos(FileName,&j,&k)) {
			FitFileName(&FileName[k],sizeof(FileName)-k,".TTL");
			strncpy_s(ShortName, sizeof(ShortName),&FileName[k], _TRUNCATE);
			if (j==0) {
				strncpy_s(FileName, sizeof(FileName),HomeDir, _TRUNCATE);
				AppendSlash(FileName,sizeof(FileName));
				strncat_s(FileName,sizeof(FileName),ShortName,_TRUNCATE);
			}
		}
		else {
			FileName[0] = 0;
		}
	}
}
}

extern "C" {
BOOL GetFileName(HWND HWin)
{
	char FNFilter[31];
	OPENFILENAME FNameRec;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];

	if (FileName[0]!=0) {
		return FALSE;
	}

	memset(FNFilter, 0, sizeof(FNFilter));
	memset(&FNameRec, 0, sizeof(OPENFILENAME));
	get_lang_msg("FILEDLG_OPEN_MACRO_FILTER", uimsg, sizeof(uimsg), "Macro files (*.ttl)\\0*.ttl\\0\\0", UILanguageFile);
	memcpy(FNFilter, uimsg, sizeof(FNFilter));

	// sizeof(OPENFILENAME) では Windows98/NT で終了してしまうため (2006.8.14 maya)
	FNameRec.lStructSize = get_OPENFILENAME_SIZE();
	FNameRec.hwndOwner	 = HWin;
	FNameRec.lpstrFilter	 = FNFilter;
	FNameRec.nFilterIndex  = 1;
	FNameRec.lpstrFile  = FileName;
	FNameRec.nMaxFile  = sizeof(FileName);
	// 以前読み込んだ .ttl ファイルのパスを記憶できるように、初期ディレクトリを固定にしない。
	// (2008.4.7 yutaka)
#if 0
	FNameRec.lpstrInitialDir = HomeDir;
#endif
	FNameRec.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	FNameRec.lpstrDefExt = "TTL";
	get_lang_msg("FILEDLG_OPEN_MACRO_TITLE", uimsg2, sizeof(uimsg2), "MACRO: Open macro", UILanguageFile);
	FNameRec.lpstrTitle = uimsg2;
	if (GetOpenFileName(&FNameRec)) {
		strncpy_s(ShortName, sizeof(ShortName), &(FileName[FNameRec.nFileOffset]), _TRUNCATE);
	}
	else {
		FileName[0] = 0;
	}

	if (FileName[0]==0) {
		ShortName[0] = 0;
		return FALSE;
	}
	else {
		return TRUE;
	}
}
}

extern "C" {
void SetDlgPos(int x, int y)
{
	DlgPosX = x;
	DlgPosY = y;
	if (StatDlg!=NULL) { // update status box position
		StatDlg->Update(NULL,NULL,TRUE,DlgPosX,DlgPosY);
	}
}
}

extern "C" {
void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd, BOOL SPECIAL)
{
	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,SPECIAL,DlgPosX,DlgPosY);
	InpDlg.DoModal();
}
}

extern "C" {
int OpenErrDlg(PCHAR Msg, PCHAR Line)
{
	CErrDlg ErrDlg(Msg,Line,DlgPosX,DlgPosY);
	return ErrDlg.DoModal();
}
}

extern "C" {
int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo, BOOL SPECIAL)
{
	CMsgDlg MsgDlg(Text,Caption,YesNo,SPECIAL,DlgPosX,DlgPosY);
	return MsgDlg.DoModal();
}
}

extern "C" {
void OpenStatDlg(PCHAR Text, PCHAR Caption, BOOL SPECIAL)
{
	if (StatDlg==NULL) {
		StatDlg = new CStatDlg();
		StatDlg->Create(Text,Caption,SPECIAL,DlgPosX,DlgPosY);
	}
	else {// if status box already exists,
		// update text and caption only.
		StatDlg->Update(Text,Caption,SPECIAL,32767,0);
	}
}
}

extern "C" {
void CloseStatDlg()
{
	if (StatDlg==NULL) {
		return;
	}
	StatDlg->DestroyWindow();
	StatDlg = NULL;
}
}
