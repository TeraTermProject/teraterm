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

// TTMACRO.EXE, Macro file buffer

#include "teraterm.h"
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include "ttmparse.h"
#include "ttlib.h"
#include "fileread.h"
#include "codeconv.h"

#include "ttmbuff.h"

int EndWhileFlag;
int BreakFlag;
BOOL ContinueFlag;

#define MAXBUFFLEN 2147483647	// 0x7FFF_FFFF

#define MAXNESTLEVEL 10

#define MAXFILENAME 128   // .ttl�t�@�C�����̍ő�T�C�Y

static int INest;
static char BuffHandleFileName[MAXNESTLEVEL][MAXFILENAME];  // �e�K�w��.ttl�t�@�C����(UTF-8)
static PCHAR Buff[MAXNESTLEVEL];
static BINT BuffLen[MAXNESTLEVEL];
static BINT BuffPtr[MAXNESTLEVEL];

// �s���z��
#define MAX_LINENO 100000
static BINT *BuffLineNo[MAXNESTLEVEL];
static BINT BuffLineNoMaxIndex[MAXNESTLEVEL];

#define MAXSP 10

// Control type
  // Call
#define CtlCall  1
  // For ... Next
#define CtlFor   2
  // While ... EndWhile
#define CtlWhile 3

// Control stack
#define INVALIDPTR (BINT)-1
static BINT PtrStack[MAXSP];
static int LevelStack[MAXSP];
static int TypeStack[MAXSP];
static int SP; // Stack pointer
static BINT LineStart;
static BOOL NextFlag;

static int LineNo;  // line number (2005.7.18 yutaka)

void InitLineNo(void)
{
	LineNo = 1;
}

int GetLineNo(void)
{
	return (LineNo);
}

char *GetLineBuffer(void)
{
	char *p = LineBuff;

	while (*p == 0x20 || *p == 0x09) // space or tab
		p++;
	return (p);
}

// �}�N���E�B���h�E�ɕ\������R�}���h���ǂ������ʂ��� (2006.2.24 yutaka)
int IsUpdateMacroCommand(void)
{
	char *p = GetLineBuffer();

	// �����Ƃ��ăE�F�C�g�E�X���[�v�n�R�}���h��ΏۂƂ���
	if (_strnicmp(p, "wait", 4) == 0)
		return 1;

	if (_strnicmp(p, "pause", 5) == 0)
		return 1;

	if (_strnicmp(p, "mpause", 6) == 0)
		return 1;

	return 0;
}


static BOOL LoadMacroFile(const wchar_t *FileName, int IBuff)
{
	wchar_t basename[MAX_PATH];
	char *basenameU8;
	size_t Len;

	if ((FileName[0]==0) || (IBuff>MAXNESTLEVEL-1)) {
		return FALSE;
	}

	// include�ɐ��������t�@�C������A�t�@�C�������L�^����B
	// �}�N���̃G���[�_�C�A���O�ŁA�t�@�C������\�����������߁B
	// (2013.9.8 yutaka)
	if (GetFileTitleW(FileName, basename, _countof(basename)) != 0)
		wcsncpy_s(basename, _countof(basename), FileName, _TRUNCATE);
	basenameU8 = ToU8W(basename);
	strncpy_s(&BuffHandleFileName[IBuff][0], MAXFILENAME, basenameU8, _TRUNCATE);
	free(basenameU8);

	BuffPtr[IBuff] = 0;

	Buff[IBuff] = LoadFileU8W(FileName, &Len);
	if (Buff[IBuff] == NULL) {
		BuffLen[IBuff] = 0;
		return FALSE;
	}
	if (Len>MAXBUFFLEN) {
		return FALSE;
	}
	BuffLen[IBuff] = Len;

	// �s�ԍ��z������B����ɂ��A�o�b�t�@�̃C���f�b�N�X����s�ԍ��ւ̕ϊ���
	// O(N)->O(logN)�Ō����ł���悤�ɂȂ�B
	// (2014.1.18 yutaka)
	{
		unsigned int i, n;
		n = 0;
		if (BuffLineNo[IBuff] == NULL) {
			BuffLineNo[IBuff] = malloc(MAX_LINENO * sizeof(BINT));
		}
		BuffLineNo[IBuff][n] = 0;
		for (i = 0 ; i < BuffLen[IBuff] ; i++) {
			if (Buff[IBuff][i] == 0x0A) {
				if (i == BuffLen[IBuff] - 1) {
					// �o�b�t�@�̍Ōオ���s�R�[�h�������ꍇ�A�������̍s�ԍ��͑��݂��Ȃ��B

				} else {
					if (n < MAX_LINENO - 1) {
						n++;
						BuffLineNo[IBuff][n] = i + 1;
					} else {
						// Out of memory

					}
				}
			}
		}

		BuffLineNoMaxIndex[IBuff] = n + 1;
	}

	return TRUE;
}


/**
 *	���ݎ��s���̃}�N���t�@�C���̃t�@�C������Ԃ�
 *	@return	�t�@�C����(UTF-8)
 */
