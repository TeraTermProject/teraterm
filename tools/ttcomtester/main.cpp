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

#include <stdio.h>
#include <locale.h>
#include <windows.h>
#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#endif
#include <conio.h>
#include <crtdbg.h>
#include <string>
#include <map>
#include <assert.h>

#include "getopt.h"
#include "buildcommdcbw.h"
#include "deviceope.h"

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char	uint8_t;
//typedef unsigned short  uint16_t;
//typedef unsigned int	uint32_t;
#endif

static void usage()
{
	printf(
        "ttcomtester [options]\n"
        "  -h, --help               show this\n"
        "  -v, --verbose            verbose, 'v' command\n"
        "  -i, --inifile [inifile]  read inifile, default=ttcomtester.ini\n"
        "  -d, --device [name]      open device name, ex com2\n"
        "  -t, --test [name]        run test\n"
        "  -o, --option [option]\n"
        "        \"BuildComm=baud=9600 parity=N data=8 stop=1\"\n"
		);
}

void DispErrorStr(const wchar_t *str, DWORD err)
{
	wchar_t *buf;
	DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessageW(flags, 0, err, 0, (LPWSTR) & buf, 0, NULL);
	wprintf(L"%s %d,0x%x %s", str, err, err, buf);
	LocalFree(buf);
}

void dump(const uint8_t *buf, size_t len)
{
	if (len == 0) {
		return;
	}
	printf("%ld :", (long)len);
	for (size_t i = 0; i < len; i++) {
		printf("%02x ", *buf++);
	}
	printf("\n");
}

void dumpDCB(const DCB *p)
{
	printf("DCBlength         sizeof(DCB)                     %d\n", p->DCBlength);
	printf("BaudRate          Baudrate at which running       %d\n", p->BaudRate);
	printf("fBinary           Binary Mode (skip EOF check)    %d\n", p->fBinary);
	printf("fParity           Enable parity checking          %d\n", p->fParity);
	printf("fOutxCtsFlow      CTS handshaking on output       %d\n", p->fOutxCtsFlow);
	printf("fOutxDsrFlow      DSR handshaking on output       %d\n", p->fOutxDsrFlow);
	printf("fDtrControl       DTR Flow control                %d\n", p->fDtrControl);
	printf("fDsrSensitivity   DSR Sensitivity                 %d\n", p->fDsrSensitivity);
	printf("fTXContinueOnXoff Continue TX when Xoff sent      %d\n", p->fTXContinueOnXoff);
	printf("fOutX             Enable output X-ON/X-OFF        %d\n", p->fOutX);
	printf("fInX              Enable input X-ON/X-OFF         %d\n", p->fInX);
	printf("fErrorChar        Enable Err Replacement          %d\n", p->fErrorChar);
	printf("fNull             Enable Null stripping           %d\n", p->fNull);
	printf("fRtsControl       Rts Flow control                %d\n", p->fRtsControl);
	printf("fAbortOnError Abort all reads and writes on Error %d\n", p->fAbortOnError);
	printf("fDummy2           Reserved                        %d\n", p->fDummy2);
	printf("wReserved         Not currently used              %d\n", p->wReserved);
	printf("XonLim            Transmit X-ON threshold         %d\n", p->XonLim);
	printf("XoffLim           Transmit X-OFF threshold        %d\n", p->XoffLim);
	printf("ByteSize          Number of bits/byte, 4-8        %d\n", p->ByteSize);
	printf("Parity            0-4=None,Odd,Even,Mark,Space    %d\n", p->Parity);
	printf("StopBits          0,1,2 = 1, 1.5, 2               %d\n", p->StopBits);
	printf("XonChar           Tx and Rx X-ON character        %d\n", p->XonChar);
	printf("XoffChar          Tx and Rx X-OFF character       %d\n", p->XoffChar);
	printf("ErrorChar         Error replacement char          %d\n", p->ErrorChar);
	printf("EofChar           End of Input character          %d\n", p->EofChar);
	printf("EvtChar           Received Event character        %d\n", p->EvtChar);
	printf("wReserved1        Fill for now.                   %d\n", p->wReserved1);
}

