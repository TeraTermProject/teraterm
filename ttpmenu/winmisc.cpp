/*
 * Copyright (C) S.Hayakawa NTT-IT 1998-2002
 * (C) 2002- TeraTerm Project
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

#define		STRICT

#include	"codeconv.h"
#include	"i18n.h"
#include	"ttcommdlg.h"

#include	"winmisc.h"


/* ==========================================================================
	Function Name	: (void) SetDlgPos()
	Outline			: ダイアログの位置を移動する
	Arguments		: HWND		hWnd	(in)	ダイアログのウインドウハンドル
					: int		pos		(in)	移動場所を示す値
	Return Value	: なし
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
void SetDlgPos(HWND hWnd, int pos)
{
	int		x;
	int		y;
	RECT	rect;

	::GetWindowRect(hWnd, &rect);

	int	cx		= ::GetSystemMetrics(SM_CXFULLSCREEN);
	int	cy		= ::GetSystemMetrics(SM_CYFULLSCREEN);
	int	xsize	= rect.right - rect.left;
	int	ysize	= rect.bottom - rect.top;

	switch (pos) {
	case POSITION_LEFTTOP:
		x = 0;
		y = 0;
		break;
	case POSITION_LEFTBOTTOM:
		x = 0;
		y = cy - ysize;
		break;
	case POSITION_RIGHTTOP:
		x = cx - xsize;
		y = 0;
		break;
	case POSITION_RIGHTBOTTOM:
		x = cx - xsize;
		y = cy - ysize;
		break;
	case POSITION_CENTER:
		x = (cx - xsize) / 2;
		y = (cy - ysize) / 2;
		break;
	case POSITION_OUTSIDE:
		x = cx;
		y = cy;
		break;
	}

	::MoveWindow(hWnd, x, y, xsize, ysize, TRUE);
}

/* ==========================================================================
	Function Name	: (BOOL) EnableItem()
	Outline			: ダイアログアイテムを有効／無効にする
	Arguments		: HWND		hWnd		(in)	ダイアログのハンドル
					: int		idControl	(in)	ダイアログアイテムの ID
					: BOOL		flag
	Return Value	: 成功 TRUE
					: 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL EnableItem(HWND hWnd, int idControl, BOOL flag)
{
	HWND	hWndItem;

	if ((hWndItem = ::GetDlgItem(hWnd, idControl)) == NULL)
		return FALSE;

	return ::EnableWindow(hWndItem, flag);
}

/* ==========================================================================
	Function Name	: (void) EncodePassword()
	Outline			: パスワードをエンコード(?)する。
					  エンコードされた文字列をデコードする
					  入出力は文字列またはバイナリとなる
	Arguments		: const char *cPassword			(in)	変換する文字列
					: char		 *cEncodePassword	(out)	変換された文字列
	Return Value	: なし
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
void EncodePassword(const char *cPassword, char *cEncodePassword)
{
	DWORD	dwCnt;
	DWORD	dwPasswordLength = ::lstrlenA(cPassword);

	for (dwCnt = 0; dwCnt < dwPasswordLength; dwCnt++)
		cEncodePassword[dwPasswordLength - 1 - dwCnt] = cPassword[dwCnt] ^ 0xff;

	cEncodePassword[dwPasswordLength] = '\0';
}

/* ==========================================================================
	Function Name	: (BOOL) OpenFileDlg()
	Outline			: 「ファイルを開く」ダイアログを開き、指定されたファイル
					: パスを指定されたアイテムに送る
	Arguments		: HWND		hWnd		(in)	親ウインドウのハンドル
					: UINT		editCtl		(in)	アイテムの ID
					: char		*title		(in)	ウインドウタイトル
					: char		*filter		(in)	表示するファイルのフィルタ
					: char		*defaultDir	(in,out)デフォルトのパス
					: size_t    max_path
	Return Value	: 成功 TRUE
					: 失敗 FALSE
	Reference		: 
	Renewal			: 
	Notes			: 
	Attention		: 
	Up Date			: 
   ======1=========2=========3=========4=========5=========6=========7======= */
