
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <stdint.h>
#include <conio.h>
#include <crtdbg.h>

#include "getopt.h"

#include "asprintf.h"
#include "codeconv.h"
#include "win32helper.h"

#include "deviceope.h"

static void usage()
{
	printf(
        "ttcomtester [options]\n"
        "  -h, --help               show this\n"
        "  -v, --verbose            verbose, 'v' command\n"
        "  -i, --inifile [inifile]  read inifile, default=ttcomtester.ini\n"
        "  -r, rts [rts]            RTS/CTS(HARD) flow {off|on|hs|toggle}\n"
        "  -d, --device [name]      open device name, ex com2\n"
		);
}

static void key_usage(void)
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
	printf("sizeof(DCB)                     %d\n", p->DCBlength);
	printf("Baudrate at which running       %d\n", p->BaudRate);
	printf("Binary Mode (skip EOF check)    %d\n", p->fBinary);
	printf("Enable parity checking          %d\n", p->fParity);
	printf("CTS handshaking on output       %d\n", p->fOutxCtsFlow);
	printf("DSR handshaking on output       %d\n", p->fOutxDsrFlow);
	printf("DTR Flow control                %d\n", p->fDtrControl);
	printf("DSR Sensitivity                 %d\n", p->fDsrSensitivity);
	printf("Continue TX when Xoff sent      %d\n", p->fTXContinueOnXoff);
	printf("Enable output X-ON/X-OFF        %d\n", p->fOutX);
	printf("Enable input X-ON/X-OFF         %d\n", p->fInX);
	printf("Enable Err Replacement          %d\n", p->fErrorChar);
	printf("Enable Null stripping           %d\n", p->fNull);
	printf("Rts Flow control                %d\n", p->fRtsControl);
	printf("Abort all reads and writes on Error %d\n", p->fAbortOnError);
	printf("Reserved                        %d\n", p->fDummy2);
	printf("Not currently used              %d\n", p->wReserved);
	printf("Transmit X-ON threshold         %d\n", p->XonLim);
	printf("Transmit X-OFF threshold        %d\n", p->XoffLim);
	printf("Number of bits/byte, 4-8        %d\n", p->ByteSize);
	printf("0-4=None,Odd,Even,Mark,Space    %d\n", p->Parity);
	printf("0,1,2 = 1, 1.5, 2               %d\n", p->StopBits);
	printf("Tx and Rx X-ON character        %d\n", p->XonChar);
	printf("Tx and Rx X-OFF character       %d\n", p->XoffChar);
	printf("Error replacement char          %d\n", p->ErrorChar);
	printf("End of Input character          %d\n", p->EofChar);
	printf("Received Event character        %d\n", p->EvtChar);
	printf("Fill for now.                   %d\n", p->wReserved1);
}

