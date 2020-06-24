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

#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "png.h"
#include "spi_common.h"

#include "susie_plugin.h"

typedef struct {
	const unsigned char *top;
	size_t len;
	size_t pos;
} data_on_memory_t;

static void PNGCBAPI read_from_memory(png_structp png_ptr, png_bytep data, size_t length)
{
	data_on_memory_t *data_on_memory = (data_on_memory_t *)png_get_io_ptr(png_ptr);
	if (data_on_memory->pos + length > data_on_memory->len) {
		png_error(png_ptr, "read error");
		return;
	}
	memcpy(data, data_on_memory->top + data_on_memory->pos, length);
	data_on_memory->pos += length;
}

static int CheckHeader(const unsigned char *data, size_t len)
{
	if (png_sig_cmp(data, 0, len)) {
		return 0;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		return 0;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 0;
	}

	data_on_memory_t data_on_memory;
	data_on_memory.top = data;
	data_on_memory.len = len;
	data_on_memory.pos = 0;

	png_set_read_fn(png_ptr, &data_on_memory, read_from_memory);
	png_read_info(png_ptr, info_ptr);

	int result = 1;
	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);
	if (w == 0 || h == 0) {
		result = 0;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	return result;
}

static unsigned char *DecodePng(const unsigned char *data, size_t len, int *_width, int *_height)
{
	*_width = 0;
	*_height = 0;
	if (png_sig_cmp(data, 0, len)) {
		return NULL;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		return NULL;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return NULL;
	}

	data_on_memory_t data_on_memory;
	data_on_memory.top = data;
	data_on_memory.len = len;
	data_on_memory.pos = 0;

	png_set_read_fn(png_ptr, &data_on_memory, read_from_memory);
	png_read_info(png_ptr, info_ptr);

	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);
	if (w == 0 || h == 0) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (bit_depth == 16) {
		png_set_strip_16(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}
	if (color_type & PNG_COLOR_MASK_ALPHA) {
		png_set_strip_alpha(png_ptr);
	}
	// png_set_bgr(png_ptr);

	unsigned char *image = (unsigned char *)malloc(w * h * 3);
	if (image == NULL) {
		return NULL;
	}
	unsigned char **pimage = (unsigned char **)malloc(h * sizeof(char *));
	if (pimage == NULL) {
		return NULL;
	}
	unsigned char *p = image;
	for (int i = 0; i < h; i++) {
		pimage[i] = p;
		p += w * 3;
	}
	png_read_image(png_ptr, pimage);

	free(pimage);

	*_width = w;
	*_height = h;
	return image;
}

__declspec(dllexport) int _stdcall GetPluginInfo(int infono, LPSTR buf, int buflen)
{
	switch (infono) {
		case 0:
			strncpy_s(buf, buflen, "00IN", _TRUNCATE);
			break;
		default:
			buf[0] = '\0';
			break;
	}
	return (int)strlen(buf);
}

__declspec(dllexport) int __stdcall IsSupported(LPCSTR filename, void *dw)
{
	if (((UINT_PTR)dw & 0xffff) == 0) {
		// ファイルから読む
		(void)filename;
		return 0;  // サポートしない
	}

	const unsigned char *data = (unsigned char *)dw;
	size_t len = 4 * 1024;
	return CheckHeader(data, len);
}

__declspec(dllexport) int __stdcall GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo,
											   HANDLE *pHBm, SUSIE_PROGRESS, LONG_PTR)
{
	if (flag != 1) {
		// memory only
		return 0;
	}
	const unsigned char *data = (unsigned char *)buf;
	int width;
	int height;
	unsigned char *image = DecodePng(data, len, &width, &height);
	if (image == NULL) {
		return 2;
	}

	SPICreateBitmapInfoHeaderRGB(image, width, height, pHBInfo, pHBm);
	free(image);

	return 0;
}
