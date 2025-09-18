/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004- TeraTerm Project
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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "util.h"
#include "pkt.h"

#define READAMOUNT CHAN_SES_WINDOW_DEFAULT

void PKT_init(PTInstVar pvar)
{
	buf_create(&pvar->pkt_state.buf, &pvar->pkt_state.buflen);
	pvar->pkt_state.datastart = 0;
	pvar->pkt_state.datalen = 0;
	pvar->pkt_state.seen_server_ID = FALSE;
	pvar->pkt_state.seen_newline = FALSE;
	pvar->pkt_state.predecrypted_packet = FALSE;
}

/* Read some data, leave no more than up_to_amount bytes in the buffer,
   return the number of bytes read or -1 on error or blocking. */
static int recv_data(PTInstVar pvar, unsigned long up_to_amount)
{
	int amount_read;

	/* Shuffle data to the start of the buffer */
	if (pvar->pkt_state.datastart != 0) {
		memmove(pvar->pkt_state.buf,
		        pvar->pkt_state.buf + pvar->pkt_state.datastart,
		        pvar->pkt_state.datalen);
		pvar->pkt_state.datastart = 0;
	}

	buf_ensure_size(&pvar->pkt_state.buf, &pvar->pkt_state.buflen, up_to_amount);

	_ASSERT(pvar->pkt_state.buf != NULL);

	amount_read = (pvar->Precv) (pvar->socket,
	                             pvar->pkt_state.buf + pvar->pkt_state.datalen,
	                             up_to_amount - pvar->pkt_state.datalen,
	                             0);

	if (amount_read > 0) {
		/* Update seen_newline if necessary */
		if (!pvar->pkt_state.seen_server_ID && !pvar->pkt_state.seen_newline) {
			int i;

			for (i = 0; i < amount_read; i++) {
				if (pvar->pkt_state.buf[pvar->pkt_state.datalen + i] == '\n') {
					pvar->pkt_state.seen_newline = 1;
				}
			}
		}
		pvar->pkt_state.datalen += amount_read;
	}

	return amount_read;
}

// 改行コードが出てくるまで読む
static int recv_line_data(PTInstVar pvar)
{
	int amount_read;
	char buf[256];
	size_t up_to_amount = sizeof(buf);
	int i;

	/* Shuffle data to the start of the buffer */
	if (pvar->pkt_state.datastart != 0) {
		memmove(pvar->pkt_state.buf,
		        pvar->pkt_state.buf + pvar->pkt_state.datastart,
		        pvar->pkt_state.datalen);
		pvar->pkt_state.datastart = 0;
	}

	buf_ensure_size(&pvar->pkt_state.buf, &pvar->pkt_state.buflen, up_to_amount);

	for (i = 0 ; i < (int)up_to_amount ; i++) {
		amount_read = (pvar->Precv) (pvar->socket, &buf[i], 1, 0);
		if (amount_read != 1) {
			return 0; // error
		}

		pvar->pkt_state.datalen += amount_read;

		if (buf[i] == '\n') { // 0x0a
			buf[i+1] = 0;
			break;
		}
	}
	amount_read = i + 1; // 読み込みサイズ（LFも含む）
	memcpy(pvar->pkt_state.buf, buf, amount_read);

	pvar->pkt_state.seen_newline = 1;

	return amount_read;
}