void dumpCOMMTIMEOUTS(const COMMTIMEOUTS *p)
{
	printf("Maximum time between read chars %d\n", p->ReadIntervalTimeout);
	printf("read Multiplier of characters   %d\n", p->ReadTotalTimeoutMultiplier);
	printf("read Constant in milliseconds   %d\n", p->ReadTotalTimeoutConstant);
	printf("write Multiplier of characters  %d\n", p->WriteTotalTimeoutMultiplier);
	printf("write Constant in milliseconds  %d\n", p->WriteTotalTimeoutConstant);
}

bool verbose = false;
std::wstring prog;
std::wstring device_name;
std::map<std::wstring, std::wstring> options;

static void SendReceiveKeyUsage(void)
{
	printf(
		"key:\n"
		"   command mode\n"
		"':'	go send mode\n"
		"'o'	device open\n"
		"'c'	device close\n"
		"'q'	quit\n"
		"'r'	RTS 0/1\n"
		"'d'	DTR 0/1\n"
		"'x'	XON/XOFF\n"
		"'e'	echo on/off\n"
		"'s'	send big data\n"
		"'l'	disp line state\n"
		"'L'	check line state before sending\n"
		"'D'	open device dialogbox\n"
		"'v'	verbose on/off\n"
		"'1'   send some bytes\n"
		"   send mode\n"
		"':'	go command mode\n"
		);
}

