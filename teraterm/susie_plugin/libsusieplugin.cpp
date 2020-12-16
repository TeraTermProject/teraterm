/*
 * Copyright (C) 2020- TeraTerm Project
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

#include "libsusieplugin.h"

#if !defined(WITHOUT_TERATERM)
#include "layer_for_unicode.h"
#define CreateFileW _CreateFileW
#define GetFileAttributesW _GetFileAttributesW
#define GetFullPathNameW _GetFullPathNameW
#define FindFirstFileW _FindFirstFileW
#define FindNextFileW _FindNextFileW
#define LoadLibraryW _LoadLibraryW
#endif

#if defined(_M_X64)
#define PLUGIN_EXT	L".sph"
#else
#define PLUGIN_EXT	L".spi"
#endif

/**
 * Susie プラグインを使って画像を読み込む
 *
 *	@param[in]	nameSPI		プラグインファイル名
 *	@param[in]	nameFile	画像ファイル名(画像の種類を判定する際に使われるかもしれない)
 *	@param[in]	bufFile		画像データへのポインタ
 *	@param[in]	sizeFile	画像データサイズ
 *	@param[out]	pHBInfo		BITMAPINFO			LocalFree()すること
 *	@param[out]	pHBm		bitmap data			LocalFree()すること
 *
 *	プラグインはUnicodeパス名に存在してもロードできる
 *	画像ファイル名はプラグイン内で多分使用されない
 *	プラグイン内でファイルは扱わないので Unicode化はokと思われる
 */
BOOL LoadPictureWithSPI(const wchar_t *nameSPI, const wchar_t *nameFile, unsigned char *bufFile, size_t sizeFile, HLOCAL *hbuf,
						HLOCAL *hbmi)
{
	// 画像ファイルのファイル名部分を取り出す
	const wchar_t *image_base = wcsrchr(nameFile, L'\\');
	if (image_base != NULL) {
		image_base++;
	}
	else {
		image_base = wcsrchr(nameFile, L'/');
		if (image_base != NULL) {
			image_base++;
		}
		else {
			image_base = nameFile;
		}
	}

	char nameFileA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, image_base, -1, nameFileA, _countof(nameFileA), NULL, NULL);

	HINSTANCE hSPI;
	char spiVersion[8];
	int(__stdcall * SPI_IsSupported)(LPCSTR, void *);
	int(__stdcall * SPI_GetPicture)(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC,
									LONG_PTR lData);
	int(__stdcall * SPI_GetPluginInfo)(int, LPSTR, int);
	int ret;

	ret = FALSE;
	hSPI = NULL;

	// SPI をロード
	hSPI = LoadLibraryW(nameSPI);
	if (!hSPI) {
		return FALSE;
	}

	FARPROC *p = (FARPROC *)&SPI_GetPluginInfo;
	*p = GetProcAddress(hSPI, "GetPluginInfo");
	p = (FARPROC *)&SPI_IsSupported;
	*p = GetProcAddress(hSPI, "IsSupported");
	p = (FARPROC *)&SPI_GetPicture;
	*p = GetProcAddress(hSPI, "GetPicture");

	if (!SPI_GetPluginInfo || !SPI_IsSupported || !SPI_GetPicture)
		goto error;

	//バージョンチェック
	SPI_GetPluginInfo(0, spiVersion, 8);

	if (spiVersion[2] != 'I' || spiVersion[3] != 'N')
		goto error;

	if (!(SPI_IsSupported)(nameFileA, bufFile))
		goto error;

	if ((SPI_GetPicture)((LPCSTR)bufFile, sizeFile, 1, hbmi, hbuf, NULL, 0))
		goto error;

	ret = TRUE;

error:

	if (hSPI)
		FreeLibrary(hSPI);

	return ret;
}

static unsigned char *LoadImageFile(const wchar_t *image_file, size_t *file_size)
{
	HANDLE hPictureFile = CreateFileW(image_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPictureFile == INVALID_HANDLE_VALUE) {
		*file_size = 0;
		return FALSE;
	}
	DWORD fileSize = GetFileSize(hPictureFile, 0);
	if (fileSize < 2 * 1024) {
		//最低 2kb は確保 (Susie plugin の仕様より)
		fileSize = 2 * 1024;
	}
	unsigned char *fileBuf = (unsigned char *)malloc(fileSize);
	memset(fileBuf, 0, 2*1024);	//頭の 2kb は 0 で初期化
	DWORD readByte;
	ReadFile(hPictureFile, fileBuf, fileSize, &readByte, 0);
	CloseHandle(hPictureFile);

	*file_size = fileSize;
	return fileBuf;
}