/* This function does two things:
   -- reads data from the sshd and feeds the SSH protocol packets to ssh.c
   -- copies any available decrypted session data into the application buffer
*/
int PKT_recv(PTInstVar pvar, char *buf, int buflen)
{
	int amount_in_buf = 0;
	BOOL connection_closed = FALSE;

	while (SSH_is_any_payload(pvar) ? buflen > 0 : !connection_closed) {
		if (SSH_is_any_payload(pvar)) {
			/* ssh.c has some session data for us to give to Tera Term. */
			int grabbed = SSH_extract_payload(pvar, buf, buflen);

			amount_in_buf += grabbed;
			buf += grabbed;
			buflen -= grabbed;
		}
		else if (!pvar->pkt_state.seen_server_ID && (pvar->pkt_state.seen_newline || pvar->pkt_state.datalen >= 255)) {
			/*
			 * We're looking for the initial ID string and either we've seen the
			 * terminating newline, or we've exceeded the limit at which we should see a newline.
			 */
			unsigned int i;

			for (i = 0; pvar->pkt_state.buf[i] != '\n' && i < pvar->pkt_state.datalen; i++) {
			}
			if (pvar->pkt_state.buf[i] == '\n') {
				i++;
			}

			// SSHサーバのバージョンチェックを行う
			if (SSH_handle_server_ID(pvar, pvar->pkt_state.buf, i)) {
				pvar->pkt_state.seen_server_ID = 1;

				if (SSHv2(pvar)) {
					// send Key Exchange Init
					SSH2_send_kexinit(pvar);
				}
			} else {
				// reset flag to re-read server ID (2008.1.24 yutaka)
				pvar->pkt_state.seen_newline = 0;
			}

			pvar->pkt_state.datastart += i;
			pvar->pkt_state.datalen -= i;
		}
		else if (pvar->pkt_state.seen_server_ID && pvar->pkt_state.datalen >= SSH_get_min_packet_size(pvar)) {
			char *data = pvar->pkt_state.buf + pvar->pkt_state.datastart;
			uint32 padding_size = 0;
			u_int pktsize = 0;
			uint32 total_packet_size;
			struct Mac *mac = &pvar->ssh2_keys[MODE_IN].mac;
			struct Enc *enc = &pvar->ssh2_keys[MODE_IN].enc;
			int authlen = 0, aadlen = 0;

			if (SSHv2(pvar)) {
				/*
				 * pktsize
				 *   uint32   packet_length
				 * は
				 *   byte     padding_length
				 *   byte[n1] payload; n1 = packet_length - padding_length - 1
				 *   byte[n2] random padding; n2 = padding_length
				 * の長さの合計で
				 *   byte[m]  mac (Message Authentication Code - MAC); m = mac_length
				 * の長さを含まない。
				 * cf. RFC 4253 6. Binary Packet Protocol
				 */

				if (enc && enc->auth_len > 0) {
					authlen = enc->auth_len;
				}

				/*
				 *                      |          | lead 4 bytes are encrypted | aadlen |
				 * Encryption type
				 *   enc->auth_len >  0 | AEAD     | AES-GCM ... no             | 4      |     (2)
				 *                      |          | chacha20-poly1305 ... yes  | 4      | (1) (2)
				 *   enc->auth_len == 0 | not AEAD | depends on MAC type        | <-     |
				 * MAC type
				 *   mac->etm == true   | EtM      | no                         | 4      |
				 *   mac->etm == false  | E&M      | yes                        | 0      |
				 * (1) lead 4 bytes are encrypted separately from main part.
				 * (2) implicit MAC type of AEAD is EtM
				 */
				/*
				 * aadlen: Additional Authenticated Data Length
				 *   MAC の対象となるデータと一緒に暗号化されない、"MAC の対象となるデータの長さ"のサイズ
				 *   この部分は packet_length で、uint32 (4バイト)
				 *
				 * - 通常の MAC 方式 (E&M) ではパケット長部分が一緒に暗号化されるので aadlen は 0 となる。
				 * - EtM 方式の MAC や AEAD の AES-GCM では、パケット長部分が暗号化されないので
				 * aadlen は 4 となる。
				 * - AEAD の chacha20-poly1305 ではパケット長部分が暗号化されるが、MAC の対象となるデータ
				 * とは別に暗号化されるので aadlen は 4 となる。
				 *
				 */
				if ((mac && mac->etm) || authlen > 0) {
					aadlen = 4;
				}

				if (authlen > 0 &&
				    pvar->cc[MODE_IN]->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
					/*
					 * AEAD の chacha20-poly1305 ではパケット長部分が別に暗号化されている。
					 * この処理は長さを取得するが、data は暗号化されたままとなる。
					 */
					chachapoly_get_length(pvar->cc[MODE_IN]->cp_ctx, &pktsize,
					                      pvar->ssh_state.receiver_sequence_number,
					                      data, pvar->pkt_state.datalen);
				}
				else if (authlen == 0 &&
				         aadlen == 0 &&
				         !pvar->pkt_state.predecrypted_packet && aadlen == 0) {
					/*
					 * AEAD でなく E&M (aadlen が 0) の時は、暗号化されているパケット長を
					 * 知る必要が有るため、先頭の 1 ブロックだけ事前に復号する。
					 */
					SSH_predecrypt_packet(pvar, data);
					pvar->pkt_state.predecrypted_packet = TRUE;

					pktsize = get_uint32_MSBfirst(data);
				}
				else {
					/*
					 * EtM 方式の MAC や、AEAD で AES-GCM のときなどはそのまま読める。
					 */
					pktsize = get_uint32_MSBfirst(data);
				}
			}
			else {
				pktsize = get_uint32_MSBfirst(data);
			}

			if (SSHv1(pvar)) {
				// SSH1 ではパケット長の値には padding の長さが含まれていない。
				// また padding の長さの情報もパケット上には無いので、パケット長の値から計算する。
				padding_size = 8 - (pktsize % 8);

				// 以降の処理は pktsize に padding_size の値が含まれている事が前提となっている。
				pktsize += padding_size;
			}

			// パケット(TCPペイロード)の全体のサイズは、
			// 4（パケット長のサイズ）+パケット長（+MACのサイズ）となる。
			total_packet_size = 4 + pktsize + SSH_get_authdata_size(pvar, MODE_IN);

			if (total_packet_size <= pvar->pkt_state.datalen) {
				// 受信済みデータが十分有る場合はパケットの実処理を行う
				if (SSHv1(pvar)) {
					// SSH1 は EtM 非対応 (そもそも MAC ではなく CRC を使う)
					SSH1_handle_packet(pvar, data, pktsize, padding_size);
				}
				else {
					// SSH2 ではこの時点では padding 長部分が復号されていない場合があるので、
					// padding 長は渡さずに、必要になった時に内部で取得する。
					SSH2_handle_packet(pvar, data, pktsize, aadlen, authlen);
				}

				pvar->pkt_state.predecrypted_packet = FALSE;
				pvar->pkt_state.datastart += total_packet_size;
				pvar->pkt_state.datalen -= total_packet_size;

			}
			else if (total_packet_size > PACKET_MAX_SIZE) {
				// パケット長が大きすぎる場合は異常終了する。
				// 実際には何らかの要因で復号失敗⇒パケット長部分が壊れている事が多い。
				UTIL_get_lang_msg("MSG_PKT_OVERSIZED_ERROR", pvar,
				                  "Oversized packet received from server; connection will close.");
				notify_fatal_error(pvar, pvar->UIMsg, TRUE);
			}
			else {
				int amount_read = recv_data(pvar, max(total_packet_size, READAMOUNT));

				if (amount_read == SOCKET_ERROR) {
					if (amount_in_buf == 0) {
						return SOCKET_ERROR;
					} else {
						return amount_in_buf;
					}
				} else {
					if (amount_read == 0) {
						connection_closed = TRUE;
					}
				}
			}
		} else {
			// パケットの受信
			int amount_read;

			amount_read = recv_data(pvar, READAMOUNT);

			if (amount_read == SOCKET_ERROR) {
				if (amount_in_buf == 0) {
					return SOCKET_ERROR;
				} else {
					return amount_in_buf;
				}
			} else if (amount_read == 0) {
				connection_closed = TRUE;
			}
		}

		if (pvar->fatal_error) {
			return amount_in_buf;
		}
	}

	if (SSH_is_any_payload(pvar)) {
		PostMessage(pvar->NotificationWindow, WM_USER_COMMNOTIFY, pvar->socket, MAKELPARAM(FD_READ, 0));
	}

	return amount_in_buf;
}

void PKT_end(PTInstVar pvar)
{
	buf_destroy(&pvar->pkt_state.buf, &pvar->pkt_state.buflen);
}
