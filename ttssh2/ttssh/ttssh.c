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
#include <tchar.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance,
                   LPSTR cmdLine, int show) {
	TCHAR buf[2048];
	int i, filename_index = 0;
	STARTUPINFO startup_info;
	PROCESS_INFORMATION process_info;

	SetEnvironmentVariable(_TEXT("TERATERM_EXTENSIONS"), _TEXT("1"));

	GetModuleFileName(NULL, buf, sizeof(buf));
	for (i = 0; buf[i] != 0; i++) {
		if (buf[i] ==  _TEXT('\\') || buf[i] == _TEXT('/') || buf[i] == _TEXT(':')) {
			filename_index = i + 1;
		}
	}
	_tcsncpy(buf + filename_index, _TEXT("ttermpro.exe"), sizeof(buf) - filename_index);

	GetStartupInfo(&startup_info);
	CreateProcess(buf, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL,
	&startup_info, &process_info);
	return 0;
}
