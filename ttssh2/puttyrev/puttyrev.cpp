/*
 * (C) 2004-2017 TeraTerm Project
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
#include <stdio.h>
#include "puttyrev.h"

void write_putty_version(char *path)
{
	BOOL ret;
	FILE *fp;
	char *keywords[] = {
		//"AppVerName",
		"AppVersion",
		"VersionInfoTextVersion",
		NULL,
	};
	int i;
	char filename[MAX_PATH * 2], buf[64];
	char revision[64] = {0};
	char header_line[64]= {0}, *p;

	// PuTTYのバージョンを取得する。
	_snprintf_s(filename, sizeof(filename), _TRUNCATE,
	            "%s%s", path, "\\libs\\putty\\windows\\putty.iss");

	if (fopen_s(&fp, filename, "r") != 0) {
		goto write;
	}

	while(!feof(fp)){
		char tmp[64];
		fgets(buf, sizeof(buf), fp);
		for (i = 0 ; keywords[i] ; i++) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
			            "%s%s", keywords[i], "=%[^\n]s");
			ret = sscanf_s(buf, tmp, revision, sizeof(revision));
			if (ret != 1) 
				continue;
			printf("%s\n", revision);
			goto close;
		}
	}

close:
	fclose(fp);

	_snprintf_s(filename, sizeof(filename), _TRUNCATE,
	            "%s%s", path, "\\ttssh2\\ttxssh\\puttyversion.h");

	// バージョンをチェックし、変更がなければ抜ける
	if (fopen_s(&fp, filename, "r") != 0) {
		goto write;
	}

	memset(header_line, 0, sizeof(header_line));
	if (fread(header_line, sizeof(char), sizeof(header_line)-1, fp) == 0) {
		fclose(fp);
		goto write;
	}

	if ( (p = strchr(header_line, '"')) == NULL ) {
		fclose(fp);
		goto write;
	}

	p++;
	if (strncmp(p, revision, strlen(p)-2) == 0) {
		fclose(fp);
		goto end;
	}

	fclose(fp);

write:
	_snprintf_s(filename, sizeof(filename), _TRUNCATE,
	            "%s%s", path, "\\ttssh2\\ttxssh\\puttyversion.h");

	// バージョンをヘッダに書き込む。
	if (fopen_s(&fp, filename, "w+") != 0) {
		goto end;
	}

	if (revision[0] != '\0') {
		fprintf(fp, "#define PUTTYVERSION \"%s\"\n", revision);
	}
	else {
		fprintf(fp, "#undef PUTTYVERSION\n");
	}

	fclose(fp);

end:;
}

int main(int argc, char* argv[])
{
	int nRetCode = 0;
	char path[MAX_PATH * 2];
	int i, len;

	GetModuleFileName(::GetModuleHandle(NULL), path, sizeof(path));
	len = (int)strlen(path);
	for (i=len; i>=0; i--) {
		if (path[i] == '\\') {
			break;
		}
		path[i] = '\0';
	}
	SetCurrentDirectory(path); // teraterm\debug or teraterm\release
	SetCurrentDirectory("..\\..\\..\\"); // top of source tree
	GetCurrentDirectory(sizeof(path), path);

	write_putty_version(path);

	return nRetCode;
}
