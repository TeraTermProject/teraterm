/*
 * Copyright (C) 2019- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include <devguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <assert.h>

#include "ttlib.h"
#include "codeconv.h"
#include "asprintf.h"
#include "compat_win.h"
#include "win32helper.h"

#include "comportinfo.h"

/*
 *	devpkey.h がある環境?
 *		HAS_DEVPKEY_H が define される
 */
#if	defined(_MSC_VER)
#if	(_MSC_VER > 1400)

// VS2019のとき(VS2005より大きいとしている)
#define HAS_DEVPKEY_H	1

#else // _MSC_VER > 1400

// VS2008のとき
#if defined(_INC_SDKDDKVER)

// VS2008 + SDK 7.0ではないとき(SDK 7.1のとき)
//   SDK 7.0 の場合は sdkddkver.h が include されていない
#define HAS_DEVPKEY_H	1

#endif  //  defined(_INC_SDKDDKVER)
#endif
#elif defined(__MINGW32__)

#if	__MINGW64_VERSION_MAJOR >= 8
// mingw64 8+ のとき
#define HAS_DEVPKEY_H	1
#endif

#endif  // defined(_MSC_VER)

/*
 *	devpkey.h の include
 */
#if defined(HAS_DEVPKEY_H)

#define INITGUID
#include <devpkey.h>

#else //  defined(HAS_DEVPKEY_H)

#include "devpkey_teraterm.h"

#endif //  defined(HAS_DEVPKEY_H)

#define INITGUID
#include <guiddef.h>

// 1のとき、"COM%d" のみ検出する
#define DETECT_COM_ONLY		1

/**
 *	ポート名を取得
 */
static BOOL GetComPortName(HDEVINFO hDevInfo, SP_DEVINFO_DATA *DeviceInfoData, wchar_t **str)
{
	HKEY hKey = SetupDiOpenDevRegKey(hDevInfo,
									 DeviceInfoData,
									 DICS_FLAG_GLOBAL,
									 0, DIREG_DEV, KEY_READ);
	if (hKey == NULL){
		// ポート名が取得できない?
		*str = NULL;
		return FALSE;
	}

	wchar_t *port_name = NULL;
	long r = hRegQueryValueExW(hKey, L"PortName", 0, NULL, (void **)&port_name, NULL);

	RegCloseKey(hKey);
	if (r == ERROR_SUCCESS) {
		*str = port_name;
		return TRUE;
	}
	else {
		free(port_name);
		*str = NULL;
		return FALSE;
	}
}

/**
 *	プロパティ文字列作成
 */
static void CreatePropertyStr(ComPortInfo_t *info)
{
	ComPortInfo_t *p = info;
	wchar_t *s = NULL;

	if (p->friendly_name != NULL) {
		awcscats(&s, L"Device Friendly Name: ", p->friendly_name, L"\r\n", NULL);
	}
	if (p->instance_id != NULL) {
		awcscats(&s, L"Device Instance ID: ", p->instance_id, L"\r\n", NULL);
	}
	if (p->manufacturer != NULL) {
		awcscats(&s, L"Device Manufacturer: ", p->manufacturer, L"\r\n", NULL);
	}
	if (p->provider_name != NULL) {
		awcscats(&s, L"Provider Name: ", p->provider_name, L"\r\n", NULL);
	}
	if (p->driverdate != NULL) {
		awcscats(&s, L"Driver Data: ", p->driverdate, L"\r\n", NULL);
	}
	if (p->driverversion != NULL) {
		awcscats(&s, L"Driver Version: ", p->driverversion, L"\r\n", NULL);
	};
	if (p->class_name!= NULL) {
		awcscats(&s, L"Class: ", p->class_name, L"\r\n", NULL);
	};
	if (s == NULL) {
		awcscats(&s, L"no infomation for ", p->port_name, NULL);
	}
	p->property = s;
}


/**
 *	情報を取得
 */
