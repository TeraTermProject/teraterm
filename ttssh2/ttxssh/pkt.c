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

// ���s�R�[�h���o�Ă���܂œǂ�
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
	amount_read = i + 1; // �ǂݍ��݃T�C�Y�iLF���܂ށj
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

			// SSH�T�[�o�̃o�[�W�����`�F�b�N���s��
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
				 * ��
				 *   byte     padding_length
				 *   byte[n1] payload; n1 = packet_length - padding_length - 1
				 *   byte[n2] random padding; n2 = padding_length
				 * �̒����̍��v��
				 *   byte[m]  mac (Message Authentication Code - MAC); m = mac_length
				 * �̒������܂܂Ȃ��B
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
				 *   MAC �̑ΏۂƂȂ�f�[�^�ƈꏏ�ɈÍ�������Ȃ��A"MAC �̑ΏۂƂȂ�f�[�^�̒���"�̃T�C�Y
				 *   ���̕����� packet_length �ŁAuint32 (4�o�C�g)
				 *
				 * - �ʏ�� MAC ���� (E&M) �ł̓p�P�b�g���������ꏏ�ɈÍ��������̂� aadlen �� 0 �ƂȂ�B
				 * - EtM ������ MAC �� AEAD �� AES-GCM �ł́A�p�P�b�g���������Í�������Ȃ��̂�
				 * aadlen �� 4 �ƂȂ�B
				 * - AEAD �� chacha20-poly1305 �ł̓p�P�b�g���������Í�������邪�AMAC �̑ΏۂƂȂ�f�[�^
				 * �Ƃ͕ʂɈÍ��������̂� aadlen �� 4 �ƂȂ�B
				 *
				 */
				if ((mac && mac->etm) || authlen > 0) {
					aadlen = 4;
				}

				if (authlen > 0 &&
				    pvar->cc[MODE_IN]->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
					/*
					 * AEAD �� chacha20-poly1305 �ł̓p�P�b�g���������ʂɈÍ�������Ă���B
					 * ���̏����͒������擾���邪�Adata �͈Í������ꂽ�܂܂ƂȂ�B
					 */
					chachapoly_get_length(pvar->cc[MODE_IN]->cp_ctx, &pktsize,
					                      pvar->ssh_state.receiver_sequence_number,
					                      data, pvar->pkt_state.datalen);
				}
				else if (authlen == 0 &&
				         aadlen == 0 &&
				         !pvar->pkt_state.predecrypted_packet && aadlen == 0) {
					/*
					 * AEAD �łȂ� E&M (aadlen �� 0) �̎��́A�Í�������Ă���p�P�b�g����
					 * �m��K�v���L�邽�߁A�擪�� 1 �u���b�N�������O�ɕ�������B
					 */
					SSH_predecrypt_packet(pvar, data);
					pvar->pkt_state.predecrypted_packet = TRUE;

					pktsize = get_uint32_MSBfirst(data);
				}
				else {
					/*
					 * EtM ������ MAC ��AAEAD �� AES-GCM �̂Ƃ��Ȃǂ͂��̂܂ܓǂ߂�B
					 */
					pktsize = get_uint32_MSBfirst(data);
				}
			}
			else {
				pktsize = get_uint32_MSBfirst(data);
			}

			if (SSHv1(pvar)) {
				// SSH1 �ł̓p�P�b�g���̒l�ɂ� padding �̒������܂܂�Ă��Ȃ��B
				// �܂� padding �̒����̏����p�P�b�g��ɂ͖����̂ŁA�p�P�b�g���̒l����v�Z����B
				padding_size = 8 - (pktsize % 8);

				// �ȍ~�̏����� pktsize �� padding_size �̒l���܂܂�Ă��鎖���O��ƂȂ��Ă���B
				pktsize += padding_size;
			}

			// �p�P�b�g(TCP�y�C���[�h)�̑S�̂̃T�C�Y�́A
			// 4�i�p�P�b�g���̃T�C�Y�j+�p�P�b�g���i+MAC�̃T�C�Y�j�ƂȂ�B
			total_packet_size = 4 + pktsize + SSH_get_authdata_size(pvar, MODE_IN);

			if (total_packet_size <= pvar->pkt_state.datalen) {
				// ��M�ς݃f�[�^���\���L��ꍇ�̓p�P�b�g�̎��������s��
				if (SSHv1(pvar)) {
					// SSH1 �� EtM ��Ή� (�������� MAC �ł͂Ȃ� CRC ���g��)
					SSH1_handle_packet(pvar, data, pktsize, padding_size);
				}
				else {
					// SSH2 �ł͂��̎��_�ł� padding ����������������Ă��Ȃ��ꍇ������̂ŁA
					// padding ���͓n�����ɁA�K�v�ɂȂ������ɓ����Ŏ擾����B
					SSH2_handle_packet(pvar, data, pktsize, aadlen, authlen);
				}

				pvar->pkt_state.predecrypted_packet = FALSE;
				pvar->pkt_state.datastart += total_packet_size;
				pvar->pkt_state.datalen -= total_packet_size;

			}
			else if (total_packet_size > PACKET_MAX_SIZE) {
				// �p�P�b�g�����傫������ꍇ�ُ͈�I������B
				// ���ۂɂ͉��炩�̗v���ŕ������s�˃p�P�b�g�����������Ă��鎖�������B
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
			// �p�P�b�g�̎�M
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
