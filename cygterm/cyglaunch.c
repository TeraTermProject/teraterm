//
// Cygterm launcher
//
// Copyright (c)2007-2008 TeraTerm Project
//   http://ttssh2.sourceforge.jp/
//
// [How to compile]
// Cygwin:
//  # cc -mno-cygwin -mwindows -o cyglaunch cyglaunch.c
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


#define Section "Tera Term"
char *FName = ".\\TERATERM.INI";


//
// Connect to local cygwin
//
void OnCygwinConnection(char *CygwinDirectory, char *cmdline)
{
	char file[MAX_PATH];
	char c, *envptr, *envbuff;
	int envbufflen;
	char *exename = "cygterm.exe";
	char cmd[1024];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	envptr = getenv("PATH");
	if (strstr(envptr, "cygwin\\bin") != NULL) {
		goto found_path;
	}

	_snprintf(file, MAX_PATH, "%s\\bin", CygwinDirectory);
	if (GetFileAttributes(file) == -1) { // open error
		for (c = 'C' ; c <= 'Z' ; c++) {
			file[0] = c;
			if (GetFileAttributes(file) != -1) { // open success
				goto found_dll;
			}
		}
		MessageBox(NULL, "Can't find Cygwin directory.", "ERROR", MB_OK | MB_ICONWARNING);
		return;
	}
found_dll:;
	if (envptr != NULL) {
		envbufflen = strlen(file) + strlen(envptr) + 7; // "PATH="(5) + ";"(1) + NUL(1)
		if ((envbuff=malloc(envbufflen)) == NULL) {
			MessageBox(NULL, "Can't allocate memory.", "ERROR", MB_OK | MB_ICONWARNING);
			return;
		}
		_snprintf(envbuff, envbufflen, "PATH=%s;%s", file, envptr);
	} else {
		envbufflen = strlen(file) + strlen(envptr) + 6; // "PATH="(5) + NUL(1)
		if ((envbuff=malloc(envbufflen)) == NULL) {
			MessageBox(NULL, "Can't allocate memory.", "ERROR", MB_OK | MB_ICONWARNING);
			return;
		}
		_snprintf(envbuff, envbufflen, "PATH=%s", file);
	}
	_putenv(envbuff);
	if (envbuff) {
		free(envbuff);
		envbuff = NULL;
	}

found_path:;
	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));

	strcpy(cmd, exename);
	strcat(cmd, " ");
	strncat(cmd, cmdline, sizeof(cmd)-strlen(cmd)-1);
//printf("%s", cmd);
//MessageBox(NULL, cmd, "", MB_OK);
	if (CreateProcess(
			NULL,
			cmd,
			NULL, NULL, FALSE, 0,
			NULL, NULL,
			&si, &pi) == 0) {
		MessageBox(NULL, "Can't execute Cygterm.", "ERROR", MB_OK | MB_ICONWARNING);
	}
}


int main(int argc, char** argv)
{
	char Temp[256], Cmdline[256];
	int i;
	
	// Cygwin install path
 	GetPrivateProfileString(Section,"CygwinDirectory","c:\\cygwin",
			  Temp,sizeof(Temp),FName);
	
	//printf("%s %d\n", Temp, GetLastError());
	
	Cmdline[0] = 0;
	for (i=1; i<argc; i++) {
		strncat(Cmdline, argv[i], sizeof(Cmdline)-strlen(Cmdline)-1);
		strncat(Cmdline, " ", sizeof(Cmdline)-strlen(Cmdline)-1);
	}
	//printf("%s\n", Cmdline);
	
	OnCygwinConnection(Temp, Cmdline);
	
	return 0;
}
