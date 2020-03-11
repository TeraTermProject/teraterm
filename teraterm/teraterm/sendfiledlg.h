
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// in
	const char* UILanguageFile;
	const char *filesend_filter;
	// out
	wchar_t* filename;		// IDOK時、選択ファイル名が返る,使用後free()すること
	BOOL binary;			// TRUE/FALSE = バイナリ/テキスト
	int delay_type;
	DWORD delay_tick;
	size_t send_size;
	// work
	WORD MsgDlgHelp;
} sendfiledlgdata;

INT_PTR sendfiledlg(HINSTANCE hInstance, HWND hWndParent, sendfiledlgdata *data);

#ifdef __cplusplus
}
#endif

