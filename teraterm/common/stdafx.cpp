#include "stdafx.h"


// Visual Studio が MS11-025 適用済みで MFC がスタティックリンクの場合
// http://club.pep.ne.jp/~hiroki.o/visualcpp/ms11-025_problem.htm
// http://tedwvc.wordpress.com/2011/04/16/fixing-problems-with-findactctxsectionstring-in-mfc-security-updates/

#undef FindActCtxSectionString
#define FindActCtxSectionString MyFindActCtxSectionString

#ifdef _UNICODE
#define _FINDACTCTXSECTIONSTRING "FindActCtxSectionStringW"
#else
#define _FINDACTCTXSECTIONSTRING "FindActCtxSectionStringA"
#endif

typedef BOOL (WINAPI * PFN_FINDAC)(DWORD dwFlags, const GUID *lpExtensionGuid,ULONG ulSectionId,LPCTSTR lpStringToFind,PACTCTX_SECTION_KEYED_DATA ReturnedData);

BOOL WINAPI MyFindActCtxSectionString(
     DWORD dwFlags,
     const GUID *lpExtensionGuid,
     ULONG ulSectionId,
     LPCTSTR lpStringToFind,
     PACTCTX_SECTION_KEYED_DATA ReturnedData)
{
	// Bug #1 - Windows 2000
	PFN_FINDAC pfnFindActCtxSectionString =NULL;
	{
		HINSTANCE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (hKernel32 == NULL)
		{
			return FALSE;
		}

		pfnFindActCtxSectionString = (PFN_FINDAC) GetProcAddress(hKernel32, _FINDACTCTXSECTIONSTRING);

		if (pfnFindActCtxSectionString == NULL)
		{
			/* pre-fusion OS, so no more checking.*/
			return FALSE;
		}
	}

	ReturnedData->cbSize = sizeof(ACTCTX_SECTION_KEYED_DATA); // Bug #2 - missing cbSize initializer
	return pfnFindActCtxSectionString(/* dwFlags */ 0,  // Bug #3 memory leak - pass in zero as return handle not freed
		lpExtensionGuid, ulSectionId, lpStringToFind, ReturnedData);
}