static void GetComProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA *DeviceInfoData,
						   ComPortInfo_t *info)
{
	ComPortInfo_t *p = info;
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_FriendlyName, (void **)&p->friendly_name, NULL);
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_InstanceId, (void **)&p->instance_id, NULL);
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_Manufacturer, (void **)&p->manufacturer, NULL);
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_DriverProvider, (void **)&p->provider_name, NULL);
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_DriverDate, (void **)&p->driverdate, NULL);
	hSetupDiGetDevicePropertyW(
		hDevInfo, DeviceInfoData,
		&DEVPKEY_Device_DriverVersion, (void **)&p->driverversion, NULL);
}

/* COMポート情報ソート */
static int sort_sub(const void *a_, const void *b_)
{
	const ComPortInfo_t *a = (ComPortInfo_t *)a_;
	const ComPortInfo_t *b = (ComPortInfo_t *)b_;
	BOOL a_com = (wcsncmp(a->port_name, L"COM", 3) == 0);
	BOOL b_com = (wcsncmp(b->port_name, L"COM", 3) == 0);
	if (a_com && !b_com) {
		// "COM%d"が後ろ
		return 1;
	}
	if (!a_com && b_com) {
		// "COM%d"が後ろ
		return -1;
	}
	if (a_com && b_com) {
		// 両方"COM%d"のときは、数字が大きいほうが後ろ
		return (a->port_no == b->port_no) ? 0 : (a->port_no > b->port_no) ? 1 : -1;
	}
	// アルファベット順並び
	return wcscmp(a->port_name, b->port_name);
}

/**
 *	実際にデバイスをオープンすることでcomポート検出
 */
static ComPortInfo_t *ComPortInfoGetByCreatefile(int *count)
{
	const int ComPortMax = 256;
	int comport_count = 0;
	ComPortInfo_t *comport_infos = NULL;
	for (int i = 1; i <= ComPortMax; i++) {
		char buf[12];  // \\.\COMxxxx + NULL
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "\\\\.\\COM%d", i);
		HANDLE h = CreateFileA(buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);

			ComPortInfo_t info = {};
			wchar_t com_name[12];
			_snwprintf_s(com_name, _countof(com_name), _TRUNCATE, L"COM%d", i);
			info.port_name = _wcsdup(com_name);  // COMポート名
			info.port_no = i;  // COMポート番号
			CreatePropertyStr(&info);

			comport_count++;
			comport_infos = (ComPortInfo_t *)realloc(comport_infos, sizeof(ComPortInfo_t) * comport_count);
			ComPortInfo_t *p = &comport_infos[comport_count - 1];
			*p = info;
		}
	}

	*count = comport_count;
	return comport_infos;
}

/**
 * COM0COM の class GUID
 */
DEFINE_GUID( GUID_DEVCLASS_COM0COM, 0xdf799e12L, 0x3c56, 0x421b, 0xb2, 0x98, 0xb6, 0xd3, 0x64, 0x2b, 0xc8, 0x78 );

