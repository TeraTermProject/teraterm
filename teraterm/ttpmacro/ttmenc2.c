/*
 * (C) 2024- TeraTerm Project
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

/* TTMACRO.EXE, password encryption 2 */

#include "teraterm.h"
#include <stdlib.h>
#include <string.h>
#include <fileapi.h>

#define LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>
#include "ttmdef.h"
#include "ttmparse.h"

#define ENCRYPT2_SALTLEN		16
#define ENCRYPT2_CIPHER			EVP_aes_256_ctr()
#define ENCRYPT2_DIGEST			EVP_sha512()
#define ENCRYPT2_ITER1			1001	// パスワード以外
#define ENCRYPT2_ITER2			210001	// パスワード用 (参考 https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2)
#define ENCRYPT2_IKLEN			32
#define ENCRYPT2_IVLEN			16
#define ENCRYPT2_TAG			"\000\002"
#define ENCRYPT2_PWD_MAX_LEN	203
#define ENCRYPT2_ENCRYPT		TRUE
#define ENCRYPT2_DECRYPT		FALSE

// Encrypt2 パスワードプロファイル
typedef struct {									//	   計 381バイト(base64エンコード後は508バイト + \r\n)
	unsigned char Tag[2];							// 平文		2 Encrypt2識別タグ (ENCRYPT2_TAG)
	unsigned char KeySalt[ENCRYPT2_SALTLEN];		// 平文	   16 RAND_bytes()
	unsigned char KeyHash[SHA512_DIGEST_LENGTH];	// 平文	   64 PBKDF2(KeyStr)
	unsigned char PassSalt[ENCRYPT2_SALTLEN];		// 平文	   16 RAND_bytes()
	unsigned char PassStr[ENCRYPT2_PWD_MAX_LEN];	// 暗号文 203 EVP_aes_256_ctr()
	unsigned char EncSalt[ENCRYPT2_SALTLEN];		// 暗号文  16 RAND_bytes()
	unsigned char EncHash[SHA512_DIGEST_LENGTH];	// 暗号文  64 HMAC512(Tag 〜 EncSalt)
} Encrypt2Profile, *Encrypt2ProfileP;

#define ENCRYPT2_PROFILE_LEN	sizeof(Encrypt2Profile)				// 381 バイト
#define ENCRYPT2_BASE64_LEN		sizeof(Encrypt2Profile) / 3 * 4		// 508 バイト
#define ENCRYPT2_MaxLineLen		512

// パスワードファイル 1行読み出し
// 復帰値 -1:エラー、n:読み込みバイト数
static int PwdFileReadln(HANDLE FH, unsigned char *lpBuffer, UINT uBytes)
{
	DWORD Count;
	UINT Rnum;
	BOOL Result, EndFile, EndLine;
	unsigned char Byte[1];

	Rnum = 0;
	EndLine = FALSE;
	EndFile = TRUE;
	do {
		Result = ReadFile(FH, Byte, 1, &Count, NULL);
		if (Result == FALSE) {
			break;
		}
		if (Count > 0) {
			EndFile = FALSE;
		}
		if (Count == 1) {
			switch (Byte[0]) {
			case 0x0d:
				Result = ReadFile(FH, Byte, 1, &Count, NULL);
				if (Result == FALSE) {
					break;
				}
				if ((Count == 1) && (Byte[0] != 0x0a)) {
					SetFilePointer(FH, -1, NULL, FILE_CURRENT);
				}
				EndLine = TRUE;
				break;
			case 0x0a:
				EndLine = TRUE;
				break;
			default:
				if (Rnum < uBytes - 1) {
					lpBuffer[Rnum] = Byte[0];
					Rnum++;
				}
			}
		}
	} while ((Count >= 1) && (! EndLine));
	lpBuffer[Rnum] = 0;

	if (EndFile) {
		return -1;
	}
	return Rnum;
}

