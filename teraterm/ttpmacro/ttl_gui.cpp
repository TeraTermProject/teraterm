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

/* TTMACRO.EXE, Tera Term Language interpreter */

#include "teraterm.h"
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <crtdbg.h>
#include <string.h>
#include <errno.h>
#include "ttmdlg.h"
#include "ttmparse.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmenc.h"
#include "ttmenc2.h"
#include "tttypes.h"
#include "ttmacro.h"
#include "ttl.h"
#include "ttl_gui.h"
#include "codeconv.h"
#include "ttlib.h"
#include "dlglib.h"
#include "win32helper.h"

// add 'clipb2var' (2006.9.17 maya)
WORD TTLClipb2Var()
{
	WORD Err;
	TVarId VarId;
	static char *cbbuff;
	static int cblen;
	int Num = 0;

	Err = 0;
	GetStrVar(&VarId, &Err);
	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&Num, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Num == 0) {
		if (cbbuff != NULL) {
			free(cbbuff);
			cbbuff = NULL;
		}
		wchar_t *cbbuffW = GetClipboardTextW(NULL, FALSE);
		if (cbbuffW == NULL) {
			// クリップボードを開けなかった。またはテキストデータではなかった。
			cblen = 0;
			SetResult(0);
			return 0;
		}
		cbbuff = ToU8W(cbbuffW);
		free(cbbuffW);
		cblen = strlen(cbbuff);
	}

	if (cbbuff != NULL && Num >= 0 && Num * (MaxStrLen - 1) < cblen) {
		char buf[MaxStrLen];
		if (strncpy_s(buf ,sizeof(buf), cbbuff + Num * (MaxStrLen-1), _TRUNCATE) == STRUNCATE)
			SetResult(2); // Copied string is truncated.
		else {
			SetResult(1);
		}
		SetStrVal(VarId, buf);
	}
	else {
		SetResult(0);
	}

	return Err;
}

// add 'var2clipb' (2006.9.17 maya)
WORD TTLVar2Clipb()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	BOOL r = CBSetTextW(NULL, wc::fromUtf8(Str), 0);
	// 0 == クリップボードを開けなかった
	// 1 == クリップボードへのコピーに成功した
	SetResult(r ? 1 : 0);

	return Err;
}

// add 'filenamebox' (2007.9.13 maya)
WORD TTLFilenameBox()
{
	TStrVal Str1;
	WORD Err;
	TVariableType ValType;
	TVarId VarId;
	BOOL SaveFlag = FALSE;
	TStrVal InitDir = "";
	wc InitDirT;

	Err = 0;
	GetStrVal(Str1,&Err);
	wc Str1T = wc::fromUtf8(Str1);

	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // dialogtype
		GetIntVal(&SaveFlag, &Err);
		if (Err!=0) return Err;

		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) { // initdir
			GetStrVal(InitDir, &Err);
			if (Err!=0) return Err;
			InitDirT = InitDir;
		}
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) && (ValType==TypString)) {
		wchar_t *FNFilter = GetCommonDialogFilterWW(NULL, UILanguageFileW);

		OPENFILENAMEW ofn = {};
		wchar_t filename[MAX_PATH];
		filename[0] = 0;
		ofn.lStructSize     = get_OPENFILENAME_SIZEW();
		ofn.hwndOwner       = GetHWND();
		ofn.lpstrTitle      = Str1T;
		ofn.lpstrFile       = filename;
		ofn.nMaxFile        = _countof(filename);
		ofn.lpstrFilter     = FNFilter;
		ofn.lpstrInitialDir = NULL;
		if (strlen(InitDir) > 0) {
			ofn.lpstrInitialDir = InitDirT;
		}
		BringupWindow(GetHWND());
		BOOL ret;
		if (SaveFlag) {
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ret = GetSaveFileNameW(&ofn);
		}
		else {
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ret = GetOpenFileNameW(&ofn);
		}
		free(FNFilter);
		char *filenameU8 = ToU8W(filename);
		SetStrVal(VarId, filenameU8);
		free(filenameU8);

		SetResult(ret);
	}
	return Err;
}

