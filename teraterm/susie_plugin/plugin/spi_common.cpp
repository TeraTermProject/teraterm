/*
 * Copyright (C) 2020 TeraTerm Project
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

#include "spi_common.h"

void SPICreateBitmapInfoHeaderRGB(const unsigned char *image, int width, int height, HANDLE *phBmpInfo, HANDLE *phBmp)
{
	BITMAPINFOHEADER bmpi = {};
	bmpi.biSize = sizeof(BITMAPINFOHEADER);
	bmpi.biPlanes = 1;
	bmpi.biCompression = BI_RGB;
	bmpi.biWidth = width;
	bmpi.biHeight = height;
	bmpi.biBitCount = 24;
	int width4 = (width + 3) & (~3);
	bmpi.biSizeImage = width4 * height * 3;

	HANDLE hBmpInfo = LocalAlloc(LMEM_FIXED, sizeof(BITMAPINFOHEADER));
	memcpy((unsigned char *)hBmpInfo, &bmpi, sizeof(bmpi));
	HANDLE hBmp = LocalAlloc(LMEM_FIXED, bmpi.biSizeImage);
	unsigned char *bmp = (unsigned char *)hBmp;

	for (int y = 0; y < height; y++) {
		unsigned char *dest = bmp + (y * width4 * 3);
		const unsigned char *src = image + ((height - y - 1) * width * 3);
		for (int x = 0; x < width; x++) {
			dest[2] = src[0];	// R
			dest[1] = src[1];	// G
			dest[0] = src[2];	// B
			dest += 3;
			src += 3;
		}
	}

	*phBmpInfo = hBmpInfo;
	*phBmp = hBmp;
}
