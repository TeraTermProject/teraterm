
#include <stdio.h>
#include <locale.h>
#include <windows.h>

#include "asprintf.h"
#include "codeconv.h"

#include "getopt.h"

#define IPC_BROADCAST_COMMAND 1		// ‘S’[––‚Ö‘—M
#define IPC_MULTICAST_COMMAND 2		// ”CˆÓ‚Ì’[––ŒQ‚Ö‘—M

BOOL IsUnicharSupport(HWND hwnd)
{
	LRESULT r = SendMessage(hwnd, WM_UNICHAR, UNICODE_NOCHAR, 0);
	return (BOOL)r;
}

static COPYDATASTRUCT *BuildBroadcastCDS(const void *buf, size_t buflen)
{
	COPYDATASTRUCT *cds = (COPYDATASTRUCT *)malloc(sizeof(COPYDATASTRUCT));

	cds->dwData = IPC_BROADCAST_COMMAND;
	cds->cbData = (DWORD)buflen;	// '\0' ‚ÍŠÜ‚Ü‚È‚¢
	cds->lpData = (void *)buf;

	return cds;
}

static COPYDATASTRUCT *BuildBroadcastCDS(const wchar_t *buf)
{
	size_t buflen = wcslen(buf) * sizeof(wchar_t);	// '\0' ‚ÍŠÜ‚Ü‚È‚¢
	return BuildBroadcastCDS(buf, buflen);
}

static COPYDATASTRUCT *BuildBroadcastCDS(const char *buf)
{
	size_t buflen = strlen(buf);	// '\0' ‚ÍŠÜ‚Ü‚È‚¢
	return BuildBroadcastCDS(buf, buflen);
}

static COPYDATASTRUCT *BuildMuitlcastCDS(const void *name, size_t namelen, const void *buf, size_t buflen)
{
	size_t data_len = namelen + buflen;
	COPYDATASTRUCT *cds = (COPYDATASTRUCT *)malloc(sizeof(COPYDATASTRUCT));
	unsigned char *lpData = (unsigned char *)malloc(data_len);
	memcpy(lpData, name, namelen);
	memcpy(lpData + namelen, buf, buflen);

	cds->dwData = IPC_MULTICAST_COMMAND;
	cds->cbData = (DWORD)buflen;
	cds->lpData = (void *)lpData;

	return cds;
}

static COPYDATASTRUCT *BuildMuitlcastCDS(const wchar_t *name, const wchar_t *buf)
{
	size_t namelen = (wcslen(name) + 1) * sizeof(wchar_t);	// '\0' ŠÜ‚Þ
	size_t buflen = wcslen(buf) * sizeof(wchar_t);	// '\0' ‚ÍŠÜ‚Ü‚È‚¢
	return BuildMuitlcastCDS(name, namelen, buf, buflen);
}

static COPYDATASTRUCT *BuildMuitlcastCDS(const char *name, const char *buf)
{
	size_t namelen = strlen(name) + 1;	// '\0' ŠÜ‚Þ
	size_t buflen = strlen(buf);	// '\0' ‚ÍŠÜ‚Ü‚È‚¢
	return BuildMuitlcastCDS(name, namelen, buf, buflen);
}

typedef struct {
	wchar_t *strW;
	char *strA;
	COPYDATASTRUCT *csdW;
	COPYDATASTRUCT *csdA;
	BOOL multicast;
	const wchar_t *nameW;
	char *nameA;
} ewdata_t;

BOOL CALLBACK EnumWindowsProc(HWND hWnd , LPARAM lp)
{
	char class_name[32];
	GetClassNameA(hWnd, class_name, _countof(class_name));
	if (strcmp(class_name, "VTWin32") != 0) {
		return TRUE;
	}

	ewdata_t *ew_data = (ewdata_t *)lp;
	COPYDATASTRUCT *cds;
	if (IsUnicharSupport(hWnd)) {
		// Unicode‚É‘Î‰ž Tera Term 5Œn
		if (ew_data->multicast) {
			if (ew_data->csdW == NULL) {
				ew_data->csdW = BuildMuitlcastCDS(ew_data->nameW ,ew_data->strW);
			}
		}
		else {
			if (ew_data->csdW == NULL) {
				ew_data->csdW = BuildBroadcastCDS(ew_data->strW);
			}
		}
		cds = ew_data->csdW;
	}
	else {
		// Unicode‚É”ñ‘Î‰ž Tera Term 5ˆÈ‘O
		if (ew_data->multicast) {
			if (ew_data->csdA == NULL) {
				ew_data->nameA = ToCharW(ew_data->nameW);
				ew_data->strA = ToCharW(ew_data->strW);
				ew_data->csdA = BuildMuitlcastCDS(ew_data->nameA ,ew_data->strA);
			}
		}
		else {
			if (ew_data->csdA == NULL) {
				ew_data->strA = ToCharW(ew_data->strW);
				ew_data->csdA = BuildBroadcastCDS(ew_data->strA);
			}
		}
		cds = ew_data->csdA;
	}
	SendMessage(hWnd, WM_COPYDATA, (WPARAM)0, (LPARAM)cds);

	return TRUE;
}

static void usage()
{
	printf(
		"ttbroadcast [option] \"broadcast string\"\n"
		"  -n     do not output the trailing newline\n"
		"  -m [name], --multicast [name]\n"
		"         multicast\n"
		);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");

	int version = 0;
	int no_newline = 0;
	int multicast = 0;
	const wchar_t *multicast_name;
	static const struct option_w long_options[] = {
		{L"help", no_argument, NULL, L'h'},
		{L"multicast", required_argument, NULL, L'm'},
		{}
	};

	opterr = 0;
    while(1) {
		int c = getopt_long_w(argc, argv, L"hnm:", long_options, NULL);
        if(c == -1) break;

        switch (c)
        {
		case L'h':
		case L'?':
			usage();
			return 0;
			break;
		case L'n':
			no_newline = 1;
			break;
		case L'm':
			multicast = 1;
			multicast_name = optarg_w;
			break;
		default:
			printf("usage 3\n");
			break;
		}
    }

	if (version) {
		printf("version\n");
		return 0;
	}

	if (optind + 1 != argc) {
		usage();
		return 0;
	}

	wprintf(L"'%s'\n", argv[optind]);

	wchar_t *strW;
	if (no_newline) {
		strW = _wcsdup(argv[optind]);
	}
	else {
		aswprintf(&strW, L"%s\n", argv[optind]);
	}

	ewdata_t ewdata = {};
	ewdata.strW = strW;
	if (multicast) {
		ewdata.multicast = 1;
		ewdata.nameW = multicast_name;
	}
	EnumWindows(EnumWindowsProc , (LPARAM)&ewdata);
	if (multicast) {
		if (ewdata.csdW != NULL) {
			free(ewdata.csdW->lpData);
		}
		if (ewdata.csdA != NULL) {
			free(ewdata.csdA->lpData);
		}
	}
	free(ewdata.strW);
	free(ewdata.csdW);
	free(ewdata.strA);
	free(ewdata.csdA);
	free(ewdata.nameA);
	return 0;
}