WORD TTLGetPassword()
{
	TStrVal Str, Str2, Temp2;
	wchar_t *passwd_encW;
	WORD Err;
	TVarId VarId;
	int result = 0;  /* failure */

	Err = 0;
	GetStrVal(Str,&Err);  // ファイル名
	GetStrVal(Str2,&Err);  // キー名
	GetStrVar(&VarId,&Err);  // パスワード更新時にパスワードを格納する変数
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetStrVal(VarId,"");
	if (Str[0]==0) return Err;
	if (Str2[0]==0) return Err;

	GetAbsPath(Str,sizeof(Str));

	wc key = wc::fromUtf8(Str2);
	wc ini = wc::fromUtf8(Str);

	hGetPrivateProfileStringW(L"Password", key, L"", ini, &passwd_encW);
	if (passwd_encW[0] == L'\0')	// password not exist
	{
		wchar_t input_string[MaxStrLen] = {};
		size_t Temp2_len = sizeof(Temp2);
		free(passwd_encW);
		Err = OpenInpDlg(input_string, key, L"Enter password", L"", TRUE);
		if (Err == IDCLOSE) {
			// 閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
			TTLStatus = IdTTLEnd;
			return 0;
		} else {
			Err = 0;
			WideCharToUTF8(input_string, NULL, Temp2, &Temp2_len);
			if (Temp2[0]!=0) {
				char TempA[512];
				Encrypt(Temp2, TempA);
				if (WritePrivateProfileStringW(L"Password", key, wc::fromUtf8(TempA), ini) != 0) {
					result = 1;  /* success */
				}
			}
		}
	}
	else {// password exist
		Decrypt((u8)passwd_encW, Temp2);
		free(passwd_encW);
		result = 1;  /* success */
	}

	if (result == 1) {
		SetStrVal(VarId,Temp2);
	}
	// パスワード入力がないときは変数を更新しない

	SetResult(result);  // 成功可否を設定する。
	return Err;
}

// getpassword2 <filename> <password name> <strvar> <encryptstr>
WORD TTLGetPassword2()
{
	TStrVal FileNameStr, KeyStr, EncryptStr;
	TVarId PassStr;
	TStrVal inputU8;
	WORD Err = 0;
	int result = 0;

	GetStrVal(FileNameStr, &Err);	// ファイル名
	GetStrVal(KeyStr, &Err);		// キー名
	GetStrVar(&PassStr, &Err);		// パスワード更新時にパスワードを格納する変数
	SetStrVal(PassStr, "");
	GetStrVal(EncryptStr, &Err);	// パスワード文字列を暗号化するためのパスワード（共通鍵）
	if ((Err == 0) && (GetFirstChar() != 0)) {
		Err = ErrSyntax;
	}
	if (Err != 0) {
		return Err;
	}
	if (FileNameStr[0] == 0 ||
		KeyStr[0] == 0 ||
		EncryptStr[0] == 0) {
		return ErrSyntax;
	}

	GetAbsPath(FileNameStr, sizeof(FileNameStr));
	if (Encrypt2IsPassword(wc::fromUtf8(FileNameStr), KeyStr) == 0) {
		// キー名にマッチするエントリ無し
		wchar_t inputW[MaxStrLen] = {};
		wc key = wc::fromUtf8(KeyStr);
		size_t inputU8len = sizeof(inputU8);

		OpenInpDlg(inputW, key, L"Enter password", L"", TRUE);
		WideCharToUTF8(inputW, NULL, inputU8, &inputU8len);
		if (inputU8[0] != 0) {
			result = Encrypt2SetPassword(wc::fromUtf8(FileNameStr), KeyStr, inputU8, EncryptStr);
		}
	} else {
		// キー名にマッチするエントリ有り
		result = Encrypt2GetPassword(wc::fromUtf8(FileNameStr), KeyStr, inputU8, EncryptStr);
	}

	// パスワード入力がないときは変数を更新しない
	if (result == 1) {
		SetStrVal(PassStr, inputU8);
	}
	SetResult(result);	// 成功可否を設定する。
	return 0;
}

