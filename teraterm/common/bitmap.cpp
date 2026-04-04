/*
 * Copyright (C) 2026- TeraTerm Project
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

/* save bmp file for debug */

#include <windows.h>
#include <assert.h>

#include "bitmap.h"
#include "ttlib.h"
#include "compat_win.h"
#include "asprintf.h"

/**
 *	デバグ用ファイル名、desktopに保存する
 */
static wchar_t *GetFilename(const wchar_t* fname)
{
	if (IsRelativePathW(fname)) {
		wchar_t *desktop;
		_SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_CREATE, NULL, &desktop);
		wchar_t *new_fname = NULL;
		awcscats(&new_fname, desktop, L"\\", fname, NULL);
		free(desktop);
		return new_fname;
	}
	else {
		return _wcsdup(fname);
	}
}

/**
 *	bmpファイル保存
 *	BITMAPINFO から
 */
BOOL SaveBmpFromBmi(const BITMAPINFO *pbmi, const unsigned char *pbuf, const wchar_t *filename)
{
	int bmiSize;
	DWORD writtenByte;
	HANDLE hFile;
	BITMAPFILEHEADER bfh;

	wchar_t *filename_new = GetFilename(filename);
	hFile = CreateFileW(filename_new, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	free(filename_new);

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	bmiSize = pbmi->bmiHeader.biSize;

	switch (pbmi->bmiHeader.biBitCount) {
		case 1:
			bmiSize += pbmi->bmiHeader.biClrUsed ? sizeof(RGBQUAD) * 2 : 0;
			break;

		case 2:
			bmiSize += sizeof(RGBQUAD) * 4;
			break;

		case 4:
			bmiSize += sizeof(RGBQUAD) * 16;
			break;

		case 8:
			bmiSize += sizeof(RGBQUAD) * 256;
			break;
	}

	ZeroMemory(&bfh, sizeof(bfh));
	bfh.bfType = MAKEWORD('B', 'M');
	bfh.bfOffBits = sizeof(bfh) + bmiSize;
	bfh.bfSize = bfh.bfOffBits + pbmi->bmiHeader.biSizeImage;

	WriteFile(hFile, &bfh, sizeof(bfh), &writtenByte, 0);
	WriteFile(hFile, pbmi, bmiSize, &writtenByte, 0);
	WriteFile(hFile, pbuf, pbmi->bmiHeader.biSizeImage, &writtenByte, 0);

	CloseHandle(hFile);

	return TRUE;
}

/**
 *	bmpファイル保存
 *	HBITMAPから
 */
BOOL SaveBmpFromHBitmap(HBITMAP hBitmap, const wchar_t *filename)
{
	BITMAP bm;
	if (!GetObject(hBitmap, sizeof(BITMAP), &bm))
		return FALSE;

	// BITMAPINFOHEADER
	BITMAPINFOHEADER bi = {};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bm.bmBitsPixel;
	bi.biCompression = BI_RGB;

	// size(4byte align)
	DWORD stride = ((bm.bmWidth * bi.biBitCount + 31) / 32) * 4;
	DWORD image_size = stride * bm.bmHeight;

	// ピクセルデータ
	BYTE *pixels = (BYTE *)malloc(image_size);
	if (pixels == NULL)
		return FALSE;

	HDC hdc = GetDC(NULL);
	BITMAPINFO bmi = {};
	bmi.bmiHeader = bi;
	int ret = GetDIBits(hdc, hBitmap, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);

	if (!ret) {
		free(pixels);
		return FALSE;
	}

	BOOL r = SaveBmpFromBmi(&bmi, pixels, filename);
	free(pixels);
	return r;
}

/**
 *	bmpファイル保存
 *	HDC(メモリDC)から
 */
BOOL SaveBmpFromHDC(HDC hdc, const wchar_t *filename)
{
	HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
	if (hBitmap == NULL) {
		// 画面やプリンタの場合
		assert(FALSE);
		return FALSE;
	}
	return SaveBmpFromHBitmap(hBitmap, filename);
}
