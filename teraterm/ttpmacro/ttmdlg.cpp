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
#include "ListDlg.h"
#include "ttmlib.h"

#define MaxStrLen 512

extern "C" {
char HomeDir[MAXPATHLEN];
char FileName[MAX_PATH];
char TopicName[11];
char ShortName[MAX_PATH];
char Param2[MaxStrLen];
char Param3[MaxStrLen];
char Param4[MaxStrLen];
char Param5[MaxStrLen];
char Param6[MaxStrLen];
char Param7[MaxStrLen];
char Param8[MaxStrLen];
char Param9[MaxStrLen];
BOOL SleepFlag;
int ParamCnt;   /* 引数の個数 */
}

static int DlgPosX = -10000;
static int DlgPosY = 0;

static PStatDlg StatDlg = NULL;

extern "C" {
void ParseParam(PBOOL IOption, PBOOL VOption)
{
	int dirlen, fnpos;
	char *Param;
	char Temp[MaxStrLen];
	PCHAR start, cur, next;

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
	Param = GetCommandLine();

	// the first term shuld be executable filename of TTMACRO
	start = GetParam(Temp, sizeof(Temp), Param);
	ParamCnt = 0;

	for (cur = start; next = GetParam(Temp, sizeof(Temp), cur); cur = next) {
		DequoteParam(Temp, sizeof(Temp), Temp);
		if (ParamCnt == 0) {
			if (_strnicmp(Temp,"/D=",3)==0) { // DDE option
				strncpy_s(TopicName, sizeof(TopicName), &Temp[3], _TRUNCATE);
				continue;
			}
			else if (_stricmp(Temp, "/I")==0) {
				*IOption = TRUE;
				continue;
			}
			else if (_stricmp(Temp, "/S")==0) {
				SleepFlag = TRUE;
				continue;
			}
			else if (_stricmp(Temp, "/V")==0) {
				*VOption = TRUE;
				continue;
			}
		}

		switch (++ParamCnt) {
			case 1: strncpy_s(FileName, sizeof(FileName), Temp, _TRUNCATE); break;
			case 2: strncpy_s(Param2,   sizeof(Param2),   Temp, _TRUNCATE); break;
			case 3: strncpy_s(Param3,   sizeof(Param3),   Temp, _TRUNCATE); break;
			case 4: strncpy_s(Param4,   sizeof(Param4),   Temp, _TRUNCATE); break;
			case 5: strncpy_s(Param5,   sizeof(Param5),   Temp, _TRUNCATE); break;
			case 6: strncpy_s(Param6,   sizeof(Param6),   Temp, _TRUNCATE); break;
			case 7: strncpy_s(Param7,   sizeof(Param7),   Temp, _TRUNCATE); break;
			case 8: strncpy_s(Param8,   sizeof(Param8),   Temp, _TRUNCATE); break;
			case 9: strncpy_s(Param9,   sizeof(Param9),   Temp, _TRUNCATE); break;
			default: ;/* nothing to do */
		}
	}

	if (ParamCnt > 9) {
		ParamCnt = 9;
	}

	if (FileName[0]=='*') {
		FileName[0] = 0;
	}
	else if (FileName[0]!=0) {
		if (GetFileNamePos(FileName, &dirlen, &fnpos)) {
			FitFileName(&FileName[fnpos], sizeof(FileName) - fnpos, ".TTL");
			strncpy_s(ShortName, sizeof(ShortName), &FileName[fnpos], _TRUNCATE);
			if (dirlen==0) {
				strncpy_s(FileName, sizeof(FileName), HomeDir, _TRUNCATE);
				AppendSlash(FileName, sizeof(FileName));
				strncat_s(FileName, sizeof(FileName), ShortName, _TRUNCATE);
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
		/* ttpmacro.exeが単体で起動され、ダイアログでマクロファイルが読み込まれた場合は、
		 * 引数の個数は"1"となり、"param1"が更新される。
		 * (2012.4.14 yutaka)
		 */
		ParamCnt = 1;
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
		StatDlg->Update(NULL,NULL,DlgPosX,DlgPosY);
	}
}
}

extern "C" {
void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd)
{
	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,DlgPosX,DlgPosY);
	InpDlg.DoModal();
}
}

extern "C" {
int OpenErrDlg(PCHAR Msg, PCHAR Line, int lineno, int start, int end, PCHAR FileName)
{
	CErrDlg ErrDlg(Msg,Line,DlgPosX,DlgPosY, lineno, start, end, FileName);
	return ErrDlg.DoModal();
}
}

extern "C" {
int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo)
{
	CMsgDlg MsgDlg(Text,Caption,YesNo,DlgPosX,DlgPosY);
	return MsgDlg.DoModal();
}
}

extern "C" {
void OpenStatDlg(PCHAR Text, PCHAR Caption)
{
	if (StatDlg==NULL) {
		StatDlg = new CStatDlg();
		StatDlg->Create(Text,Caption,DlgPosX,DlgPosY);
	}
	else {// if status box already exists,
		// update text and caption only.
		StatDlg->Update(Text,Caption,32767,0);
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

extern "C" {
void BringupStatDlg()
{
	if (StatDlg==NULL) {
		return;
	}
	StatDlg->Bringup();
}
}

extern "C" {
int OpenListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int Selected)
{
	int ret = -1;

	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY);
	if (ListDlg.DoModal() == IDOK) {
		ret = ListDlg.m_SelectItem;
	}
	return (ret);
}
}

