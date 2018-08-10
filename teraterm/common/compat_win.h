
#include <windows.h>

extern "C" {

#if !defined(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE          ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE     ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2  ((DPI_AWARENESS_CONTEXT)-4)
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED                   0x02E0
#endif

typedef DPI_AWARENESS_CONTEXT (WINAPI *TSetThreadDpiAwarenessContext)
	(DPI_AWARENESS_CONTEXT dpiContext);
extern TSetThreadDpiAwarenessContext PSetThreadDpiAwarenessContext;

void WinCompatInit();


}
