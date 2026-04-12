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
#include <ole2.h>
#include <olectl.h>
#include <wincodec.h>   // WIC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "../teraterm/teraterml.h"	// for ENABLE_GDIPLUS 要整理
#include "libsusieplugin.h"
#include "ttlib.h"
#include "compat_win.h"
#include "asprintf.h"
#if ENABLE_GDIPLUS
#include "ttgdiplus.h"
#endif

#include "bitmap.h"

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

/**
 *	GetFileSizeEx()はWindows 2000,XPごろから
 */
BOOL _GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
	lpFileSize->LowPart = GetFileSize(hFile, (DWORD *)&lpFileSize->HighPart);
	if (lpFileSize->LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		lpFileSize->LowPart = 0;
		lpFileSize->HighPart = 0;
		return FALSE;
	}
	return TRUE;
}

/**
 *	OleLoadPicture() を使った画像読み込み
 * 	jpeg, bmp を読み込むことができる
 *	(Windowsのバージョンによっては他の形式も読めるかもしれない)
 *
 */
static HBITMAP GetBitmapHandleW(const wchar_t *File)
{
	OLE_HANDLE hOle = 0;
	IStream *iStream=NULL;
	IPicture *iPicture=NULL;
	HGLOBAL hMem = NULL;
	LPVOID pvData = NULL;
	DWORD nReadByte=0,nFileSize;
	HANDLE hFile;
	short type;
	HBITMAP hBitmap = NULL;
	HRESULT result;
	BOOL r;

	hFile = CreateFileW(File, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	{
		LARGE_INTEGER fsize;
		if (_GetFileSizeEx(hFile, &fsize) == FALSE) {
			goto error;
		}
		if (fsize.u.HighPart != 0) {
			goto error;
		}
		nFileSize = (DWORD)fsize.QuadPart;
	}
	hMem = GlobalAlloc(GMEM_MOVEABLE, nFileSize);
	if (hMem == NULL) {
		goto error;
	}
	pvData = GlobalLock(hMem);
	if (pvData == NULL) {
		goto error;
	}

	r = ReadFile(hFile, pvData, nFileSize, &nReadByte, NULL);
	GlobalUnlock(hMem);
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	if (!r || nReadByte != nFileSize) {
		goto error;
	}

	CreateStreamOnHGlobal(hMem, TRUE, &iStream);

	result = OleLoadPicture(iStream, nFileSize, FALSE, IID_IPicture, (LPVOID *)&iPicture);
	if (result != S_OK || iPicture == NULL) {
		// 画像ファイルではない,対応した画像ファイル場合
		if (iStream != NULL) {
			iStream->Release();
		}
		return NULL;
	}

	iStream->Release();

	iPicture->get_Type(&type);
	if(type==PICTYPE_BITMAP){
		iPicture->get_Handle(&hOle);
	}

	hBitmap=(HBITMAP)(UINT_PTR)hOle;

	iPicture->Release();

	return hBitmap;

error:
	if (hMem != NULL) {
		GlobalFree(hMem);
	}
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	return NULL;
}

static HBITMAP CreateBitmapFromBITMAPINFO(const BITMAPINFO *pbmi, const unsigned char *pbuf)
{
	void* pvBits;
	HBITMAP hBmp = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

	if (hBmp != NULL && pbuf != NULL) {
		memcpy(pvBits, pbuf, pbmi->bmiHeader.biSizeImage);
	}

	return hBmp;
}

static HBITMAP CreateHBitmapFromBitmapSource(IWICBitmapSource *pImage)
{
	HRESULT hr;
	WICPixelFormatGUID format;
	BITMAPINFO bminfo = {};
	UINT width = 0;
	UINT height = 0;
	HBITMAP hBmp = NULL;
	LPVOID imageBits = NULL;

	// 32bppBGRAか確認
	hr = pImage->GetPixelFormat(&format);
	if (SUCCEEDED(hr) && format == GUID_WICPixelFormat32bppBGRA) {
		hr = S_OK;
	}
	else {
		hr = E_FAIL;
	}

	// ビットマップサイズを取得
	if (SUCCEEDED(hr)) {
		hr = pImage->GetSize(&width, &height);
	}

	// DIBを生成
	if (SUCCEEDED(hr)) {
		bminfo.bmiHeader.biSize = sizeof(BITMAPINFO);
		bminfo.bmiHeader.biBitCount = 32;
		bminfo.bmiHeader.biCompression = BI_RGB;
		bminfo.bmiHeader.biWidth = width;
		bminfo.bmiHeader.biHeight = -static_cast<LONG>(height);
		bminfo.bmiHeader.biPlanes = 1;
		hBmp = CreateDIBSection(NULL, &bminfo, DIB_RGB_COLORS, &imageBits, NULL, 0);
	}

	// イメージをコピー
	if (SUCCEEDED(hr)) {
		hr = pImage->CopyPixels(NULL, width * 4, width * height * 4, reinterpret_cast<BYTE *>(imageBits));
	}
	else if (hBmp != NULL) {
		DeleteObject(hBmp);
		hBmp = NULL;
	}

	return hBmp;
}

static HBITMAP LoadByWICFromImage(const uint8_t *image, size_t imageSize)
{
	HRESULT hr;
	HBITMAP hBmp = NULL;
	IWICImagingFactory *pFactory = NULL;
	IWICBitmapDecoder *pDecoder = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapFrameDecode *pFrame = NULL;
	IWICBitmapSource *pImage = NULL;
	IWICStream *pStream = NULL;

	// WIC初期化
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
	if (FAILED(hr)) {
		return NULL;
	}

	// ストリームを生成
	hr = pFactory->CreateStream(&pStream);

	if (SUCCEEDED(hr)) {
		hr = pStream->InitializeFromMemory((BYTE*)image, (DWORD)imageSize);
		// WICInProcPointer だとエラーが出る環境があるので、BYTE* とする
	}

	// デコーダを生成
	if (SUCCEEDED(hr)) {
		hr = pFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder);
	}

	// 最初のフレームを取得
	if (SUCCEEDED(hr)) {
		hr = pDecoder->GetFrame(0, &pFrame);
	}

	// コンバータ生成 (32bppBGRAビットマップ)
	if (SUCCEEDED(hr)) {
		hr = pFactory->CreateFormatConverter(&pConverter);
	}
	if (SUCCEEDED(hr)) {
		hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
	}

	// イメージ取得
	if (SUCCEEDED(hr)) {
		hr = pConverter->QueryInterface(IID_PPV_ARGS(&pImage));
	}

	// イメージからビットマップに変換
	if (SUCCEEDED(hr)) {
		hBmp = CreateHBitmapFromBitmapSource(pImage);
	}

	// 解放
	if (pImage) pImage->Release();
	if (pConverter) pConverter->Release();
	if (pFrame) pFrame->Release();
	if (pDecoder) pDecoder->Release();
	if (pStream) pStream->Release();
	if (pFactory) pFactory->Release();

	return hBmp;
}