WORD TTLInputBox(BOOL Paswd)
{
	TStrVal Str1, Str2, Str3;
	WORD Err, P;
	TVariableType ValType;
	TVarId VarId;
	int sp = 0;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if (Err!=0) return Err;

	if (!Paswd && CheckParameterGiven()) {
		// get 3rd arg(optional)
		P = LinePtr;
		GetStrVal(Str3,&Err);
		if (Err == ErrTypeMismatch) {
			strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
			LinePtr = P;
			Err = 0;
		}
	}
	else {
		strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
	}

	// get 4th(3rd) arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&sp, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (sp) {
		RestoreNewLine(Str1);
	}

	SetInputStr("");
	if (CheckVar("inputstr",&ValType,&VarId) && (ValType==TypString)) {
		wchar_t input_string[MaxStrLen];
		Err = OpenInpDlg(input_string,wc::fromUtf8(Str1),wc::fromUtf8(Str2),wc::fromUtf8(Str3),Paswd);
		if (Err == IDCLOSE) {
			// 閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
		  	TTLStatus = IdTTLEnd;
			SetStrVal(VarId, "");
		} else {
			char *u8 = ToU8W(input_string);
			SetStrVal(VarId, u8);
			free(u8);
		}
		Err = 0;
	}
	return Err;
}

WORD TTLDirnameBox()
{
	TStrVal Title;
	WORD Err;
	TVariableType ValType;
	TVarId VarId;
	TStrVal InitDir = "";
	BOOL ret;

	Err = 0;
	GetStrVal(Title, &Err);
	if (Err != 0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // initdir
		GetStrVal(InitDir, &Err);
		if (Err != 0) return Err;
	}

	if ((Err == 0) && (GetFirstChar() != 0))
		Err = ErrSyntax;
	if (Err != 0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) &&
	    (ValType == TypString)) {
		BringupWindow(GetHWND());
		wchar_t *buf;
		if (doSelectFolderW(GetHWND(), wc::fromUtf8(InitDir), wc::fromUtf8(Title), &buf)) {
			const char *bufU8 = ToU8W(buf);
			SetInputStr((PCHAR)bufU8);
			free((void *)bufU8);
			free(buf);
			ret = 1;
		}
		else {
			ret = 0;
		}
		SetResult(ret);
	}
	return Err;
}

enum MessageCommandBoxId {
	IdMsgBox,
	IdYesNoBox,
	IdStatusBox,
	IdListBox,
};

