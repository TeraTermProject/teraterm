
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
		"ttcomtester [option]\n"
		"  -h\n"
		"  -v\n"
		"  -i ttcomtester.ini\n"
		);
}

static void key_usage(void)
{
	printf(
		"key:\n"
		"   command mode\n"
		"':'	go send mode\n"
		"'o'	open\n"
		"'c'	close\n"
		"'q'	quit\n"
		"'r'	RTS 0/1\n"
		"'d'	DTR 0/1\n"
		"'x'	XON/XOFF\n"
		"'e'	echo on/off\n"
		"'s'	send big data\n"
		"'l'	disp line state\n"
		"'L'	check line state before sending\n"
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


int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(269);
#endif

	bool verbose = false;
	wchar_t *ini_base = L"ttcomtester.ini";
	int arg_rts = RTS_CONTROL_DISABLE;
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
		int c = getopt_long_w(argc, argv, L"vhi:r:", long_options, NULL);
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
				arg_rts = RTS_CONTROL_DISABLE;
			} else if (wcscmp(optarg_w, L"on") == 0) {
				arg_rts = RTS_CONTROL_ENABLE;
			} else if (wcscmp(optarg_w, L"hs") == 0) {
				arg_rts = RTS_CONTROL_HANDSHAKE;
			} else if (wcscmp(optarg_w, L"on") == 0) {
				arg_rts = RTS_CONTROL_TOGGLE;
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

	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));	// 100% build‚µ‚Ä‚­‚ê‚È‚¢‚æ‚¤‚¾
	dcb.DCBlength = sizeof(dcb);
	BOOL r = BuildCommDCBW(com_param, &dcb);
	dcb.fRtsControl = arg_rts;
	if (r == FALSE) {
		DWORD e = GetLastError();
		wchar_t b[128];
		swprintf_s(b, _countof(b), L"BuildCommDCBW('%s')", com_param);
		DispErrorStr(b, e);
		goto finish;
	}
	if (verbose) {
		printf("param='%ls'\n", com_param);
		dumpDCB(&dcb);
	}
	ope->ctrl(dev, SET_COM_DCB, &dcb);

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
	enum {
		STATE_CLOSE,
		STATE_OPEN,
		STATE_ERROR,
	} state = STATE_CLOSE;
	while (!quit_flag) {
		if (_kbhit() == 0) {
			// ƒL[‚ª‰Ÿ‚³‚ê‚Ä‚¢‚È‚¢
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
					}
					else {
						DispErrorStr(L"open()", e);
					}
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
					size_t sended_len;
					DWORD e = ope->write(dev, send_data, send_len, &sended_len);
					if (e == ERROR_SUCCESS) {
						printf("sent\n");
					}
					else if (e == ERROR_IO_PENDING) {
						size_t sended_len_total = sended_len;
						while(1) {
							r = ope->wait_write(dev, &sended_len);
#if 0
							if (r == ERROR_SUCCESS) {
								printf("sent\n");
							}
#endif
							sended_len_total += sended_len;
							printf("send size %zu(%zu)/%zu\n", sended_len_total, sended_len, send_len);
							Sleep(100);
						}
					}
					else {
						DispErrorStr(L"open()", e);
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
				case ':': {
					printf("\nsend mode\n");
					command_mode = false;
					break;
				}
				default:
					printf("unknown command '%c'\n", c);
					key_usage();
					break;
				}
			}
			else {
				if (c == ':') {
					printf("\ncommand mode\n");
					command_mode = true;
				}
				else {
					if (state == STATE_OPEN) {
						char send_text[2];
						size_t sended_len;
						send_text[0] = (char)c;
						DWORD e = ope->write(dev, send_text, 1, &sended_len);
						if (e == ERROR_SUCCESS) {
							printf("send %02x, %zu byte\n", c, sended_len);
						}
						else {
							DispErrorStr(L"write() error", e);
							state = STATE_ERROR;
						}
					}
				}
			}
		}

		uint8_t buf[1024];
		size_t len = 0;
		DWORD e;
		if (state != STATE_OPEN) {
			Sleep(1);
		}
		else if (receive_pending == false) {
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
				// ‚Ü‚¾‘Ò‚¿ó‘Ô
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
