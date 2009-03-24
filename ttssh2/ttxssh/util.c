/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"

void UTIL_init_sock_write_buf(UTILSockWriteBuf FAR * buf)
{
	buf_create(&buf->bufdata, &buf->buflen);
	buf->datastart = 0;
	buf->datalen = 0;
}

static int send_until_block(PTInstVar pvar, SOCKET s,
                            const char FAR * data, int len)
{
	int total_sent = 0;

	while (len > 0) {
		int sent_amount = (pvar->Psend) (s, data, len, 0);

		if (sent_amount < 0) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				return total_sent;
			} else {
				return sent_amount;
			}
		} else {
			total_sent += sent_amount;
			data += sent_amount;
			len -= sent_amount;
		}
	}

	return total_sent;
}

BOOL UTIL_sock_buffered_write(PTInstVar pvar, UTILSockWriteBuf FAR * buf,
                              UTILBlockingWriteCallback blocking_write,
                              SOCKET socket, const char FAR * data,
                              int len)
{
	int curlen;
	int desiredlen;
	int space_required;
	int amount_to_write_from_buffer;
	BOOL did_block = FALSE;
	int first_copy_start;
	int first_copy_amount;

	/* Fast path case: buffer is empty, try nonblocking write */
	if (buf->datalen == 0) {
#if 0
		int sent_amount = send_until_block(pvar, socket, data, len);

		if (sent_amount < 0) {
			return FALSE;
		}
		data += sent_amount;
		len -= sent_amount;
#else
		// ノンブロッキングモードで送れなかった場合、以降の処理へ続くが、バグがあるため、
		// まともに動かない。ゆえに、初回でブロッキングモードを使い、確実に送信してしまう。
		// ポート転送(local-to-remote)において、でかいパケットを受信できない問題への対処。
		// (2007.11.29 yutaka)
		if (!blocking_write(pvar, socket, data, len)) {
			return FALSE;
		} else {
			len = 0;
		}
#endif
	}

	if (len == 0) {
		return TRUE;
	}

	/* We blocked or the buffer has data in it. We need to put this data
	   into the buffer.
	   First, expand buffer as much as possible and necessary. */
	curlen = buf->buflen;
	desiredlen =
		min(pvar->session_settings.WriteBufferSize, buf->datalen + len);

	if (curlen < desiredlen) {
		int wrap_amount = buf->datastart + buf->datalen - curlen;
		int newlen =
			min(pvar->session_settings.WriteBufferSize, 2 * desiredlen);

		buf->bufdata = realloc(buf->bufdata, newlen);
		buf->buflen = newlen;

		if (wrap_amount > 0) {
			int wrap_to_copy = min(wrap_amount, newlen - curlen);

			memmove(buf->bufdata + curlen, buf->bufdata, wrap_to_copy);
			memmove(buf->bufdata, buf->bufdata + wrap_to_copy,
					wrap_amount - wrap_to_copy);
		}
	}

	/* 1) Write data from buffer
	   2) Write data from user
	   3) Copy remaining user data into buffer
	 */
	space_required = max(0, buf->datalen + len - buf->buflen);
	amount_to_write_from_buffer = min(buf->datalen, space_required);

	if (amount_to_write_from_buffer > 0) {
		int first_part =
			min(amount_to_write_from_buffer, buf->buflen - buf->datastart);

		did_block = TRUE;
		if (!blocking_write
			(pvar, socket, buf->bufdata + buf->datastart, first_part)) {
			return FALSE;
		}
		if (first_part < amount_to_write_from_buffer) {
			if (!blocking_write
				(pvar, socket, buf->bufdata,
				 amount_to_write_from_buffer - first_part)) {
				return FALSE;
			}
		}

		buf->datalen -= amount_to_write_from_buffer;
		if (buf->datalen == 0) {
			buf->datastart = 0;
		} else {
			buf->datastart =
				(buf->datastart +
				 amount_to_write_from_buffer) % buf->buflen;
		}
		space_required -= amount_to_write_from_buffer;
	}

	if (space_required > 0) {
		did_block = TRUE;
		if (!blocking_write(pvar, socket, data, space_required)) {
			return FALSE;
		}
		data += space_required;
		len -= space_required;
	}

	first_copy_start = (buf->datastart + buf->datalen) % buf->buflen;
	first_copy_amount = min(len, buf->buflen - first_copy_start);
	memcpy(buf->bufdata + first_copy_start, data, first_copy_amount);
	if (first_copy_amount < len) {
		memcpy(buf->bufdata, data + first_copy_amount,
			   len - first_copy_amount);
	}
	buf->datalen += len;

	if (did_block) {
		return UTIL_sock_write_more(pvar, buf, socket);
	} else {
		return TRUE;
	}
}

BOOL UTIL_sock_write_more(PTInstVar pvar, UTILSockWriteBuf FAR * buf,
						  SOCKET socket)
{
	int first_amount = min(buf->buflen - buf->datastart, buf->datalen);
	int sent =
		send_until_block(pvar, socket, buf->bufdata + buf->datastart,
						 first_amount);

	if (sent < 0) {
		return FALSE;
	} else {
		if (sent == first_amount && first_amount < buf->datalen) {
			int sentmore =
				send_until_block(pvar, socket, buf->bufdata,
								 buf->datalen - first_amount);

			if (sentmore < 0) {
				return FALSE;
			}
			sent += sentmore;
		}

		buf->datalen -= sent;
		if (buf->datalen == 0) {
			buf->datastart = 0;
		} else {
			buf->datastart = (buf->datastart + sent) % buf->buflen;
		}
	}

	return TRUE;
}

void UTIL_destroy_sock_write_buf(UTILSockWriteBuf FAR * buf)
{
	memset(buf->bufdata, 0, buf->buflen);
	buf_destroy(&buf->bufdata, &buf->buflen);
}

BOOL UTIL_is_sock_deeply_buffered(UTILSockWriteBuf FAR * buf)
{
	return buf->buflen / 2 < buf->datalen;
}

void UTIL_get_lang_msg(PCHAR key, PTInstVar pvar, PCHAR def)
{
	GetI18nStr("TTSSH", key, pvar->ts->UIMsg, sizeof(pvar->ts->UIMsg),
		def, pvar->ts->UILanguageFile);
}

int UTIL_get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PTInstVar pvar)
{
	if (GetI18nLogfont("TTSSH", key, logfont,
					   GetDeviceCaps(GetDC(dlg),LOGPIXELSY),
					   pvar->ts->UILanguageFile) == FALSE) {
		return FALSE;
	}

	if ((*font = CreateFontIndirect(logfont)) == NULL) {
		return FALSE;
	}

	return TRUE;
}

BOOL is_NT4()
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
	    osvi.dwMajorVersion == 4) {
		return TRUE;
	}
	return FALSE;
}
