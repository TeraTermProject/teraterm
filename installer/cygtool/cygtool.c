#pragma comment(lib, "version.lib")

#include <windows.h>
#include <stdio.h>

#include "compat_w95.h"

int __stdcall FindCygwinPath(char *CygwinDirectory, char *Dir, int Dirlen)
{
	char file[MAX_PATH], *filename;
	char c;

	if (strlen(CygwinDirectory) > 0) {
		if (SearchPath(CygwinDirectory, "bin\\cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
#ifdef EXE
			printf("  %s from CygwinDirectory\n", file);
#endif
			goto found_dll;
		}
	}

	if (SearchPath(NULL, "cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
#ifdef EXE
		printf("  %s from PATH\n", file);
		goto found_dll;
#endif
	}

	for (c = 'C' ; c <= 'Z' ; c++) {
		char tmp[MAX_PATH];
		sprintf(tmp, "%c:\\cygwin\\bin;%c:\\cygwin64\\bin", c, c);
		if (SearchPath(tmp, "cygwin1", ".dll", sizeof(file), file, &filename) > 0) {
#ifdef EXE
			printf("  %s from %c:\\\n", file, c);
#endif
			goto found_dll;
		}
	}

	return 0;

found_dll:;
	memset(Dir, '\0', Dirlen);
	if (Dirlen <= strlen(file) - 16) {
		return 0;
	}
	memcpy(Dir, file, strlen(file) - 16); // delete "\\bin\\cygwin1.dll"
	return 1;
}

int __stdcall CygwinMachine(char *file)
{
	FILE *fp;
	unsigned char buf[4];
	long e_lfanew;
	WORD Machine;

	if ((fp = fopen(file, "rb")) == NULL) {
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}

	// IMAGE_DOS_HEADER
	if (fseek(fp, 0x3c, SEEK_SET) != 0) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	if (fread(buf, sizeof(char), 4, fp) < 4) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	e_lfanew = buf[0] + (buf[1] << 8) + (buf[1] << 16) + (buf[1] << 24);
#ifdef EXE
	printf("  e_lfanew => x%08x\n", e_lfanew);
#endif

	// IMAGE_NT_HEADERS32
	//   DWORD Signature;
	//   IMAGE_FILE_HEADER FileHeader;
	if (fseek(fp, e_lfanew + 4, SEEK_SET) != 0) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	if (fread(buf, sizeof(char), 2, fp) < 2) {
		fclose(fp);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}
	Machine = buf[0] + (buf[1] << 8);

	fclose(fp);

	return Machine;
}

int __stdcall CygwinVersion(char *dll, int *major, int *minor)
{
	DWORD dwSize;
	DWORD dwHandle;
	DWORD dwLen;
	LPVOID lpBuf;
	UINT uLen;
	VS_FIXEDFILEINFO *pFileInfo;
	
	dwSize = GetFileVersionInfoSize(dll, &dwHandle);
	if (dwSize == 0) {
		return 0;
	}
	
	lpBuf = malloc(dwSize);
	if (!GetFileVersionInfo(dll, dwHandle, dwSize, lpBuf)) {
		free(lpBuf);
		return 0;
	}
	
	if (!VerQueryValue(lpBuf, "\\", (LPVOID*)&pFileInfo, &uLen)) {
		free(lpBuf);
		return 0;
	}
	
	*major = HIWORD(pFileInfo->dwFileVersionMS);
	*minor = LOWORD(pFileInfo->dwFileVersionMS);
	
	free(lpBuf);
	
	return 1;
}

#ifdef EXE
int main(void)
{
	char file[MAX_PATH];
	char version[MAX_PATH];
	int file_len = sizeof(file);
	int version_major, version_minor;
	int res;
	
	printf("FindCygwinPath()\n");
	res = FindCygwinPath("", file, file_len);
	printf("  result => %d\n", res);
	if (!res) {
		printf("\n");
		return -1;
	}
	printf("  Cygwin directory => %s\n", file);
	printf("\n");
	
	printf("CygwinMachine()\n");
	strncat_s(file, sizeof(file), "\\bin\\cygwin1.dll", _TRUNCATE);
	printf("  Cygwin DLL => %s\n", file);
	res = CygwinMachine(file);
	printf("  Machine => x%04x", res);
	switch (res) {
		case IMAGE_FILE_MACHINE_I386:
			printf(" = %s\n", "IMAGE_FILE_MACHINE_I386");
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			printf(" = %s\n", "IMAGE_FILE_MACHINE_AMD64");
			break;
		default:
			printf("\n");
			return -1;
			break;
	}
	printf("\n");
	
	printf("CygwinVersion()\n");
	printf("  Cygwin DLL => %s\n", file);
	res = CygwinVersion(file, &version_major, &version_minor);
	printf("  result => %d\n", res);
	if (!res) {
	printf("\n");
		return -1;
	}
	printf("  version_major => %d\n", version_major);
	printf("  version_minor => %d\n", version_minor);
	printf("\n");
	
	return 0;
}
#else
BOOL WINAPI DllMain(HANDLE hInstance,
		    ULONG ul_reason_for_call,
		    LPVOID lpReserved)
{
  switch( ul_reason_for_call ) {
    case DLL_THREAD_ATTACH:
      /* do thread initialization */
      break;
    case DLL_THREAD_DETACH:
      /* do thread cleanup */
      break;
    case DLL_PROCESS_ATTACH:
      /* do process initialization */
      DoCover_IsDebuggerPresent();
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
#endif