static wchar_t *NormalizePath(const wchar_t *path)
{
	size_t len = GetFullPathNameW(path, 0, NULL, NULL);		// include L'\0'
	if (len == 0) {
		return NULL;
	}
	wchar_t *normalized_path = (wchar_t *)malloc(sizeof(wchar_t) * len);
	len = GetFullPathNameW(path, (DWORD)len, normalized_path, NULL);
	if (len == 0) {
		free(normalized_path);
		return NULL;
	}
	wchar_t *p = wcsrchr(normalized_path, L'\\');
	if (p != NULL) {
		if (*(p + 1) == 0) {
			*p = 0;		// delete last '\\'
		}
	}
	DWORD attr = GetFileAttributesW(normalized_path);
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		free(normalized_path);
		return NULL;
	}
	return normalized_path;
}

/**
 *	Susieプラグインを使って画像ファイルをロードする
 *	指定フォルダ内のプラグインを使ってロードを試みる
 *
 *	@param[in]	image_file	画像ファイル
 *	@param[in]	spi_path	"c:\\path\\to\\spi"
 *							このフォルダにあるプラグインファイルで画像のロードを試みる
 *							パスの最後に "\\" があってもなくても良い
 *	@param[out]	pHBInfo		BITMAPINFO			LocalFree()すること
 *	@param[out]	pHBm		bitmap data			LocalFree()すること
 *	@retval		TRUE		ロードok
 *	@retval		FALSE		ロードできなかった
 */
BOOL SusieLoadPicture(const wchar_t *image_file, const wchar_t *spi_path, HANDLE *pHBInfo, HANDLE *pHBm)
{
	BOOL result = FALSE;
	*pHBInfo = NULL;
	*pHBm = NULL;

	size_t file_size;
	unsigned char *file_ptr = LoadImageFile(image_file, &file_size);
	if (file_ptr == NULL) {
		return FALSE;
	}

	// spi_path を絶対パスに変換
	wchar_t *spi_path_full = NormalizePath(spi_path);
	if (spi_path_full == NULL) {
		free(file_ptr);
		return FALSE;
	}
	const size_t spi_path_full_len = wcslen(spi_path_full);

	// mask作成
	const size_t spi_path_mask_len = spi_path_full_len + 4 + 1;
	wchar_t *spi_path_mask = (wchar_t *)malloc(spi_path_mask_len * sizeof(wchar_t));
	wcsncpy_s(spi_path_mask, spi_path_mask_len, spi_path_full, _TRUNCATE);
	wcsncat_s(spi_path_mask, spi_path_mask_len, L"\\*.*", _TRUNCATE);

	//プラグインを当たっていく
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(spi_path_mask, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		const size_t spiFileNameLen = spi_path_full_len + 1 + _countof(fd.cFileName);
		wchar_t *spiFileName = (wchar_t *)malloc(spiFileNameLen * sizeof(wchar_t));
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			const wchar_t *ext = wcsrchr(fd.cFileName, L'.');
			if (ext == NULL) {
				// 拡張子がないファイル?
				continue;
			}
			if (wcscmp(ext, L".dll") != 0 && wcscmp(ext, PLUGIN_EXT) != 0) {
				// .dll or .spi(or sph) 以外のファイル
				continue;
			}

			wcsncpy_s(spiFileName, spiFileNameLen, spi_path_full, _TRUNCATE);
			wcsncat_s(spiFileName, spiFileNameLen, L"\\", _TRUNCATE);
			wcsncat_s(spiFileName, spiFileNameLen, fd.cFileName, _TRUNCATE);

			HLOCAL hbuf, hbmi;
			if (LoadPictureWithSPI(spiFileName, image_file, file_ptr, file_size, &hbuf, &hbmi)) {
				*pHBInfo = hbmi;
				*pHBm = hbuf;
				result = TRUE;
				break;
			}
		} while (FindNextFileW(hFind, &fd));
		free(spiFileName);
		FindClose(hFind);
	}

	free(spi_path_full);
	free(spi_path_mask);
	free(file_ptr);
	return result;
}