static int MessageCommand(MessageCommandBoxId BoxId, LPWORD Err)
{
	TStrVal Str1, Str2;
	int sp = 0;
	int ret;
	int i, ary_size;
	int sel = 0;
	TVarId VarId;
	TStrVal StrTmp;
	int ext = 0;
	int width = 0;
	int height = 0;

	*Err = 0;
	GetStrVal2(Str1, Err, TRUE);
	GetStrVal2(Str2, Err, TRUE);
	if (*Err!=0) return 0;

	if (BoxId != IdListBox) {
		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) {
			GetIntVal(&sp, Err);
		}
		if ((*Err==0) && (GetFirstChar()!=0))
			*Err = ErrSyntax;
		if (*Err!=0) return 0;
	}

	if (sp) {
		RestoreNewLine(Str1);
	}

	if (BoxId==IdMsgBox) {
		ret = OpenMsgDlg(wc::fromUtf8(Str1),wc::fromUtf8(Str2),FALSE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.5 yutaka)
		if (ret == IDCANCEL) {
			TTLStatus = IdTTLEnd;
		}
	} else if (BoxId==IdYesNoBox) {
		ret = OpenMsgDlg(wc::fromUtf8(Str1),wc::fromUtf8(Str2),TRUE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.6 yutaka)
		if (ret == IDCLOSE) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);
	}
	else if (BoxId==IdStatusBox) {
		OpenStatDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2));

	} else if (BoxId==IdListBox) {
		//  リストボックスの選択肢を取得する。
		GetStrAryVar(&VarId, Err);
		while (CheckParameterGiven()) {
			GetStrVal2(StrTmp, Err, TRUE);
			if (*Err == 0) {
				if (_wcsicmp(wc::fromUtf8(StrTmp), L"dblclick=on") == 0) {
					ext |= ExtListBoxDoubleclick;
				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"minmaxbutton=on") == 0) {
					ext |= ExtListBoxMinmaxbutton;
				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"minimize=on") == 0) {
					ext |=  ExtListBoxMinimize;
					ext &= ~ExtListBoxMaximize;
				} else if (_wcsicmp(wc::fromUtf8(StrTmp), L"maximize=on") == 0) {
					ext &= ~ExtListBoxMinimize;
					ext |=  ExtListBoxMaximize;
				} else if (_wcsnicmp(wc::fromUtf8(StrTmp), L"listboxsize=", 5) == 0) {
				  	wchar_t dummy1[24], dummy2[24];
					if (swscanf_s(wc::fromUtf8(StrTmp), L"%[^=]=%d%[xX]%d", dummy1, 24, &width, dummy2, 24, &height) == 4) {
						if (width < 0 || height < 0) {
							*Err = ErrSyntax;
							break;
						} else {
							ext |= ExtListBoxSize;
						}
					} else {
						*Err = ErrSyntax;
						break;
					}
				} else if (sscanf_s(StrTmp, "%d", &sel) != 1) {
					*Err = ErrSyntax;
					break;
				}
			} else {
				break;
			}
		}
		if (*Err==0 && GetFirstChar()!=0)
			*Err = ErrSyntax;
		if (*Err!=0) return 0;

		ary_size = GetStrAryVarSize(VarId);
		if (sel < 0 || sel >= ary_size) {
			sel = 0;
		}

		wchar_t **s = (wchar_t **)calloc(ary_size + 1, sizeof(wchar_t *));
		if (s == NULL) {
			*Err = ErrFewMemory;
			return -1;
		}
		for (i = 0 ; i < ary_size ; i++) {
			TVarId VarId2;
			VarId2 = GetStrVarFromArray(VarId, i, Err);
			if (*Err!=0) return -1;
			s[i] = ToWcharU8(StrVarPtr(VarId2));
		}
		if (s[0] == NULL) {
			*Err = ErrSyntax;
			return -1;
		}

		// return
		//   0以上: 選択項目
		//   -1: キャンセル
		//	 -2: close
		ret = OpenListDlg(wc::fromUtf8(Str1), wc::fromUtf8(Str2), s, sel, ext, width, height);

		for (i = 0 ; i < ary_size ; i++) {
			free((void *)s[i]);
		}
		free(s);

		// リストボックスの閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
		if (ret == -2) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);

	}
	return 0;
}

// リストボックス
// (2013.3.13 yutaka)
WORD TTLListBox()
{
	WORD Err;
	int ret;

	ret = MessageCommand(IdListBox, &Err);
	SetResult(ret);
	return Err;
}

WORD TTLMessageBox()
{
	WORD Err;

	MessageCommand(IdMsgBox, &Err);
	return Err;
}

WORD TTLStatusBox()
{
	WORD Err;

	MessageCommand(IdStatusBox, &Err);
	return Err;
}

WORD TTLYesNoBox()
{
	WORD Err;
	int YesNo;

	YesNo = MessageCommand(IdYesNoBox, &Err);
	if (Err!=0) return Err;
	if (YesNo==IDOK)
		YesNo = 1;	// Yes
	else
		YesNo = 0;	// No
	SetResult(YesNo);
	return Err;
}