BOOL OpenFileDlg(HWND hWnd, UINT editCtl, const wchar_t *title, const wchar_t *filter, wchar_t *defaultDir, size_t max_path)
{
	wchar_t			*szDirName;
	wchar_t			szFile[MAX_PATH] = L"";
	wchar_t			*szPath;

	szDirName	= (wchar_t *) malloc(MAX_PATH * sizeof(wchar_t));

	if (editCtl != 0xffffffff) {
		::GetDlgItemTextW(hWnd, editCtl, szDirName, MAX_PATH);
		wcscpy(szDirName, defaultDir);
	}

	if (*szDirName == '"')
		szDirName++;
	if (szDirName[wcslen(szDirName) - 1] == '"')
		szDirName[wcslen(szDirName) - 1] = '\0';

	wchar_t *ptr = wcsrchr(szDirName, '\\');
	if (ptr == NULL) {
		wcscpy(szFile, szDirName);
		if (defaultDir != NULL && *szDirName == 0)
			wcscpy(szDirName, defaultDir);
	} else {
		*ptr = 0;
		wcscpy(szFile, ptr + 1);
	}

	TTOPENFILENAMEW	ofn = {};
	ofn.hwndOwner		= hWnd;
	ofn.lpstrFilter		= filter;
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.lpstrTitle		= title;
	ofn.lpstrInitialDir	= szDirName;
	ofn.Flags			= OFN_HIDEREADONLY | OFN_NODEREFERENCELINKS;

	if (::TTGetOpenFileNameW(&ofn, &szPath) == FALSE) {
		free(szDirName);
		return	FALSE;
	}

	::SetDlgItemTextW(hWnd, editCtl, szPath);

	wcscpy_s(defaultDir, max_path, szPath);

	free(szDirName);
	free(szPath);

	return	TRUE;
}

wchar_t* lwcsstri(wchar_t *s1, const wchar_t *s2)
{
	size_t	dwLen1 = wcslen(s1);
	size_t	dwLen2 = wcslen(s2);

	for (size_t dwCnt = 0; dwCnt <= dwLen1; dwCnt++) {
		size_t dwCnt2;
		for (dwCnt2 = 0; dwCnt2 <= dwLen2; dwCnt2++)
			if (towlower(s1[dwCnt + dwCnt2]) != towlower(s2[dwCnt2]))
				break;
		if (dwCnt2 > dwLen2)
			return s1 + dwCnt;
	}

	return NULL;
}

BOOL SetForceForegroundWindow(HWND hWnd)
{
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
#endif
	DWORD	foreId, targId, svTmOut;

	foreId = ::GetWindowThreadProcessId(::GetForegroundWindow(), NULL);
	targId = ::GetWindowThreadProcessId(hWnd, NULL);
	::AttachThreadInput(targId, foreId, TRUE);
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, (void *)&svTmOut, 0);
	::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, 0);
	BOOL	ret = ::SetForegroundWindow(hWnd);
	::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (void *)(UINT_PTR)svTmOut, 0);
	::AttachThreadInput(targId, foreId, FALSE);

	return	ret;
}

void UTIL_get_lang_msgW(const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const wchar_t *iniFile)
{
	wchar_t *str;
	GetI18nStrWW("TTMenu", key, def, iniFile, &str);
	wcscpy_s(buf, buf_len, str);
	free(str);
}

int UTIL_get_lang_font(const char *key, HWND dlg, PLOGFONTA logfont, HFONT *font, const char *iniFile)
{
	if (GetI18nLogfont("TTMenu", key, logfont,
					   GetDeviceCaps(GetDC(dlg),LOGPIXELSY),
					   iniFile) == FALSE) {
		return FALSE;
	}

	if ((*font = CreateFontIndirectA(logfont)) == NULL) {
		return FALSE;
	}

	return TRUE;
}

LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                   WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CHAR:
		if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
			char chars[] = { (char) wParam, 0 };

			SendMessage(control, EM_REPLACESEL, (WPARAM) TRUE,
			            (LPARAM) (char *) chars);
			return 0;
		}
	}

	return CallWindowProc((WNDPROC) GetWindowLongPtr(control, GWLP_USERDATA),
	                      control, msg, wParam, lParam);
}

/* ==========================================================================
	Function Name	: (int) Encrypt2EncDec()
	Outline			: パスワードの暗号化/復号処理(aes_256_ctr)
	Arguments		:										暗号化  復号
					: char					*szPassword 	(in)	(out)	パスワード(暗号化/復号対象の文字列)
					: const unsigned char	*szEncryptKey	(in)	(in)	秘密鍵(パスワードを暗号化/復号するためのパスワード)
					: Encrypt2ProfileP		profile			(out)	(in)	パスワードプロファイル
					: int					encrypt			(in)	(in)	1:暗号化、0:復号
	Return Value	: 成功 1
					: 失敗 0
	Reference		:
	Renewal			:
	Notes			:
	Attention		:
	Up Date			:
   ======1=========2=========3=========4=========5=========6=========7======= */
