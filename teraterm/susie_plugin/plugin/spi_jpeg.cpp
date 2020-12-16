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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setjmp.h>

#include "jpeglib.h"
#include "spi_common.h"

#include "susie_plugin.h"

static void init_mem_source(j_decompress_ptr)
{
  /* no work necessary here */
}

static boolean fill_mem_input_buffer(j_decompress_ptr cinfo)
{
	static const JOCTET mybuffer[4] = {
		(JOCTET)0xFF, (JOCTET)JPEG_EOI, 0, 0
	};

	/* Insert a fake EOI marker */

	cinfo->src->next_input_byte = mybuffer;
	cinfo->src->bytes_in_buffer = 2;

	return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *src = cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num_bytes > 0) {
    while (num_bytes > (long)src->bytes_in_buffer) {
      num_bytes -= (long)src->bytes_in_buffer;
      (void)(*src->fill_input_buffer) (cinfo);
      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->next_input_byte += (size_t)num_bytes;
    src->bytes_in_buffer -= (size_t)num_bytes;
  }
}

static void term_source(j_decompress_ptr)
{
	/* no work necessary here */
}

static void jpeg_mem_src_tj(j_decompress_ptr cinfo, const unsigned char *inbuffer, size_t insize)
{
    struct jpeg_source_mgr *src;

	/* The source object is made permanent so that a series of JPEG images
	 * can be read from the same buffer by calling jpeg_mem_src only before
	 * the first one.
	 */
	if (cinfo->src == NULL) {     /* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
										sizeof(struct jpeg_source_mgr));
	}

	src = cinfo->src;
	src->init_source = init_mem_source;
	src->fill_input_buffer = fill_mem_input_buffer;
	src->skip_input_data = skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->term_source = term_source;
	src->bytes_in_buffer = (size_t)insize;
	src->next_input_byte = (const JOCTET *)inbuffer;
}

static void error_exit(j_common_ptr cinfo)
{
	jmp_buf *setjmp_buffer = (jmp_buf*)cinfo->client_data;
	longjmp(*setjmp_buffer, 1);
}

static int CheckHeader(const unsigned char *data, size_t len)
{
	struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct dinfo;
	jmp_buf setjmp_buffer;
	if (setjmp(setjmp_buffer)) {
		jpeg_destroy_decompress(&dinfo);
		return 0;
	}
	dinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit = error_exit;
	dinfo.client_data = setjmp_buffer;
	jpeg_create_decompress(&dinfo);
	jpeg_mem_src_tj(&dinfo, data, len);
	int r = jpeg_read_header(&dinfo, TRUE);
	jpeg_abort_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);

    return r == JPEG_HEADER_OK ? 1 : 0;
}

static unsigned char *DecodeJpeg(const unsigned char *data, size_t len, int *_width, int *_height)
{
	*_width = 0;
	*_height = 0;

    struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct dinfo;
	jmp_buf setjmp_buffer;
	if (setjmp(setjmp_buffer)) {
		jpeg_destroy_decompress(&dinfo);
		return NULL;
	}
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);
	jpeg_mem_src_tj(&dinfo, data, len);
	int r = jpeg_read_header(&dinfo, TRUE);
    if (r != JPEG_HEADER_OK) {
		return NULL;
	}

	int width = dinfo.image_width;
	int height = dinfo.image_height;
	int stride = width * 3;
//	dinfo.out_color_space = JCS_EXT_BGR;
	unsigned char *dstBuf = (unsigned char *)malloc(width * stride);

	JSAMPROW *row_pointer = (JSAMPROW *)malloc(sizeof(JSAMPROW) * height);
	for (int i = 0; i < height; i++) {
		row_pointer[i] = &dstBuf[i * (size_t)stride];
	}

	jpeg_start_decompress(&dinfo);
	while (dinfo.output_scanline < dinfo.output_height) {
		jpeg_read_scanlines(&dinfo,  &row_pointer[dinfo.output_scanline],
							dinfo.output_height - dinfo.output_scanline);
	}
	free(row_pointer);
	jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

	*_width = width;
	*_height = height;
    return dstBuf;
}

__declspec(dllexport) int __stdcall GetPluginInfo(int infono,LPSTR buf,int buflen)
{
	switch (infono){
	case 0:
		strncpy_s(buf, buflen, "00IN", _TRUNCATE);
		break;
	default:
		buf[0] = '\0';
		break;
	}
	return (int)strlen(buf);
}

__declspec(dllexport) int __stdcall IsSupported(LPCSTR filename,void *dw)
{
	if (((UINT_PTR)dw & 0xffff) == 0) {
		// ファイルから読む
        (void)filename;
		return 0;   // サポートしない
	}

    const unsigned char *data = (unsigned char *)dw;
    size_t len = 4 * 1024;
	return CheckHeader(data, len);
}

__declspec(dllexport) int __stdcall GetPicture(LPCSTR buf,LONG_PTR len,unsigned int flag,HANDLE *pHBInfo,HANDLE *pHBm,SUSIE_PROGRESS,LONG_PTR)
{
	if (flag != 1) {
		// memory only
		return 0;
	}
    const unsigned char *data = (unsigned char *)buf;
	int width;
	int height;
	unsigned char *image = DecodeJpeg(data, len, &width, &height);
	if (image == NULL) {
		return 2;
	}

	SPICreateBitmapInfoHeaderRGB(image, width, height, pHBInfo, pHBm);
    free(image);

    return 0;
}