const char *GetMacroFileName(void)
{
	return &BuffHandleFileName[INest][0];
}


// ���ݎ��s���̃}�N���t�@�C���̍s�ԍ���Ԃ� (2005.7.18 yutaka)
static int getCurrentLineNumber(BINT curpos, BINT *lineno, BINT linenomax)
{
	BINT i, no;

	no = 0;
	for (i = 0 ; i < linenomax ; i++) {
		if (curpos < lineno[i]) {
			no = i;
			break;
		}
	}
	// �Ō�̍s���p�[�X�����ہA�s�ԍ���Ԃ��Ă��Ȃ����������C�������B
	// (2014.7.6 yutaka)
	if (no == 0 && i == linenomax) {
		no = linenomax;
	}

	return (no);
}


BOOL GetRawLine()
{
	int i;
	BYTE b;

	LineStart = BuffPtr[INest];
	if (Buff[INest]==NULL) return FALSE;

	if (BuffPtr[INest]<BuffLen[INest])
		b = (Buff[INest])[BuffPtr[INest]];

	i = 0;
	memset(LineBuff, 0, sizeof(LineBuff));
	while ((BuffPtr[INest]<BuffLen[INest]) &&
		((b>=0x20) || (b==0x09)))
	{
		// LineBuff[]�̃o�b�t�@�T�C�Y�𒴂���ꍇ�̓G���[�Ƃ���B
		// �������A�}�N���������Ȃ�I������̂Ń_�C�A���O��\������悤�ɂ����ق���
		// ������������Ȃ��B
		// (2007.6.6 yutaka)
		// �o�b�t�@�T�C�Y�Ɏ��܂�͈͂ŃR�s�[����B
		// break ����Ƃ��ӂꂽ�������̍s�Ƃ��Ĉ�����̂� break ���Ȃ��B
		// (2007.6.9 maya)
		if (i < MaxLineLen-1) {
			LineBuff[i] = b;
		}
		i++;
		BuffPtr[INest]++;
		if (BuffPtr[INest]<BuffLen[INest])
			b = (Buff[INest])[BuffPtr[INest]];
	}
	LinePtr = 0;
	LineLen = (WORD)strlen(LineBuff);
	LineParsePtr = 0;

	// current line number (2005.7.18 yutaka)
	// �o�b�t�@�̃C���f�b�N�X���獂���ɍs�ԍ���������悤�ɂ����B(2014.1.18 yutaka)
	LineNo = getCurrentLineNumber(BuffPtr[INest], BuffLineNo[INest], BuffLineNoMaxIndex[INest]);

	while ((BuffPtr[INest]<BuffLen[INest]) &&
		(b<0x20) && (b!=0x09))
	{
		BuffPtr[INest]++;
		if (BuffPtr[INest]<BuffLen[INest])
			b = (Buff[INest])[BuffPtr[INest]];
	}
	return ((LineLen>0) || (BuffPtr[INest]<BuffLen[INest]));
}

BOOL GetNewLine()
{
	BOOL Ok;
	BYTE b;

	do {
		Ok = GetRawLine();
		if (! Ok && (INest>0))
			do {
				CloseBuff(INest);
				INest--;
				Ok = GetRawLine();
			} while (!Ok && (INest>0));
		if (! Ok) return FALSE;

		b = GetFirstChar();
		LinePtr--;
	} while ((b==0) || (b==':'));

	return TRUE;
}

BOOL RegisterLabels(int IBuff)
{
	BYTE b;
	TName LabName;
	WORD Err;
	TVariableType VarType;
	TVarId VarId;
	TStrVal tmp;

	if (Buff[IBuff]==NULL) {
		return FALSE;
	}
	BuffPtr[IBuff] = 0;

	while (GetRawLine()) {
		Err = 0;

		b = GetFirstChar();
		if (b==':') {
			if (GetLabelName(LabName) && (GetFirstChar()==0)) {
				if (CheckVar(LabName, &VarType, &VarId)) {
					Err = ErrLabelAlreadyDef;
				}
				else {
					if (! NewLabVar(LabName,BuffPtr[IBuff],(WORD)IBuff)) {
						Err = ErrTooManyLabels;
					}
				}
			}
			else {
				Err = ErrSyntax;
			}
		}
		else {
		    LinePtr--;
		}

		/* ���̍s�ֈڂ��O�ɁAC����R�����g��T�����߁A�s���܂ŃX�L��������B*/
		while ((b=GetFirstChar()) != 0) {
			if (b=='"' || b=='\'' || b=='#') {
				LinePtr--;
				GetString(tmp, &Err);
			}
		}

		if (Err>0) {
			DispErr(Err);
		}
	}

	if (!IsCommentClosed()) {
		DispErr(ErrCloseComment);
	}

	BuffPtr[IBuff] = 0;
	InitLineNo(); // (2005.7.18 yutaka)
	return TRUE;
}