int Encrypt2EncDec(char *szPassword, const unsigned char *szEncryptKey, Encrypt2ProfileP profile, int encrypt)
{
	unsigned char TmpKeyIV[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH];
	unsigned char Key[EVP_MAX_KEY_LENGTH], IV[EVP_MAX_IV_LENGTH];
	BIO *Bmem = NULL, *Benc = NULL, *Bio = NULL;
	EVP_CIPHER_CTX *ctx;
	Encrypt2Profile Lprofile;
	unsigned char Buf[ENCRYPT2_PROFILE_LEN - SHA512_DIGEST_LENGTH];
	unsigned char Hash[SHA512_DIGEST_LENGTH];
	unsigned int HashLen;
	int ret = 0;

	if (encrypt) {
		memcpy(profile->Tag, ENCRYPT2_TAG, sizeof(ENCRYPT2_TAG) - 1);
		if (RAND_bytes(profile->PassSalt, ENCRYPT2_SALTLEN) <= 0 ||
			RAND_bytes(profile->EncSalt, ENCRYPT2_SALTLEN) <= 0) {
			goto end;
		}
	} else {
		if (CRYPTO_memcmp(profile->Tag, ENCRYPT2_TAG, sizeof(ENCRYPT2_TAG) - 1) != 0) {
			goto end;
		}
	}

	// 鍵導出
	if (PKCS5_PBKDF2_HMAC((const char *)szEncryptKey, (int)strlen((const char *)szEncryptKey),
						  (const unsigned char *)&(profile->PassSalt), ENCRYPT2_SALTLEN,
						  ENCRYPT2_ITER2, (EVP_MD *)ENCRYPT2_DIGEST,
						  ENCRYPT2_IKLEN + ENCRYPT2_IVLEN, TmpKeyIV) != 1) {
		goto end;
	}
	memcpy(Key, TmpKeyIV, ENCRYPT2_IKLEN);
	memcpy(IV, TmpKeyIV + ENCRYPT2_IKLEN, ENCRYPT2_IVLEN);

	// 準備
	if ((Bmem = BIO_new(BIO_s_mem())) == NULL ||
		(Benc = BIO_new(BIO_f_cipher())) == NULL ||
		BIO_get_cipher_ctx(Benc, &ctx) != 1 ||
		EVP_CipherInit_ex(ctx, ENCRYPT2_CIPHER, NULL, Key, IV, encrypt) != 1 ||
		(Bio = BIO_push(Benc, Bmem))  == NULL) {
		goto end;
	}

	if (encrypt) {
		int len;
		// 暗号化
		len = (int)strlen(szPassword);
		memcpy(Buf, szPassword, len);
		memset(Buf + len, 0x00, ENCRYPT2_PWD_MAX_LEN - len);	// nullパディング
		if (BIO_write(Bio, Buf, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_write(Bio, profile->EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN ||
			BIO_flush(Bio) != 1 ||
			BIO_read(Bmem, profile->PassStr, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_read(Bmem, profile->EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN) {
			goto end;
		}
		// hmac格納
		if ((PKCS5_PBKDF2_HMAC((const char *)szEncryptKey, (int)strlen((const char *)szEncryptKey),
							   (const unsigned char *)&(profile->EncSalt), ENCRYPT2_SALTLEN,
							   ENCRYPT2_ITER1, (EVP_MD *)ENCRYPT2_DIGEST,
							   SHA512_DIGEST_LENGTH, Key) != 1) ||
			HMAC(ENCRYPT2_DIGEST, Key, SHA512_DIGEST_LENGTH,
				 (const unsigned char *)profile, ENCRYPT2_PROFILE_LEN - SHA512_DIGEST_LENGTH,
				 Hash, &HashLen) == NULL ||
			BIO_write(Bio, Hash, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
			BIO_flush(Bio) != 1 ||
			BIO_read(Bmem, profile->EncHash, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH) {
			goto end;
		}
		ret = 1;
	} else {
		// 復号
		if (BIO_write(Bmem, profile->PassStr, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_write(Bmem, profile->EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN ||
			BIO_write(Bmem, profile->EncHash, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
			BIO_flush(Bmem) != 1 ||
			BIO_read(Bio, Lprofile.PassStr, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_read(Bio, Lprofile.EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN ||
			BIO_read(Bio, Lprofile.EncHash, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH) {
			goto end;
		}
		// hmac比較
		if ((PKCS5_PBKDF2_HMAC((const char *)szEncryptKey, (int)strlen((const char *)szEncryptKey),
							   (const unsigned char *)&(profile->EncSalt), ENCRYPT2_SALTLEN,
							   ENCRYPT2_ITER1, (EVP_MD *)ENCRYPT2_DIGEST,
							   SHA512_DIGEST_LENGTH, Key) != 1) ||
			HMAC(ENCRYPT2_DIGEST, Key, SHA512_DIGEST_LENGTH,
				 (const unsigned char *)profile, ENCRYPT2_PROFILE_LEN - SHA512_DIGEST_LENGTH,
				 Hash, &HashLen) == NULL ) {
			goto end;
		}
		memcpy(szPassword, Lprofile.PassStr, ENCRYPT2_PWD_MAX_LEN);
		if (CRYPTO_memcmp(Hash, Lprofile.EncHash, SHA512_DIGEST_LENGTH) == 0) {
			szPassword[ENCRYPT2_PWD_MAX_LEN] = 0;
			ret = 1;	// 一致
		} else {
			szPassword[0] = 0;
			ret = 0;	// 不一致
		}
	}

 end:
	BIO_free(Benc);
	BIO_free(Bmem);
	return ret;
}
