#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

// callback
typedef int(__stdcall *SUSIE_PROGRESS)(int nNum, int nDenom, LONG_PTR lData);

__declspec(dllexport) int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen);
__declspec(dllexport) int __stdcall IsSupported(LPCSTR filename, void *dw);
__declspec(dllexport) int __stdcall GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo,
											   HANDLE *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData);

#ifdef __cplusplus
}
#endif