static BOOL LoadFile(const wchar_t *filename, uint8_t **file_ptr, size_t *file_size)
{
	uint8_t* data_ptr;
	uint64_t data_size;

	// ファイルを開く
	HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		// open error
	error:
		*file_ptr = NULL;
		*file_size = 0;
		return FALSE;
	}

	{
		LARGE_INTEGER fsize;
		if (_GetFileSizeEx(hFile, &fsize) == FALSE) {
			CloseHandle(hFile);
			goto error;
		}
		if (fsize.u.HighPart != 0) {
			CloseHandle(hFile);
			goto error;
		}
		data_size = fsize.QuadPart;
	}

	data_ptr = (unsigned char *)malloc(data_size);
	if (data_ptr == NULL) {
		// memory error
		CloseHandle(hFile);
		goto error;
	}
	DWORD readBytes;
	BOOL r = ReadFile(hFile, data_ptr, (DWORD)data_size, &readBytes, NULL);
	CloseHandle(hFile);
	if (!r || readBytes != (DWORD)data_size) {
		// read error
		free(data_ptr);
		goto error;
	}

	*file_ptr = data_ptr;
	*file_size = (size_t)data_size;

	return TRUE;
}

/**
 *	WICをつかって
 *	ファイルからビットマップを読み込み
 *
 *	@param filename	ファイル名
 *	@return 読み込んだビットマップのハンドルを返す
 *		    NULLのとき読めなかった
 */
static HBITMAP LoadImageWIC(const wchar_t *filename)
{
	// ファイルを読み込む
	uint8_t* file_ptr;
	size_t file_size;
	BOOL r = LoadFile(filename, &file_ptr, &file_size);
	if (r == FALSE) {
		// error
		return NULL;
	}

	// ビットマップハンドルを取得
	HBITMAP hBmp = LoadByWICFromImage(file_ptr, file_size);

	free(file_ptr);

	return hBmp;
}

/**
 *	ファイルからビットマップを読み込み
 *
 * @param filename ファイル名
 * @param hBitmap 読み込んだビットマップのハンドルを返す
 * @param param パラメータを指定する
 *				NULLのとき適切にファイルを読む
 */
DWORD BitmapLoad(const wchar_t *filename, const BitmapLoadParam_t *param, HBITMAP *hBitmap)
{
	HBITMAP hbm = NULL;
	const wchar_t *spi_path = param == NULL ? NULL : param->SusiePluginPath;
	*hBitmap = NULL;

	// Susie plugin で読み込み
	if (hbm == NULL && spi_path != NULL) {
		HANDLE hbmi;
		HANDLE hbuf;
		BOOL r = SusieLoadPicture(filename, spi_path, &hbmi, &hbuf);
		if (r != FALSE) {
			const BITMAPINFO *pbmi = (BITMAPINFO *)LocalLock(hbmi);
			const unsigned char *pbuf = (unsigned char *)LocalLock(hbuf);
			hbm = CreateBitmapFromBITMAPINFO(pbmi, pbuf);
			LocalUnlock(hbmi);
			LocalFree(hbmi);
			LocalUnlock(hbuf);
			LocalFree(hbuf);
		}
	}

	// WIC を使って読み込む
	if (hbm == NULL) {
		hbm = LoadImageWIC(filename);
	}

	// GDI+ ライブラリを使って読み込む
#if ENABLE_GDIPLUS
	if (hbm == NULL) {
		hbm = GDIPLoad(filename);
	}
#endif

	// OLE を利用して画像(jpeg)を読む
	//		LoadImage()のみ許可されている環境ではないとき
	if (hbm == NULL) {
		hbm = GetBitmapHandleW(filename);
	}

	// LoadImageW() API で読み込む
	if (hbm == NULL) {
		// LoadImageW() APIは、
		// Windows 10 のとき高さがマイナスのbmpファイルはロードに失敗する
		// Windows 7 のときは成功する
		// IMAGE_BITMAPのとき戻り値はHBITMAP
		// LR_CREATEDIBSECTION を指定すると、DIBを返す
		hbm = (HBITMAP)LoadImageW(0, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
	}

	*hBitmap = hbm;
	return NO_ERROR;
}
