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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// コマンドライン最大長
// (TGetHNRec.HostNameの確保済み文字長)
//
#define HostNameMaxLength 1024
//#define HostNameMaxLength 80

/* GetHostName dialog record */
typedef struct {
	PCHAR SetupFN; // setup file name
	const wchar_t *SetupFNW;
	WORD PortType; // TCPIP/Serial
	wchar_t *HostName; // host name
	WORD Telnet; // non-zero: enable telnet
	WORD TelPort; // default TCP port# for telnet
	WORD TCPPort; // TCP port #
	WORD ProtocolFamily; // Protocol Family (AF_INET/AF_INET6/AF_UNSPEC)
	WORD ComPort; // serial port #
	WORD MaxComPort; // max serial port #
} TGetHNRec;
typedef TGetHNRec *PGetHNRec;

#ifdef __cplusplus
}
#endif