static void SendReceive(device_t *dev)
{
	device_ope const *ope = dev->ope;
	printf(
		"':'		switch mode\n"
		"' '(space)	key usage\n"
		);
	bool quit_flag = false;
	bool receive_pending = false;
	bool command_mode = true;
	bool rts = true;
	bool xon = true;
	bool dtr = true;
	bool echo_mode = false;
	bool check_line_state = true;
	bool write_pending = false;
	enum {
		STATE_CLOSE,
		STATE_OPEN,
		STATE_ERROR,
	} state = STATE_CLOSE;
	printf("command mode\n");
	while (!quit_flag) {
		if (_kbhit() == 0) {
			// キーが押されていない
			Sleep(1);
		}
		else {
			int c = _getch();
			if (command_mode) {
				switch (c) {
				case 'o': {
					printf("open com='%ls'\n", device_name.c_str());
					DWORD e = ope->open(dev);
					if (e == ERROR_SUCCESS) {
						state = STATE_OPEN;
						if (verbose) {
							DCB dcb;
							ope->ctrl(dev, GET_COM_DCB, &dcb);
							dumpDCB(&dcb);
							COMMTIMEOUTS timeouts;
							ope->ctrl(dev, GET_COM_TIMEOUTS, &timeouts);
							dumpCOMMTIMEOUTS(&timeouts);
						}
					}
					else {
						DispErrorStr(L"open()", e);
					}
					receive_pending = false;
					write_pending = false;
					break;
				}
				case 'c': {
					printf("close\n");
					ope->close(dev);
					state = STATE_CLOSE;
					break;
				}
				case 's': {
					size_t send_len = 32*1024;
					printf("send big data %ul bytes\n", (unsigned long)send_len);
					unsigned char *send_data = (unsigned char* )malloc(send_len);
					for(size_t i = 0; i < send_len; i++) {
						send_data[i] = (unsigned char)i;
					}
					size_t sent_len;
					DWORD e = ope->write(dev, send_data, send_len, &sent_len);
					if (e == ERROR_SUCCESS) {
						printf("sent %ul bytes\n", (unsigned long)sent_len);
					}
					else if (e == ERROR_IO_PENDING) {
						printf("send pending..\n");
						write_pending = true;
					}
					else {
						DispErrorStr(L"write()", e);
					}
					free(send_data);
					break;
				}
				case 'q': {
					printf("quit\n");
					quit_flag = true;
					break;
				}
				case 'r': {
					HANDLE h;
					ope->ctrl(dev, GET_RAW_HANDLE, &h);
					BOOL b;
					if (rts) {
						rts = false;
						b = EscapeCommFunction(h, CLRRTS);
						printf("RTS=0, %d\n", b);
						if (b == 0) {
							DWORD err = GetLastError();
							DispErrorStr(L"EscapeCommFunction()", err);
						}
					}
					else {
						rts = true;
						b = EscapeCommFunction(h, SETRTS);
						printf("RTS=1, %d\n", b);
						if (b == 0) {
							DWORD err = GetLastError();
							DispErrorStr(L"EscapeCommFunction()", err);
						}
					}
					break;
				}
				case 'd': {
					HANDLE h;
					ope->ctrl(dev, GET_RAW_HANDLE, &h);
					BOOL b;
					if (dtr) {
						dtr = false;
						b = EscapeCommFunction(h, CLRDTR);
						printf("DTR=0, %d\n", b);
					}
					else {
						dtr = true;
						b = EscapeCommFunction(h, SETDTR);
						printf("DTR=1, %d\n", b);
					}
					break;
				}
				case 'x': {
					HANDLE h;
					ope->ctrl(dev, GET_RAW_HANDLE, &h);
					BOOL b;
					if (xon) {
						xon = false;
						b = EscapeCommFunction(h, SETXOFF);
						printf("XON=0, %d\n", b);
					}
					else {
						xon = true;
						b = EscapeCommFunction(h, SETXON);
						printf("XON=1, %d\n", b);
					}
					break;
				}
				case 'e': {
					if (echo_mode) {
						printf("echo off\n");
						echo_mode = false;
					}
					else {
						printf("echo on\n");
						echo_mode = true;
					}
					break;
				}
				case 'l': {
					HANDLE h;
					ope->ctrl(dev, GET_RAW_HANDLE, &h);
					DWORD modem_state;
					BOOL b = GetCommModemStatus(h, &modem_state);
					if (b == FALSE) {
						DWORD err = GetLastError();
						DispErrorStr(L"GetCommModemStatus()", err);
					}
					else {
						printf("CTS %s / DSR %s / RING %s / RLSD %s / 0x%02x\n",
							   (modem_state & MS_CTS_ON) == 0 ? "OFF" : "ON ",
							   (modem_state & MS_DSR_ON) == 0 ? "OFF" : "ON ",
							   (modem_state & MS_RING_ON) == 0 ? "OFF" : "ON ",
							   (modem_state & MS_RLSD_ON) == 0 ? "OFF" : "ON ",
							   modem_state
							);
					}
					break;
				}
				case 'L': {
					int i = check_line_state ? 0 : 1;
					printf("check line state before sending %s\n", i == 0 ? "off" : "on");
					ope->ctrl(dev, SET_CHECK_LINE_STATE_BEFORE_SEND, i);
					check_line_state = i == 0 ? false : true;
					break;
				}
				case 'D': {
					ope->ctrl(dev, OPEN_CONFIG_DIALOG);
					if (verbose) {
						DCB dcb;
						ope->ctrl(dev, GET_COM_DCB, &dcb);
						dumpDCB(&dcb);
					}
					break;
				}
				case 'v': {
					verbose = verbose ? false : true;
					printf("verbose %s\n", verbose?"on":"off");
					break;
				}
				case ':': {
					printf("\nsend mode\n");
					command_mode = false;
					break;
				}
				default:
					printf("unknown command '%c'\n", c);
					SendReceiveKeyUsage();
					break;
				case '1': {
					// 指定バイト数を1byteづつ送るテスト
					// FT232Rのとき、CTS=0状態で67byte程度送信するとおかしな状態になる
					int send_size = 65;
					for(int i =0 ; i < send_size; i++) {
						char send_text[2];
						send_text[0] = (char)(unsigned char)i;
						size_t sent_len;
						DWORD e = ope->write(dev, send_text, 1, &sent_len);
						if (e == ERROR_SUCCESS) {
							printf("send %02x, sent %ul byte %s\n", c, (unsigned long)sent_len,
								   sent_len != 0 ? "" : "(flow control or send buffer full)");
						}
						else if (e == ERROR_IO_PENDING) {
							printf("send %02x, sent %ul byte pending\n", c, (unsigned long)sent_len);
							for(;;) {
								e = ope->wait_write(dev, &sent_len);
								if (e == ERROR_IO_PENDING) {
									if (sent_len != 0) {
										printf("send size %ul (pending)\n", (unsigned long)sent_len);
									}
								}
								else if (e == ERROR_SUCCESS) {
									printf("send size %ul (finish)\n", (unsigned long)sent_len);
									break;
								}
								else {
									DispErrorStr(L"write() error", e);
									state = STATE_ERROR;
									printf("send error\n");
								}
								Sleep(1);
							}
						}
						else {
							DispErrorStr(L"write() error", e);
							state = STATE_ERROR;
						}
					}
				}
				}
			}
			else {
				if (c == ':') {
					printf("\ncommand mode\n");
					command_mode = true;
				}
				else {
					if (state == STATE_OPEN) {
						if (write_pending) {
							printf("writing(pending)..\n");
						}
						else {
							char send_text[2];
							size_t sent_len;
							send_text[0] = (char)c;
							DWORD e = ope->write(dev, send_text, 1, &sent_len);
							if (e == ERROR_SUCCESS) {
								printf("send %02x, sent %ul byte %s\n", c, (unsigned long)sent_len,
									   sent_len != 0 ? "" : "(flow control or send buffer full)");
							}
							else if (e == ERROR_IO_PENDING) {
								printf("send %02x, sent %ul byte pending\n", c, (unsigned long)sent_len);
								write_pending = true;
							} else if (e == ERROR_INSUFFICIENT_BUFFER) {
								printf("send buffer full\n");
							}
							else {
								DispErrorStr(L"write() error", e);
								state = STATE_ERROR;
							}
						}
					}
				}
			}
		}

		static uint8_t buf[1024];
		size_t len = 0;
		DWORD e;
		if (state != STATE_OPEN) {
			Sleep(1);
		}
		else {
			if (write_pending == true) {
				size_t sent_len;
				e = ope->wait_write(dev, &sent_len);
				if (e == ERROR_IO_PENDING) {
					// まだ待ち状態
					if (sent_len != 0) {
						printf("send size %ul (pending)\n", (unsigned long)sent_len);
					}
				}
				else if (e == ERROR_SUCCESS) {
					printf("send size %ul (finish)\n", (unsigned long)sent_len);
					write_pending = false;
				} else {
					DispErrorStr(L"write() error", e);
					state = STATE_ERROR;
					printf("send error\n");
				}
			}
			if (receive_pending == false) {
				e = ope->read(dev, buf, sizeof(buf), &len);
				if (e == ERROR_SUCCESS) {
					if(len > 0) {
						printf("read:\n");
						dump(buf, len);
					}
				}
				else if (e == ERROR_IO_PENDING) {
					printf("read pending\n");
					receive_pending = true;
				}
				else {
					DispErrorStr(L"read() error", e);
					state = STATE_ERROR;
				}
			}
			else {
				e = ope->wait_read(dev, &len);
				if (e == ERROR_IO_PENDING) {
					// まだ待ち状態
					;
				}
				else if (e == ERROR_SUCCESS) {
					printf("wait_read:\n");
					dump(buf, len);
					receive_pending = false;
				}
				else {
					DispErrorStr(L"wait_read", e);
					state = STATE_ERROR;
					receive_pending = false;
				}
			}
		}

		if (echo_mode) {
			if (len > 0){
				printf("echo\n");
				ope->write(dev, buf, len, NULL);
			}
		}
	}
}

