
#include <stdlib.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL GetContent(const wchar_t *url, const wchar_t *agent, void **ptr, size_t *size);

#ifdef __cplusplus
}
#endif
