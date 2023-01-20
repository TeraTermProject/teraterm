/*
 * (C) 2023- TeraTerm Project
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

#include <windows.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SET_PORT_NAME,
	GET_RAW_HANDLE,
	SET_COM_DCB,
	SET_COM_TIMEOUTS,
} device_ctrl_request;

typedef struct device {
	struct device_ope_st const *ope;
	void *private_data;
} device_t;

typedef struct device_ope_st {
	/**
	 *	使用しなくなったらコールする
	 */
	DWORD (*destroy)(device_t *device);
	/**
	 *	オープン
	 *	必ず close() すること
	 */
	DWORD (*open)(device_t *device);
	/**
	 *	close()すると再度open()できる
	 */
	DWORD (*close)(device_t *device);
	/**
	 *	read() で読み込みリクエストを行う、
	 *		すぐ読めるかもしれないし
	 *		ペンディングされて後で読み込めるかもしれない
	 */
	DWORD (*read)(device_t *device, uint8_t *buf, size_t buf_len, size_t *readed);

	/**
	 *	read() のリクエストが完了したかチェックする
	 *		読めればreadedにバイト数が入っている
	 *		データはread()時のアドレスに入っている
	 */
	DWORD (*wait_read)(device_t *device, size_t *readed);

	DWORD (*write)(device_t *device, const void *buf, size_t buf_len, size_t *writed);
	DWORD (*wait_write)(device_t *device, size_t *writed);

	/**
	 *	デバイスごとの制御
	 */
	DWORD (*ctrl)(device_t *device, device_ctrl_request request, ...);
} device_ope;


DWORD com_init(device_t **device);

#ifdef __cplusplus
}
#endif