void dumpCOMMTIMEOUTS(const COMMTIMEOUTS *p)
{
	printf("Maximum time between read chars %d\n", p->ReadIntervalTimeout);
	printf("read Multiplier of characters   %d\n", p->ReadTotalTimeoutMultiplier);
	printf("read Constant in milliseconds   %d\n", p->ReadTotalTimeoutConstant);
	printf("write Multiplier of characters  %d\n", p->WriteTotalTimeoutMultiplier);
	printf("write Constant in milliseconds  %d\n", p->WriteTotalTimeoutConstant);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(269);
#endif

	bool verbose = false;
	wchar_t *ini_base = L"ttcomtester.ini";
	const wchar_t *arg_rts = L"off";
	wchar_t *prog = argv[0];
	wchar_t *arg_device_name = NULL;

	static const struct option_w long_options[] = {
		{L"help", no_argument, NULL, L'h'},
		{L"verbose", no_argument, NULL, L'v'},
		{L"inifile", required_argument, NULL, L'i'},
		{L"rts", required_argument, NULL, L'r'},
		{L"device", required_argument, NULL, L'd'},
		{}
	};


	opterr = 0;
    while(1) {
		int c = getopt_long_w(argc, argv, L"vhi:r:d:", long_options, NULL);
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
		case L'i':
			ini_base = optarg_w;
			break;
		case L'r': {
			if (wcscmp(optarg_w, L"off") == 0) {
				arg_rts = L"off";
			} else if (wcscmp(optarg_w, L"on") == 0) {
				arg_rts = L"on";
			} else if (wcscmp(optarg_w, L"hs") == 0) {
				arg_rts = L"hs";
			} else if (wcscmp(optarg_w, L"toggle") == 0) {
				arg_rts = L"toggle";
			} else {
				printf("check rts option");
				exit(1);
			}
			break;
		}
		case L'd': {
			arg_device_name = _wcsdup(optarg_w);
			break;
		}
		default:
			usage();
			return 0;
			break;
		}
    }

	wchar_t *ini_path = _wcsdup(prog);
	wchar_t *p = wcsrchr(ini_path, '\\');
	*p = 0;
	wchar_t *ini;
	aswprintf(&ini, L"%s\\%s", ini_path, ini_base);
	wprintf(L"ini='%s'\n", ini);

	wchar_t *ini_device_name;
	hGetPrivateProfileStringW(L"ttcomtester", L"device", L"com1", ini, &ini_device_name);
	wchar_t *com_param;
	wchar_t *com_param_default = L"baud=9600 parity=N data=8 stop=1";
	//wchar_t *com_param_default = L"baud=9600 parity=N data=8 stop=1 rts=hs";		// error?
	hGetPrivateProfileStringW(L"ttcomtester", L"com_param", com_param_default, ini, &com_param);
	free(ini);
	ini = nullptr;
	free(ini_path);
	ini_path = nullptr;

	const wchar_t *device_name = arg_device_name;
	if (arg_device_name == NULL) {
		device_name = ini_device_name;
	}
	device_t *dev;
	com_init(&dev);

	device_ope const *ope = dev->ope;
	ope->ctrl(dev, SET_PORT_NAME, device_name);
	if (verbose) {
		printf("param='%ls'\n", com_param);
	}

	{
		DCB dcb;
		memset(&dcb, 0, sizeof(dcb));	// 100% buildしてくれないようだ
		dcb.DCBlength = sizeof(dcb);
		BOOL r = BuildCommDCBW(com_param, &dcb);
		if (wcscmp(arg_rts, L"off") == 0) {
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
			dcb.fOutxCtsFlow = FALSE;
		} else if (wcscmp(arg_rts, L"on") == 0) {
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			dcb.fOutxCtsFlow = FALSE;
		} else if (wcscmp(arg_rts, L"hs") == 0) {
			dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
			dcb.fOutxCtsFlow = TRUE;
		} else if (wcscmp(arg_rts, L"toggle") == 0) {
			dcb.fRtsControl = RTS_CONTROL_TOGGLE;
			//dcb.fOutxCtsFlow = TRUE; //??
		}
		if (r == FALSE) {
			DWORD e = GetLastError();
			wchar_t b[128];
			swprintf_s(b, _countof(b), L"BuildCommDCBW('%s')", com_param);
			DispErrorStr(b, e);
			goto finish;
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
					printf("open com='%ls'\n", device_name);
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
					printf("send big data %zu bytes\n", send_len);
					unsigned char *send_data = (unsigned char* )malloc(send_len);
					for(size_t i = 0; i < send_len; i++) {
						send_data[i] = (unsigned char)i;
					}
					size_t sent_len;
					DWORD e = ope->write(dev, send_data, send_len, &sent_len);
					if (e == ERROR_SUCCESS) {
						printf("sent %zu bytes\n", sent_len);
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
					check_line_state = i;
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
					key_usage();
					break;
				case '1': {
					// 指定バイト数を1byteづつ送るテスト
					// FT232Rのとき、CTS=0状態で67byte程度送信するとおかしな状態になる
					int send_size = 65;
					for(int i =0 ; i < send_size; i++) {
						char send_text[2];
						send_text[0] = i;
						size_t sent_len;
						DWORD e = ope->write(dev, send_text, 1, &sent_len);
						if (e == ERROR_SUCCESS) {
							printf("send %02x, sent %zu byte %s\n", c, sent_len,
								   sent_len != 0 ? "" : "(flow control or send buffer full)");
						}
						else if (e == ERROR_IO_PENDING) {
							printf("send %02x, sent %zu byte pending\n", c, sent_len);
							for(;;) {
								size_t sent_len;
								e = ope->wait_write(dev, &sent_len);
								if (e == ERROR_IO_PENDING) {
									if (sent_len != 0) {
										printf("send size %zu (pending)\n", sent_len);
									}
								}
								else if (e == ERROR_SUCCESS) {
									printf("send size %zu (finish)\n", sent_len);
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
								printf("send %02x, sent %zu byte %s\n", c, sent_len,
									   sent_len != 0 ? "" : "(flow control or send buffer full)");
							}
							else if (e == ERROR_IO_PENDING) {
								printf("send %02x, sent %zu byte pending\n", c, sent_len);
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
						printf("send size %zu (pending)\n", sent_len);
					}
				}
				else if (e == ERROR_SUCCESS) {
					printf("send size %zu (finish)\n", sent_len);
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

finish:
	free(ini_device_name);
	free(arg_device_name);
	free(com_param);
	return 0;
}
