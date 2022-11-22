/*
 * (C) 2022- TeraTerm Project
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

// TODO:
//	- GDI+ Flat API へ置きかえて DLL がない場合を考慮
//	  - GDI+ は Windows XPから利用ok

#include <windows.h>
#include <gdiplus.h>
#include <assert.h>

#include "ttgdiplus.h"

#define DEBUG_CHECK_IMAGE 0

static ULONG_PTR gdiplusToken;

void GDIPInit(void)
{
	assert(gdiplusToken == 0);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status r = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	assert(r == Gdiplus::Ok);
	(void)r;
}

void GDIPUninit(void)
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
	gdiplusToken = 0;
}

/**
 *	Gdiplus::Bitmap の画像保存
 *
 *	使い方
 *	Gdiplus::Bitmap bitmap;
 *	CLSID encoderClsid;
 *	GetEncoderClsid(L"image/png", &encoderClsid);
 *	bitmap.Save(L"Bird.png", &encoderClsid, NULL);
 */
#if DEBUG_CHECK_IMAGE
static int GetEncoderClsid(const wchar_t *mime_ext, CLSID *pClsid)
{
	UINT num = 0;	// number of image encoders
	UINT size = 0;	// size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;	// Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;	// Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j) {
		if ((wcsstr(pImageCodecInfo[j].MimeType, mime_ext) == NULL) ||
			(wcsstr(pImageCodecInfo[j].FilenameExtension, mime_ext) == NULL) ||
			(wcsstr(pImageCodecInfo[j].FormatDescription, mime_ext) == NULL)) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;	// Failure
}
#endif

#if DEBUG_CHECK_IMAGE
static void GDIPSavePNG(Gdiplus::Bitmap *bitmap, const wchar_t *filename)
{
	CLSID   encoderClsid;
	GetEncoderClsid(L"image/png", &encoderClsid);
	bitmap->Save(filename, &encoderClsid, NULL);
}
#endif

static HBITMAP bitmap_GetHBITMAP(Gdiplus::Bitmap *bitmap)
{
	const int width = bitmap->GetWidth();
	const int height = bitmap->GetHeight();

	Gdiplus::Rect rect(0, 0, width, height);
	Gdiplus::BitmapData src_info;
	Gdiplus::Status r;
	r = bitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &src_info);
	if(r != Gdiplus::Ok) {
		return NULL;
	}

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(bmi);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;	// = 8*4
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = width * height * 4;

	// usage == DIB_PAL_COLORS のとき hdcのパレットが使用される
	// DIB_RGB_COLORS のときは hdc は参照されない?
	// MSDNにはNULLでもokとは書かれていない
	void* pvBits;
	HBITMAP hBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	if (hBmp == NULL) {
		return NULL;
	}

	memcpy(pvBits, src_info.Scan0, width * height * 4);

	return hBmp;
}

/**
 *	画像ファイルを読み込んで HBITMAP を返す
 *
 *	@param filename	ファイル名
 *	@retval			HBITMAP
 *					不要になったら DeleteObject() すること
 *	@retval			NULL ファイルが読み込めなかった
 */
HBITMAP GDIPLoad(const wchar_t *filename)
{
	Gdiplus::Bitmap bitmap(filename);
	if (bitmap.GetLastStatus() != Gdiplus::Ok) {
		return NULL;
	}

#if DEBUG_CHECK_IMAGE
	GDIPSavePNG(&bitmap, L"test.png");
#endif

	Gdiplus::PixelFormat format = bitmap.GetPixelFormat();

	if (format != PixelFormat32bppARGB) {
		HBITMAP hBmp;
		Gdiplus::Status r;
		Gdiplus::Color bgColor = Gdiplus::Color(0, 0, 0);
		r = bitmap.GetHBITMAP(bgColor, &hBmp);
		if (r != Gdiplus::Ok) {
			return NULL;
		}
		return hBmp;
	}
	else {
		// GetHBITMAP() returns no alpha info
		return bitmap_GetHBITMAP(&bitmap);
	}
}
