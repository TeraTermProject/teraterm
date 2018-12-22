#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

HANDLE win16_lcreat(const char *FileName, int iAttribute);
HANDLE win16_lopen(const char *FileName, int iReadWrite);
void win16_lclose(HANDLE hFile);
UINT win16_lread(HANDLE hFile, void *buf, UINT uBytes);
UINT win16_lwrite(HANDLE hFile, const void *buf, UINT length);
LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin);

#ifdef __cplusplus
}
#endif

#define _lcreat(p1,p2)		win16_lcreat(p1,p2)
#define	_lopen(p1,p2)		win16_lopen(p1,p2)
#define _lclose(p1)			win16_lclose(p1)
#define _lread(p1,p2,p3)	win16_lread(p1,p2,p3)
#define _lwrite(p1,p2,p3)	win16_lwrite(p1,p2,p3)
#define	_llseek(p1,p2,p3)	win16_llseek(p1,p2,p3)