void echo(device_t *dev, int rts_toggle_ms)
{
	device_ope const *ope = dev->ope;
	bool receive_pending = false;
	bool write_pending = false;
	bool rts_assert = true;
	DWORD rts_toggle_tick = timeGetTime();

	if (rts_toggle_ms != 0) {
		// disable RTS flow control
		DCB dcb;
		ope->ctrl(dev, GET_COM_DCB, &dcb);
		dumpDCB(&dcb);
		dcb.fRtsControl = RTS_CONTROL_DISABLE;
		ope->ctrl(dev, SET_COM_DCB, &dcb);
	}

	DWORD e = ope->open(dev);
	if (e != ERROR_SUCCESS) {
		DispErrorStr(L"open()", e);
		return;
	}

	if (verbose) {
		DCB dcb;
		ope->ctrl(dev, GET_COM_DCB, &dcb);
		dumpDCB(&dcb);
		COMMTIMEOUTS timeouts;
		ope->ctrl(dev, GET_COM_TIMEOUTS, &timeouts);
		dumpCOMMTIMEOUTS(&timeouts);
	}

	printf(
		"'q'		quit\n"
		);
	for(;;) {
		if (_kbhit() == 0) {
			// キーが押されていない
			Sleep(1);
		}
		else {
			int c = _getch();
			printf("\n'%c'\n", c);
			switch (c) {
			case 'q': {
				ope->close(dev);
				return;
			}
			default:
				;
			}
		}

		if (rts_toggle_ms != 0) {
			DWORD now = timeGetTime();
			if (rts_toggle_tick == 0 || (now - rts_toggle_tick >= (DWORD)rts_toggle_ms)) {
				HANDLE h;
				ope->ctrl(dev, GET_RAW_HANDLE, &h);
				if (rts_assert) {
					rts_assert = false;
					BOOL b = EscapeCommFunction(h, CLRRTS);
					if (b == 0) {
						e = GetLastError();
						DispErrorStr(L"EscapeCommFunction()", e);
					}
					printf("t");
				} else {
					rts_assert = true;
					BOOL b = EscapeCommFunction(h, SETRTS);
					if (b == 0) {
						e = GetLastError();
						DispErrorStr(L"EscapeCommFunction()", e);
					}
					printf("T");
				}
				rts_toggle_tick = now;
			}
		}

		static uint8_t sbuf[1024];
		static size_t slen = 0;
		if (slen == 0) {
			static uint8_t rbuf[1024];
			size_t rlen = 0;
			if (receive_pending == false) {
				e = ope->read(dev, rbuf, sizeof(rbuf), &rlen);
				if (e == ERROR_SUCCESS) {
					printf("r");
					memcpy(sbuf, rbuf, rlen);
					slen = rlen;
				}
				else if (e == ERROR_IO_PENDING) {
					receive_pending = true;
				}
				else {
					DispErrorStr(L"read() error", e);
					return;
				}
			}
			else {
				e = ope->wait_read(dev, &rlen);
				if (e == ERROR_IO_PENDING) {
					// まだ待ち状態
					;
				}
				else if (e == ERROR_SUCCESS) {
					printf("R");
					receive_pending = false;
					memcpy(sbuf, rbuf, rlen);
					slen = rlen;
				}
				else {
					DispErrorStr(L"read() error", e);
					return;
				}
			}
		}
		else {
			size_t sent_len;
			if (write_pending == false) {
				e = ope->write(dev, sbuf, slen, &sent_len);
				if (e == ERROR_SUCCESS) {
					printf("s");
					slen = slen - sent_len;
					if (slen != 0) {
						printf("!");
					}
				}
				else if (e == ERROR_IO_PENDING) {
					write_pending = true;
				} else if (e == ERROR_INSUFFICIENT_BUFFER) {
					printf("send buffer full\n");
				}
				else {
					DispErrorStr(L"write() error", e);
				}
			}
			else {
				e = ope->wait_write(dev, &sent_len);
				if (e == ERROR_IO_PENDING) {
					// まだ待ち状態
					;
				}
				else if (e == ERROR_SUCCESS) {
					printf("S");
					slen = slen - sent_len;
					if (slen != 0) {
						printf("!");
					}
					else {
						write_pending = false;
					}
				} else {
					DispErrorStr(L"write() error", e);
					printf("send error\n");
				}
			}
		}
	}
}

