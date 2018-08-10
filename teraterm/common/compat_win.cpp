
#include "compat_win.h"

TSetThreadDpiAwarenessContext PSetThreadDpiAwarenessContext;

void WinCompatInit()
{
	static bool done = false;
	if (done) return;
	done = true;

	char user32_dll[MAX_PATH];
	GetSystemDirectory(user32_dll, sizeof(user32_dll));
	strncat_s(user32_dll, sizeof(user32_dll), "\\user32.dll", _TRUNCATE);

	HMODULE dll_handle = LoadLibrary(user32_dll);
	PSetThreadDpiAwarenessContext =
		(TSetThreadDpiAwarenessContext)
		GetProcAddress(dll_handle, "SetThreadDpiAwarenessContext");
}