BOOL InitBuff(const wchar_t *FileName)
{
	int i;

	SP = 0;
	NextFlag = FALSE;
	EndWhileFlag = 0;
	BreakFlag = 0;
	for (i=0 ; i<=MAXNESTLEVEL-1 ; i++) {
		Buff[i] = NULL;
	}
	INest = 0;
	if (! LoadMacroFile(FileName, INest)) {
		return FALSE;
	}
	if (! RegisterLabels(INest)) {
		return FALSE;
	}
	return TRUE;
}

void CloseBuff(int IBuff)
{
	int i;

	DelLabVar((WORD)IBuff);
	for (i=IBuff ; i<=MAXNESTLEVEL-1 ; i++) {
		free(BuffLineNo[i]);
		/* �|�C���^�̏������R����C�������B4.81�ł̃f�O���[�h�B
		 * (2014.3.4 yutaka)
		 */
		BuffLineNo[i] = NULL;
		BuffLineNoMaxIndex[i] = 0;
	}

	while ((SP>0) && (LevelStack[SP-1]>=IBuff)) {
		SP--;
	}
}

void JumpToLabel(int ILabel)
{
	BINT Ptr;
	WORD Level;

	CopyLabel((WORD)ILabel, &Ptr,&Level);
	if (Level < INest) {
		INest = Level;
		CloseBuff(INest+1);
	}
	BuffPtr[INest] = Ptr;
}

WORD CallToLabel(int ILabel)
{
	BINT Ptr;
	WORD Level;

	CopyLabel((WORD)ILabel, &Ptr,&Level);
	if (Level != INest) {
		return ErrCantCall;
	}

	if (SP>=MAXSP) {
		return ErrStackOver;
	}

	PtrStack[SP] = BuffPtr[INest];
	LevelStack[SP] = INest;
	TypeStack[SP] = CtlCall;
	SP++;

	BuffPtr[INest] = Ptr;
	return 0;
}

WORD ReturnFromSub()
{
	if ((SP<1) || (TypeStack[SP-1]!=CtlCall)) {
		return ErrInvalidCtl;
	}

	SP--;
	if (LevelStack[SP] < INest) {
		INest = LevelStack[SP];
		CloseBuff(INest+1);
	}
	BuffPtr[INest] = PtrStack[SP];
	return 0;
}

BOOL BuffInclude(const wchar_t *FileName)
{
	if (INest>=MAXNESTLEVEL-1) {
		return FALSE;
	}
	INest++;
	if (LoadMacroFile(FileName, INest)) {
		if (RegisterLabels(INest)) {
			return TRUE;
		}
		else {
			CloseBuff(INest);
			INest--;
		}
	}
	else {
		INest--;
	}

	return FALSE;
}

BOOL ExitBuffer()
{
	if (INest<1) {
		return FALSE;
	}
	CloseBuff(INest);
	INest--;
	return TRUE;
}

int SetForLoop()
{
	if (SP>=MAXSP) {
		return ErrStackOver;
	}

	PtrStack[SP] = LineStart;
	LevelStack[SP] = INest;
	TypeStack[SP] = CtlFor;
	SP++;
	return 0;
}

void LastForLoop()
{
	if ((SP<1) || (TypeStack[SP-1]!=CtlFor)) {
		return;
	}
	PtrStack[SP-1] = INVALIDPTR;
}

BOOL CheckNext()
{
	if (NextFlag) {
		NextFlag = FALSE;
		return TRUE;
	}
	return FALSE;
}

int NextLoop()
{
	if ((SP<1) || (TypeStack[SP-1]!=CtlFor)) {
		return ErrInvalidCtl;
	}

	NextFlag = (PtrStack[SP-1]!=INVALIDPTR);
	if (! NextFlag) // exit from loop
	{
		SP--;
		return 0;
	}
	if (LevelStack[SP-1] < INest) {
		INest = LevelStack[SP-1];
		CloseBuff(INest+1);
	}
	BuffPtr[INest] = PtrStack[SP-1];
	return 0;
}

int SetWhileLoop()
{
	if (SP>=MAXSP) {
		return ErrStackOver;
	}

	PtrStack[SP] = LineStart;
	LevelStack[SP] = INest;
	TypeStack[SP] = CtlWhile;
	SP++;
	return 0;
}

void EndWhileLoop()
{
	EndWhileFlag = 1;
}

int BackToWhile(BOOL flag)
{
	if ((SP<1) || (TypeStack[SP-1]!=CtlWhile)) {
		return ErrInvalidCtl;
	}

	SP--;
	if (LevelStack[SP] < INest) {
		INest = LevelStack[SP];
		CloseBuff(INest+1);
	}
	if (flag) {
		BuffPtr[INest] = PtrStack[SP];
	}
	return 0;
}

WORD BreakLoop(WORD WId)
{
	if (SP<1) {
		return ErrInvalidCtl;
	}

	switch (TypeStack[SP-1]) {
		case CtlFor:
		case CtlWhile:
			if (WId == RsvContinue) {
				ContinueFlag = TRUE;
			}
			else {
				SP--;
				if (LevelStack[SP] < INest) {
					INest = LevelStack[SP];
					CloseBuff(INest+1);
				}
			}
			BreakFlag = 1;
			break;
		default:
			return ErrInvalidCtl;
	}
	return 0;
}
