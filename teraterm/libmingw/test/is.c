
#include <windows.h>
#include "ttlib.h"

#if (_MSC_VER < 1800)
BOOL vercmp(
	DWORD cond_val,
	DWORD act_val,
	DWORD dwTypeMask)
{
	switch (dwTypeMask) {
	case VER_EQUAL:
		if (act_val == cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER:
		if (act_val > cond_val) {
			return TRUE;
		}
		break;
	case VER_GREATER_EQUAL:
		if (act_val >= cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS:
		if (act_val < cond_val) {
			return TRUE;
		}
		break;
	case VER_LESS_EQUAL:
		if (act_val <= cond_val) {
			return TRUE;
		}
		break;
	}
	return FALSE;
}

#endif

BOOL _myVerifyVersionInfo(
	LPOSVERSIONINFOEX lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
	OSVERSIONINFO osvi;
	WORD cond;
	BOOL ret, check_next;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	if (dwTypeMask & VER_BUILDNUMBER) {
		cond = (WORD)((dwlConditionMask >> (2*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwBuildNumber, osvi.dwBuildNumber, cond)) {
			return FALSE;
		}
	}
	if (dwTypeMask & VER_PLATFORMID) {
		cond = (WORD)((dwlConditionMask >> (3*3)) & 0x07);
		if (!vercmp(lpVersionInformation->dwPlatformId, osvi.dwPlatformId, cond)) {
			return FALSE;
		}
	}
	ret = TRUE;
	if (dwTypeMask & (VER_MAJORVERSION | VER_MINORVERSION)) {
		check_next = TRUE;
		if (dwTypeMask & VER_MAJORVERSION) {
			cond = (WORD)((dwlConditionMask >> (1*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, cond);
				// ret: result of major version
				if (!vercmp(lpVersionInformation->dwMajorVersion, osvi.dwMajorVersion, VER_EQUAL)) {
					// !vercmp(...: result of GRATOR/LESS than (not "GRATOR/LESS than or equal to") of major version
					// e.g.
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER_EQUAL  ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:5.0 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.1 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:5.2 cond:VER_GREATER        ret:FALSE !vercmp(...:FALSE  must check minor
					//   lpvi:5.1 actual:6.0 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.1 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					//   lpvi:5.1 actual:6.2 cond:VER_GREATER        ret:TRUE  !vercmp(...:TRUE   must not check minor
					check_next = FALSE;
				}
			}
		}
		if (check_next && (dwTypeMask & VER_MINORVERSION)) {
			cond = (WORD)((dwlConditionMask >> (0*3)) & 0x07);
			if (cond == VER_EQUAL) {
				if (!vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond)) {
					return FALSE;
				}
			}
			else {
				ret = vercmp(lpVersionInformation->dwMinorVersion, osvi.dwMinorVersion, cond);
			}
		}
	}
	return ret;
}

ULONGLONG _myVerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	ULONGLONG result, mask;
	BYTE op = dwConditionMask & 0x07;

	switch (dwTypeBitMask) {
		case VER_MINORVERSION:
			mask = 0x07 << (0 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (0 * 3);
			break;
		case VER_MAJORVERSION:
			mask = 0x07 << (1 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (1 * 3);
			break;
		case VER_BUILDNUMBER:
			mask = 0x07 << (2 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (2 * 3);
			break;
		case VER_PLATFORMID:
			mask = 0x07 << (3 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (3 * 3);
			break;
		case VER_SERVICEPACKMINOR:
			mask = 0x07 << (4 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (4 * 3);
			break;
		case VER_SERVICEPACKMAJOR:
			mask = 0x07 << (5 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (5 * 3);
			break;
		case VER_SUITENAME:
			mask = 0x07 << (6 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (6 * 3);
			break;
		case VER_PRODUCT_TYPE:
			mask = 0x07 << (7 * 3);
			result = dwlConditionMask & ~mask;
			result |= op << (7 * 3);
			break;
		default:
			result = 0;
			break;
	}

	return result;
}

ULONGLONG myVerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
	typedef ULONGLONG(WINAPI *func)(ULONGLONG, DWORD, BYTE);
	static HMODULE hmodKernel32 = NULL;
	static func pVerSetConditionMask = NULL;
	char kernel32_dll[MAX_PATH];

	GetSystemDirectory(kernel32_dll, sizeof(kernel32_dll));
	strncat_s(kernel32_dll, sizeof(kernel32_dll), "\\kernel32.dll", _TRUNCATE);
	if (hmodKernel32 == NULL) {
		hmodKernel32 = LoadLibrary(kernel32_dll);
		if (hmodKernel32 != NULL) {
			pVerSetConditionMask = (func)GetProcAddress(hmodKernel32, "VerSetConditionMask");
		}
	}

	if (pVerSetConditionMask == NULL) {
		return _myVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
	}

	return pVerSetConditionMask(dwlConditionMask, dwTypeBitMask, dwConditionMask);
}

BOOL myVerifyVersionInfo(
	LPOSVERSIONINFOEX lpVersionInformation,
	DWORD dwTypeMask,
	DWORDLONG dwlConditionMask)
{
#if (_MSC_VER >= 1800)
	return VerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask);
#else
	return _myVerifyVersionInfo(lpVersionInformation, dwTypeMask, dwlConditionMask);
#endif
}

BOOL IsWindowsNTKernel()
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
	dwlConditionMask = myVerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	ret = myVerifyVersionInfo(&osvi, VER_PLATFORMID, dwlConditionMask);
	return (ret);
}