// パスワードファイル 行追加
static int Encrypt2ProfileAdd(HANDLE FH, Encrypt2ProfileP Profile)
{
	DWORD NumberOfBytesWritten;
	char ProfileB64[ENCRYPT2_MaxLineLen];
	BOOL Result;

	if (SetFilePointer(FH, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
		return 0;
	}
	b64encode(ProfileB64, ENCRYPT2_BASE64_LEN + 1, (PCHAR)Profile, ENCRYPT2_PROFILE_LEN);
	Result = WriteFile(FH, ProfileB64, ENCRYPT2_BASE64_LEN, &NumberOfBytesWritten, NULL);
	if (Result == FALSE || ENCRYPT2_BASE64_LEN != NumberOfBytesWritten) {
		return 0;
	}
	Result = WriteFile(FH, "\015\012", 2, &NumberOfBytesWritten, NULL);
	if (Result == FALSE || NumberOfBytesWritten != 2) {
		return 0;
	}
	return 1;
}

// パスワードファイルからKeyStr(パスワード識別子)にマッチする行を検索する
// 復帰値  0:マッチする行無し、1:マッチする行有り
// Profileにマッチした行のEncrypt2Profileが設定される
#if defined(_MSC_VER)
#pragma optimize("", off)
#elif defined(__clang__)
#pragma clang optimize off
#else
#pragma GCC push_options
#pragma GCC optimize ("O0")
#endif
static int Encrypt2ProfileSearch (HANDLE FH, const char *KeyStr, Encrypt2ProfileP Profile)
{
	char ProfileB64[ENCRYPT2_MaxLineLen];
	int ProfileB64Len;
	unsigned char KeyHash[SHA512_DIGEST_LENGTH];
	Encrypt2Profile Lprofile, DummyProfile;
	int Ret, Dummy;
	DWORD Cpos, Dpos;

	if (SetFilePointer(FH, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		return 0;
	}

	Ret = 0;
	while (1) {
		if ((ProfileB64Len = PwdFileReadln(FH, ProfileB64, ENCRYPT2_MaxLineLen)) <= 0) {
			break;
		}
		if (ProfileB64Len != ENCRYPT2_BASE64_LEN ||
			b64decode((PCHAR)&Lprofile, ENCRYPT2_PROFILE_LEN + 1, ProfileB64) != ENCRYPT2_PROFILE_LEN ||
			CRYPTO_memcmp(Lprofile.Tag, ENCRYPT2_TAG, sizeof(ENCRYPT2_TAG) - 1) != 0) {
			continue;
		}
		// KeyStrのhashが一致するか確認
		if (PKCS5_PBKDF2_HMAC(KeyStr, strlen(KeyStr),
							  (const unsigned char *)&(Lprofile.KeySalt), ENCRYPT2_SALTLEN,
							  ENCRYPT2_ITER1, (EVP_MD *)ENCRYPT2_DIGEST,
							  SHA512_DIGEST_LENGTH, KeyHash) != 1) {
			continue;
		}

		if (CRYPTO_memcmp(KeyHash, Lprofile.KeyHash, SHA512_DIGEST_LENGTH) == 0) {
			memcpy(Profile, &Lprofile, ENCRYPT2_PROFILE_LEN);
			if ((Cpos = SetFilePointer(FH, 0, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER) {
				break;
			}
			Ret = 1;
			// マッチする行が見つかってもbreakしない(サイドチャネル攻撃対策)
		} else {
			memcpy(&DummyProfile, &Lprofile, ENCRYPT2_PROFILE_LEN);
			if ((Dpos = SetFilePointer(FH, 0, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER) {
				break;
			}
			Dummy = 1;
			(void)Dummy;
		}
	}

	if (Ret == 0) {
		if (SetFilePointer(FH, Dpos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
			Ret = 0;
		}
	} else {
		if (SetFilePointer(FH, Cpos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
			Ret = 0;
		}
	}

	return Ret;
}
#if defined(_MSC_VER)
#pragma optimize("", on)
#elif defined(__clang__)
#pragma clang optimize on
#else
#pragma GCC pop_options
#endif

// パスワードファイル 行更新
static int Encrypt2ProfileUpdate(HANDLE FH, const char *KeyStr, Encrypt2ProfileP Profile)
{
	Encrypt2Profile OldProfile;
	DWORD NumberOfBytesWritten;
	char ProfileB64[ENCRYPT2_MaxLineLen];
	int Result;

	if (Encrypt2ProfileSearch(FH, KeyStr, &OldProfile) == 0 ||
		SetFilePointer(FH, (LONG)(ENCRYPT2_BASE64_LEN) * -1 - 2, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
		return 0;
	}
	b64encode(ProfileB64, ENCRYPT2_BASE64_LEN + 1, (PCHAR)Profile, ENCRYPT2_PROFILE_LEN);
	Result = WriteFile(FH, ProfileB64, ENCRYPT2_BASE64_LEN, &NumberOfBytesWritten, NULL);
	if (Result == FALSE || ENCRYPT2_BASE64_LEN != NumberOfBytesWritten) {
		return 0;
	}
	Result = WriteFile(FH, "\015\012", 2, &NumberOfBytesWritten, NULL);
	if (Result == FALSE || NumberOfBytesWritten != 2) {
		return 0;
	}
	return 1;
}

// パスワードファイル 行削除
static int Encrypt2ProfileDelete(HANDLE FH, const char *KeyStr)
{
	Encrypt2Profile OldProfile;
	DWORD Cpos, Epos;
	unsigned char *p;
	DWORD NumberOfBytesRead, NumberOfBytesWritten;

	if (Encrypt2ProfileSearch(FH, KeyStr, &OldProfile) == 0) {
		return 0;
	}
	if ((Cpos = SetFilePointer(FH, 0, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER ||
		(Epos = SetFilePointer(FH, 0, NULL, FILE_END)) == INVALID_SET_FILE_POINTER ||
		(p = (unsigned char *)malloc(Epos - Cpos)) == NULL) {
		return 0;
	}
	if (SetFilePointer(FH, Cpos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
		ReadFile(FH, p, Epos - Cpos, &NumberOfBytesRead, NULL) == FALSE ||
		NumberOfBytesRead != Epos - Cpos ||
		SetFilePointer(FH, Cpos - ENCRYPT2_BASE64_LEN - 2, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
		WriteFile(FH, p, NumberOfBytesRead, &NumberOfBytesWritten, NULL) == FALSE ||
		NumberOfBytesRead != NumberOfBytesWritten ||
		SetEndOfFile(FH) == FALSE) {
		free(p);
		return 0;
	}
	free(p);
	return 1;
}

// Encrypt2 暗号化/復号処理
static int Encrypt2EncDec(char *PassStr, const char *EncryptStr, Encrypt2ProfileP profile, int encrypt)
{
	unsigned char TmpKeyIV[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH];
	unsigned char Key[EVP_MAX_KEY_LENGTH], IV[EVP_MAX_IV_LENGTH];
	BIO *Bmem = NULL, *Benc = NULL, *Bio = NULL;
	EVP_CIPHER_CTX *ctx;
	Encrypt2Profile Lprofile;
	unsigned char Buf[ENCRYPT2_PROFILE_LEN - SHA512_DIGEST_LENGTH];
	unsigned char Hash[SHA512_DIGEST_LENGTH];
	int HashLen;
	int ret = 0;

	if (encrypt) {
		if (RAND_bytes(profile->PassSalt, ENCRYPT2_SALTLEN) <= 0 ||
			RAND_bytes(profile->EncSalt, ENCRYPT2_SALTLEN) <= 0) {
			goto end;
		}
	}

	// 鍵導出
	if (PKCS5_PBKDF2_HMAC(EncryptStr, strlen(EncryptStr),
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
		len = strlen(PassStr);
		memcpy(Buf, PassStr, len);
		memset(Buf + len, 0x00, ENCRYPT2_PWD_MAX_LEN - len);	// nullパディング
		if (BIO_write(Bio, Buf, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_write(Bio, profile->EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN ||
			BIO_flush(Bio) != 1 ||
			BIO_read(Bmem, profile->PassStr, ENCRYPT2_PWD_MAX_LEN) != ENCRYPT2_PWD_MAX_LEN ||
			BIO_read(Bmem, profile->EncSalt, ENCRYPT2_SALTLEN) != ENCRYPT2_SALTLEN) {
			goto end;
		}
		// hmac格納
		if ((PKCS5_PBKDF2_HMAC(EncryptStr, strlen(EncryptStr),
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
		if ((PKCS5_PBKDF2_HMAC(EncryptStr, strlen(EncryptStr),
							   (const unsigned char *)&(profile->EncSalt), ENCRYPT2_SALTLEN,
							   ENCRYPT2_ITER1, (EVP_MD *)ENCRYPT2_DIGEST,
							   SHA512_DIGEST_LENGTH, Key) != 1) ||
			HMAC(ENCRYPT2_DIGEST, Key, SHA512_DIGEST_LENGTH,
				 (const unsigned char *)profile, ENCRYPT2_PROFILE_LEN - SHA512_DIGEST_LENGTH,
				 Hash, &HashLen) == NULL ) {
			goto end;
		}
		memcpy(PassStr, Lprofile.PassStr, ENCRYPT2_PWD_MAX_LEN);
		if (CRYPTO_memcmp(Hash, Lprofile.EncHash, SHA512_DIGEST_LENGTH) == 0) {
			PassStr[ENCRYPT2_PWD_MAX_LEN] = 0;
			ret = 1;	// 一致
		} else {
			PassStr[0] = 0;
			ret = 0;	// 不一致
		}
	}

 end:
	BIO_free(Benc);
	BIO_free(Bmem);
	return ret;
}

int Encrypt2SetPassword(LPCWSTR FileNameStr, const char *KeyStr, const char *PassStr, const char *EncryptStr)
{
	HANDLE FH;
	Encrypt2Profile Profile, OldProfile;
	BOOL update;

	if (strlen(PassStr) > ENCRYPT2_PWD_MAX_LEN) {
		return 0;
	}

	// ファイルオープン、存在しない場合は新規作成
	if ((FH = CreateFileW(FileNameStr, GENERIC_READ|GENERIC_WRITE, 0, NULL,
						  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (Encrypt2ProfileSearch(FH, KeyStr, &OldProfile) == 1) {
		if (Encrypt2EncDec(OldProfile.PassStr, EncryptStr, &OldProfile, ENCRYPT2_DECRYPT) == 1 &&
			CRYPTO_memcmp(OldProfile.PassStr, PassStr, ENCRYPT2_PWD_MAX_LEN) == 0) {
			CloseHandle(FH);
			return 1;	// パスワード変更無し
		}
		update = TRUE;	// 更新
	} else {
		update = FALSE;	// 追加
	}

	memcpy(Profile.Tag, ENCRYPT2_TAG, sizeof(ENCRYPT2_TAG) - 1);				// Tag
	if (RAND_bytes(Profile.KeySalt, ENCRYPT2_SALTLEN) <= 0) {					// KeySalt
		CloseHandle(FH);
		return 0;
	}
	if (PKCS5_PBKDF2_HMAC(KeyStr, strlen(KeyStr),
						  (const unsigned char *)&(Profile.KeySalt), ENCRYPT2_SALTLEN,
						  ENCRYPT2_ITER1, (EVP_MD *)ENCRYPT2_DIGEST,
						  SHA512_DIGEST_LENGTH, Profile.KeyHash) != 1) {		// KeyHash
		return 0;
	}
	if (Encrypt2EncDec((char *)PassStr, EncryptStr, &Profile, ENCRYPT2_ENCRYPT) == 0) {	// PassSalt, EncSalt, EncHash
		CloseHandle(FH);
		return 0;
	}

	if (update) {
		if (Encrypt2ProfileUpdate(FH, KeyStr, &Profile) == 0) {
			CloseHandle(FH);
			return 0;
		}
	} else {
		if (Encrypt2ProfileAdd(FH, &Profile) == 0) {
			CloseHandle(FH);
			return 0;
		}
	}

	CloseHandle(FH);
	return 1;
}

int Encrypt2GetPassword(LPCWSTR FileNameStr, const char *KeyStr, char *PassStr, const char *EncryptStr)
{
	HANDLE FH;
	Encrypt2Profile Profile;
	int Ret;

	if ((FH = CreateFileW(FileNameStr, GENERIC_READ, FILE_SHARE_READ, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if ((Ret = Encrypt2ProfileSearch(FH, KeyStr, &Profile)) == 0) {
		CloseHandle(FH);
		return 0;
	}
	Ret = Encrypt2EncDec(PassStr, EncryptStr, &Profile, ENCRYPT2_DECRYPT);

	CloseHandle(FH);
	return Ret;
}

int Encrypt2IsPassword(LPCWSTR FileNameStr, const char *KeyStr)
{
	HANDLE FH;
	Encrypt2Profile Profile;
	int Ret;

	if ((FH = CreateFileW(FileNameStr, GENERIC_READ, FILE_SHARE_READ, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return 0;
	}
	Ret = Encrypt2ProfileSearch(FH, KeyStr, &Profile);

	CloseHandle(FH);
	return Ret;
}

int Encrypt2DelPassword(LPCWSTR FileNameStr, const char *KeyStr)
{
	HANDLE FH;
	Encrypt2Profile Profile;

	if ((FH = CreateFileW(FileNameStr, GENERIC_READ|GENERIC_WRITE, 0, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==	 INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (KeyStr[0] != 0) {
		// 指定されたKeyStr(パスワード識別子)のプロファイルを削除
		if (Encrypt2ProfileSearch(FH, KeyStr, &Profile) == 0) {
			CloseHandle(FH);
			return 0;
		}
		if (Encrypt2ProfileDelete(FH, KeyStr) == 0) {
			CloseHandle(FH);
			return 0;
		}
		CloseHandle(FH);
		return 1;
	}

	// Encrypt2の全プロファイルを削除
	DWORD cpos, epos;
	unsigned char *p, *cp;
	char ProfileB64[ENCRYPT2_MaxLineLen];
	int ProfileB64Len, ProfileLen;
	DWORD NumberOfBytesWritten;

	if ((cpos = SetFilePointer(FH, 0, NULL, FILE_BEGIN)) == INVALID_SET_FILE_POINTER ||
		(epos = SetFilePointer(FH, 0, NULL, FILE_END)) == INVALID_SET_FILE_POINTER ||
		(cpos = SetFilePointer(FH, 0, NULL, FILE_BEGIN)) == INVALID_SET_FILE_POINTER ||
		(p = (unsigned char *)malloc(epos - cpos)) == NULL) {
		CloseHandle(FH);
		return 0;
	}

	cp = p;
	while (1) {
		if ((ProfileB64Len = PwdFileReadln(FH, ProfileB64, ENCRYPT2_MaxLineLen)) <= 0) {
			break;
		}
		// Encrypt2ではないプロファイルは消さない
		ProfileLen = b64decode((PCHAR)&Profile, ENCRYPT2_PROFILE_LEN + 1, ProfileB64);
		if (ProfileB64Len != ENCRYPT2_BASE64_LEN ||
			ProfileLen != ENCRYPT2_PROFILE_LEN ||
			CRYPTO_memcmp(Profile.Tag, ENCRYPT2_TAG, sizeof(ENCRYPT2_TAG) - 1) != 0) {
			memcpy(cp, ProfileB64, ProfileB64Len);
			cp += ProfileB64Len;
			memcpy(cp, "\015\012", 2);
			cp += 2;
		}
	}
	if (SetFilePointer(FH, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
		WriteFile(FH, p, cp - p, &NumberOfBytesWritten, NULL) == FALSE ||
		cp - p != NumberOfBytesWritten ||
		SetEndOfFile(FH) == FALSE) {
		free(p);
		CloseHandle(FH);
		return 0;
	}

	free(p);
	CloseHandle(FH);
	return 1;
}