static ComPortInfo_t *ComPortInfoGetByGetSetupAPI(int *count)
{
	int comport_count = 0;
	ComPortInfo_t *comport_infos = NULL;

	HDEVINFO hDevInfo = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	if (hDevInfo != INVALID_HANDLE_VALUE) {
		for (DWORD j = 0; ; j++) {
			SP_DEVINFO_DATA DeviceInfoData = {};
			DeviceInfoData.cbSize = sizeof (DeviceInfoData);
			if (!SetupDiEnumDeviceInfo(hDevInfo, j, &DeviceInfoData)) {
				break;
			}

			BOOL r;
			wchar_t *class_str = NULL;
			do {
				GUID class_guid;

				// class guid から選択する Windows 7 以前は失敗する
				r = hSetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_ClassGuid, (void **)&class_guid, NULL);
				if (r == TRUE) {
					if (memcmp(&class_guid, &GUID_DEVCLASS_PORTS, sizeof(GUID)) == 0) {
						// シリアルポート(class_str = "Ports"のはず)
						hSetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Class, (void **)&class_str, NULL);
						break;
					}
					if (memcmp(&class_guid, &GUID_DEVCLASS_MODEM, sizeof(GUID)) == 0) {
						// モデム(class_str = "Modem"のはず)
						hSetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Class, (void **)&class_str, NULL);
						break;
					}
					if (memcmp(&class_guid, &GUID_DEVCLASS_COM0COM, sizeof(GUID)) == 0) {
						// com0com(class_str = "CNCPorts"のはず)
						hSetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Class, (void **)&class_str, NULL);
						break;
					}
				}

				// classから決める
				wchar_t *str;
				r = hSetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Class, (void **)&str, NULL);
				if (r == TRUE) {
					if (wcsstr(str, L"Ports") != 0) {
						// "Ports" が含まれていたら シリアルポートと判定する
						class_str = str;
						break;
					} else if (wcsstr(str, L"Modem") != 0) {
						// "Modem" が含まれていたら シリアルポートと判定する
						class_str = str;
						break;
					}
					free(str);
				}
			} while(0);
			if (class_str == NULL) {
				// シリアルじゃない、次のデバイスへ
				continue;
			}

			// check status
			if (pCM_Get_DevNode_Status != NULL) {
				ULONG status  = 0;
				ULONG problem = 0;
				CONFIGRET cr = pCM_Get_DevNode_Status(&status, &problem, DeviceInfoData.DevInst, 0);
				if (cr != CR_SUCCESS) {
					free(class_str);
					continue;
				}
				if (problem != 0) {
					// 何らかの問題があった?
					free(class_str);
					continue;
				}
			}

			wchar_t *port_name;
			if (!GetComPortName(hDevInfo, &DeviceInfoData, &port_name)) {
				free(class_str);
				continue;
			}

#if DETECT_COM_ONLY
			// "COM%d" ではない場合、検出しない
			if (wcsncmp(port_name, L"COM", 3) != 0) {
				free(port_name);
				free(class_str);
				continue;
			}
#endif

			// 情報取得
			ComPortInfo_t info = {};
			info.port_name = port_name;
			info.class_name = class_str;
			GetComProperty(hDevInfo, &DeviceInfoData, &info);
			CreatePropertyStr(&info);
			if (wcsncmp(port_name, L"COM", 3) == 0) {
				info.port_no = _wtoi(port_name + 3);
			}

			ComPortInfo_t *p =
				(ComPortInfo_t *)realloc(comport_infos,
										 sizeof(ComPortInfo_t) * (comport_count + 1));
			if (p == NULL) {
				break;
			}
			comport_infos = p;
			comport_count++;
			p = &comport_infos[comport_count-1];
			*p = info;
		}
	}

	/* ポート名順に並べる */
	qsort(comport_infos, comport_count, sizeof(comport_infos[0]), sort_sub);

	*count = comport_count;
	return comport_infos;
}

/**
 *	comポートの情報を取得する
 *
 *	@param[out]	count		情報数(0のときcomポートなし)
 *	@return		情報へのポインタ(配列)、ポート番号の小さい順
 *				NULLのときcomポートなし
 *				使用後ComPortInfoFree()を呼ぶこと
 */
ComPortInfo_t *ComPortInfoGet(int *count)
{
#if defined(_MSC_VER) && _MSC_VER > 1400
	// Visual Studio 2005よりあたらしいVSでビルドすると
	// setupapi は使用できるが 9x で起動しないバイナリとなる
	const bool is_setupapi_supported = true;
#else
	// VS2005 or MinGW
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionExA(&osvi);
	bool is_setupapi_supported = true;
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
		// Windows 95
		is_setupapi_supported = false;
	}
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 4) {
		// Windows NT4.0
		is_setupapi_supported = false;
	}
#endif

	if (is_setupapi_supported) {
		return ComPortInfoGetByGetSetupAPI(count);
	}
	else {
		// setupapi の動作が今一つのOSのとき
		return ComPortInfoGetByCreatefile(count);
	}
}

/**
 *	comポートの情報をメモリを破棄する
 */
void ComPortInfoFree(ComPortInfo_t *info, int count)
{
	for (int i=0; i< count; i++) {
		ComPortInfo_t *p = &info[i];
		free(p->port_name);
		free(p->friendly_name);
		free(p->class_name);
		free(p->instance_id);
		free(p->manufacturer);
		free(p->provider_name);
		free(p->driverdate);
		free(p->driverversion);
		free(p->property);
	}
	free(info);
}
