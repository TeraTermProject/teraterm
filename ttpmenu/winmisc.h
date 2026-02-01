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

#ifndef WINMISC_H
#define	WINMISC_H

#include	<windows.h>

#define		LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING
#include	<openssl/sha.h>
#include	<openssl/evp.h>
#include	<openssl/rand.h>
#include	<openssl/hmac.h>
#include	<openssl/crypto.h>

// Window Position
#define		POSITION_LEFTTOP		0x00
#define		POSITION_LEFTBOTTOM		0x01
#define		POSITION_RIGHTTOP		0x02
#define		POSITION_RIGHTBOTTOM	0x03
#define		POSITION_CENTER			0x04
#define		POSITION_OUTSIDE		0x05

// パスワード暗号用
#define		ENCRYPT2_SALTLEN		16
#define		ENCRYPT2_CIPHER			EVP_aes_256_ctr()
#define		ENCRYPT2_DIGEST			EVP_sha512()
#define		ENCRYPT2_ITER1			1001	// パスワード以外
#define		ENCRYPT2_ITER2			210001	// パスワード用 (参考 https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2)
#define		ENCRYPT2_IKLEN			32
#define		ENCRYPT2_IVLEN			16
#define		ENCRYPT2_TAG			"\252\002"
#define		ENCRYPT2_PWD_MAX_LEN	161

typedef struct {									//	   計 259バイト(MAX_PATH - 1バイト)
	unsigned char Tag[2];							// 平文     2 Encrypt2識別タグ (ENCRYPT2_TAG)
	unsigned char PassSalt[ENCRYPT2_SALTLEN];		// 平文	   16 RAND_bytes()
	unsigned char PassStr[ENCRYPT2_PWD_MAX_LEN];	// 暗号文 161 EVP_aes_256_ctr()
	unsigned char EncSalt[ENCRYPT2_SALTLEN];		// 暗号文  16 RAND_bytes()
	unsigned char EncHash[SHA512_DIGEST_LENGTH];	// 暗号文  64 HMAC512(Tag 〜 EncSalt)
} Encrypt2Profile, *Encrypt2ProfileP;

#define	ENCRYPT2_PROFILE_LEN		sizeof(Encrypt2Profile)

// misc
void	SetDlgPos(HWND hWnd, int pos);
void	EncodePassword(const char *cPassword, char *cEncodePassword);
BOOL	EnableItem(HWND hWnd, int idControl, BOOL flag);
BOOL	OpenFileDlg(HWND hWnd, UINT editCtl, const wchar_t *title, const wchar_t *filter, wchar_t *defaultDir, size_t max_path);
BOOL	SetForceForegroundWindow(HWND hWnd);
wchar_t* lwcsstri(wchar_t *s1, const wchar_t *s2);
void	UTIL_get_lang_msgW(const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const wchar_t *iniFile);
int		UTIL_get_lang_font(const char *key, HWND dlg, PLOGFONT logfont, HFONT *font, const char *iniFile);
LRESULT CALLBACK password_wnd_proc(HWND control, UINT msg,
                                   WPARAM wParam, LPARAM lParam);
int		Encrypt2EncDec(char *szPassword, const unsigned char *szEncryptKey, Encrypt2ProfileP profile, int encrypt);

#endif
