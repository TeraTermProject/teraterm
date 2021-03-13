/*
 * (C) 2021- TeraTerm Project
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
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern "C" {
int WINAPI DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
}

#include "comportinfo.h"

#include "getopt.h"

static bool show_all_devices = false;

#define MAXCOMPORT 4096

void DetectComPortByQueryDosDevice()
{
	size_t buf_size = 65535;
	char   *devicesBuff = (char *)malloc(buf_size);

	int r = QueryDosDeviceA(NULL, devicesBuff, (DWORD)buf_size);
	if (r == 0) {
		int error = GetLastError();
		printf("QueryDosDeviceA() error %d\n", error);
		return;
	}

	int n = 0;
	char *p = devicesBuff;
	while (*p != '\0') {
		bool show = false;
		if (show_all_devices ||
			(strncmp(p, "COM", 3) == 0 && p[3] != '\0')) {
			show = true;
		}
		if (show) {
			printf("%d: %s\n", n, p);
		}
		n++;
		p += strlen(p)+1;
	}
	free(devicesBuff);
}

void lscom_DetectComPorts()
{
	static WORD ComPortTable[MAXCOMPORT];  // 使用可能なCOMポート番号
	static char *ComPortDesc[MAXCOMPORT];  // COMポートの詳細情報
	int com_count = DetectComPorts(ComPortTable, MAXCOMPORT, ComPortDesc);

	for(int i = 0; i < com_count; i++) {
		printf("com%d: %s\n", ComPortTable[i], ComPortDesc[i]);
	}
}

void lscom_fopen()
{
//	int maxcomport = MAXCOMPORT;
	int maxcomport = 20;
	for (int i = 1; i <= maxcomport; i++) {
		char buf[12];  // \\.\COMxxxx + NULL
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "\\\\.\\COM%d", i);
		FILE *fp = fopen(buf, "r");
		printf("%s %d\n", buf, fp == NULL);
		if (fp != NULL) {
			fclose(fp);
		}
	}
}

void lscom()
{
	int comPortCount;
	ComPortInfo_t *infos = ComPortInfoGet(&comPortCount, NULL);
	printf("comport count %d\n", comPortCount);
	for (int i = 0; i < comPortCount; i++) {
		ComPortInfo_t *p = &infos[i];
		wchar_t *port = p->port_name;
		wprintf(
			L"========\n"
			L"%s:\n",
			port);
		wchar_t *friendly = p->friendly_name;
		wchar_t *desc = p->property;
		if (friendly != NULL) {
			wprintf(
				L"-----\n"
				L"FRIENDLY:\n"
				L"%s\n",
				friendly);
		}
		if (desc != NULL) {
			wprintf(
				L"-----\n"
				L"DESC:\n"
				L"%s\n",
				desc);
		}
	}
	ComPortInfoFree(infos, comPortCount);

	printf("end\n");
}

static void usage()
{
	printf(
		"lscom [option]\n"
		"  list com devices\n"
		"option\n"
		"  -q, querydosdevice   list com devices by QueryDosDevice() API\n"
		"  -a, all              show all devices with -q option\n"
		"  -d, detectcomports   list by DetectComPorts()\n"
		"  -f, fopen            list by fopen()\n"
		);
}

int wmain(int argc, wchar_t *argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
	setlocale(LC_ALL, "");

	int no_newline = 0;
	int multicast = 0;
	bool querydosdevice = false;
	bool flag_DetectComPorts = false;
	bool flag_fopen = false;
	static const struct option_w long_options[] = {
		{L"help", no_argument, NULL, L'h'},
		{L"querydosdevice", no_argument, NULL, L'q'},
		{L"all", no_argument, NULL, L'a'},
		{L"detectcomports", no_argument, NULL, L'd'},
		{L"fopen", no_argument, NULL, L'f'},
		{}
	};

	opterr = 0;
    while(1) {
		int c = getopt_long_w(argc, argv, L"hqadf", long_options, NULL);
        if(c == -1) break;

        switch (c)
        {
		case L'h':
		case L'?':
			usage();
			return 0;
			break;
		case L'q':
			querydosdevice = true;
			break;
		case L'a':
			show_all_devices = true;
			break;
		case L'd':
			flag_DetectComPorts = true;
			break;
		case L'f':
			flag_fopen = true;
			break;
		default:
			printf("usage\n");
			break;
		}
    }

	if (querydosdevice) {
		printf("query\n");
		DetectComPortByQueryDosDevice();
	} else if (flag_DetectComPorts) {
		printf("DetectComPorts()\n");
		lscom_DetectComPorts();
	} else if (flag_fopen) {
		printf("fopen()\n");
		lscom_fopen();
	} else {
		printf("new\n");
		lscom();
	}

	return 0;
}