void EchoSimple(device_t *dev)
{
	echo(dev, 0);
}

void EchoRtsToggle(device_t *dev)
{
	echo(dev, 1*1000);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::wstring test_name = L"send/receive";
	options[L"BuildComm"] = L"baud=9600 parity=N data=8 stop=1 octs=on rts=on";

	{
		wchar_t *p = wcsrchr(argv[0], '\\');
		if (p != NULL) {
			prog = std::wstring(p+1);
		}
		else {
			prog = std::wstring(argv[0]);
		}
	}

	static const struct option_w long_options[] = {
		{L"help", no_argument, NULL, L'h'},
		{L"verbose", no_argument, NULL, L'v'},
		{L"device", required_argument, NULL, L'd'},
		{L"option", required_argument, NULL, L'o'},
		{L"testname", required_argument, NULL, L't'},
		{}
	};

	opterr = 0;
    for(;;) {
		int c = getopt_long_w(argc, argv, L"vhd:o:t:", long_options, NULL);
        if(c == -1) break;

        switch (c)
        {
		case L'h':
		case L'?':
			usage();
			return 0;
			break;
		case L'v':
			verbose = true;
			break;
		case L'd': {
			device_name = optarg_w;
			break;
		}
		case L'o': {
			if (optarg_w != NULL) {
				std::wstring key;
				std::wstring param;
				const wchar_t *p = wcschr(optarg_w, L'=');
				if (p == NULL) {
					key = std::wstring(optarg_w);
					param.clear();
				} else {
					size_t key_size = p - optarg_w;
					key = std::wstring(optarg_w, key_size);
					param = std::wstring(p+1);
				}
				options[key] = param;
			}
			break;
		}
		case L't':
			test_name = optarg_w;
			break;
		default:
			usage();
			return 0;
			break;
		}
    }

	if (device_name.empty()) {
		printf("specify device name\n"
			   "  --device [name]\n");
		return 0;
	}

	device_t *dev;
	com_init(&dev);

	device_ope const *ope = dev->ope;
	ope->ctrl(dev, SET_PORT_NAME, device_name.c_str());

	{
		const wchar_t *param = options[L"BuildComm"].c_str();
		if (verbose) {
			printf("param='%ls'\n", param);
		}
		DCB dcb;
		memset(&dcb, 0, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		BOOL r = _BuildCommDCBW(param, &dcb);
		if (r == FALSE) {
			DWORD e = GetLastError();
			dumpDCB(&dcb);
			wchar_t b[128];
			swprintf_s(b, _countof(b), L"BuildCommDCBW('%s')", param);
			DispErrorStr(b, e);
			return 0;
		}
		ope->ctrl(dev, SET_COM_DCB, &dcb);
	}

	{
		COMMTIMEOUTS timeouts;
		memset(&timeouts, 0, sizeof(timeouts));
#if 0
		// Tera Term が設定しているパラメータ
		timeouts.ReadIntervalTimeout = MAXDWORD;
		timeouts.WriteTotalTimeoutConstant = 500;
#endif
#if 0
		// CODE PROJECT の値
		// https://www.codeproject.com/articles/3061/creating-a-serial-communication-on-win
		timeouts.ReadIntervalTimeout = 3;
		timeouts.ReadTotalTimeoutMultiplier = 3;
		timeouts.ReadTotalTimeoutConstant = 2;
		timeouts.WriteTotalTimeoutMultiplier = 3;
		timeouts.WriteTotalTimeoutConstant = 2;
#endif
#if 0
		// PuTTYの値
		timeouts.ReadIntervalTimeout = 1;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;
#endif
#if 1
		// 今回提案する値
		timeouts.ReadIntervalTimeout = 1;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 20;
		//timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 1;
#endif
		ope->ctrl(dev, SET_COM_TIMEOUTS, &timeouts);
	}

	std::map<std::wstring, void (*)(device_t *dev)> test_list;
	test_list[L"send/receive"] = SendReceive;
	test_list[L"echo"] = EchoSimple;
	test_list[L"echo_rts"] = EchoRtsToggle;

	void (*test_func)(device_t *) = test_list[test_name];
	if (test_func == NULL) {
		printf("unknown test '%ls'\n", test_name.c_str());
	}
	else {
		test_func(dev);
	}

	return 1;
}
