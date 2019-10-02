/*
 * Copyright (C) 2019 TeraTerm Project
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

// for comportinfo_test
//#define TEST_FOR_NT
//#define TEST_FOR_9X

#include <windows.h>
#include <devguid.h>
#include <setupapi.h>
#include <tchar.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "devpkey_teraterm.h"
#include "ttlib.h"
#include "codeconv.h"

#define DllExport __declspec(dllexport)
#include "comportinfo.h"

typedef BOOL (WINAPI *TSetupDiGetDevicePropertyW)(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	const DEVPROPKEY *PropertyKey,
	DEVPROPTYPE *PropertyType,
	PBYTE PropertyBuffer,
	DWORD PropertyBufferSize,
	PDWORD RequiredSize,
	DWORD Flags
	);

typedef BOOL (WINAPI *TSetupDiGetDeviceRegistryPropertyW)(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	DWORD Property,
	PDWORD PropertyRegDataType,
	PBYTE PropertyBuffer,
	DWORD PropertyBufferSize,
	PDWORD RequiredSize
	);

typedef LONG (WINAPI *TRegQueryValueExW)(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
	);

static BOOL IsWindows9X()
{
	return !IsWindowsNTKernel();
}

/**
 *	ポート名を取得
 */
static BOOL GetComPortName(HDEVINFO hDevInfo, SP_DEVINFO_DATA *DeviceInfoData, wchar_t **str)
{
	TRegQueryValueExW pRegQueryValueExW =
		(TRegQueryValueExW)GetProcAddress(
			GetModuleHandleA("ADVAPI32.dll"), "RegQueryValueExW");
	DWORD byte_len;
	DWORD dwType = REG_SZ;
	HKEY hKey = SetupDiOpenDevRegKey(hDevInfo,
									 DeviceInfoData,
									 DICS_FLAG_GLOBAL,
									 0, DIREG_DEV, KEY_READ);
	if (hKey == NULL){
		// ポート名が取得できない?
		*str = NULL;
		return FALSE;
	}

	wchar_t* port_name = NULL;
	long r;
	if (pRegQueryValueExW != NULL && !IsWindows9X()) {
		// 9x系ではうまく動作しない
		r = pRegQueryValueExW(hKey, L"PortName", 0,
			&dwType, NULL, &byte_len);
		port_name = (wchar_t* )malloc(byte_len);
		r = pRegQueryValueExW(hKey, L"PortName", 0,
								&dwType, (LPBYTE)port_name, &byte_len);
	} else {
		r = RegQueryValueExA(hKey, "PortName", 0,
								&dwType, (LPBYTE)NULL, &byte_len);
		char *port_name_a = (char *)malloc(byte_len);
		r = RegQueryValueExA(hKey, "PortName", 0,
								&dwType, (LPBYTE)port_name_a, &byte_len);
		if (r == ERROR_SUCCESS) {
			size_t len_w = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
											   port_name_a, byte_len,
											   NULL, 0);
			port_name = (wchar_t *)malloc(sizeof(wchar_t) * len_w);
			MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
								port_name_a, byte_len,
								port_name, (int)len_w);
		}
		free(port_name_a);
	}
	RegCloseKey(hKey);
	if (r != ERROR_SUCCESS) {
		free(port_name);
		*str = NULL;
		return FALSE;
	}
	*str = port_name;
	return TRUE;
}

/**
 *	プロパティ取得
 *
 * レジストリの場所(Windows10)
 * HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{GUID}\0000
 *
 */
static void GetComPropartys(HDEVINFO hDevInfo, SP_DEVINFO_DATA *DeviceInfoData, const char *lang,
							wchar_t **friendly_name, wchar_t **str)
{
	const char *FriendlyNameString = "FRIENDLY_NAME";
	const char *DriverDateString = "DRIVER_DATE";
	typedef struct {
		const wchar_t *name;  // 本関数内で使う
		const char *key;      // 本関数内で使う
		const DEVPROPKEY *PropertyKey; // レジストリ取得に使うのはこれのみ
		DWORD Property;       // 未使用
	} list_t;
	static const list_t list[] = {
		{ L"Device Friendly Name",
		  FriendlyNameString,
		  &DEVPKEY_Device_FriendlyName,
		  SPDRP_FRIENDLYNAME },
		{ L"Device Instance ID",
		  "DEVICE_INSTANCE_ID",
		  &DEVPKEY_Device_InstanceId,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Device Manufacturer",
		  "DEVICE_MANUFACTURER",
		  &DEVPKEY_Device_Manufacturer,
		  SPDRP_MFG },
		{ L"Provider Name",
		  "PROVIDER_NAME",
		  &DEVPKEY_Device_DriverProvider,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Driver Date",
		  DriverDateString,
		  &DEVPKEY_Device_DriverDate,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Driver Version",
		  "DRIVER_VERSION",
		  &DEVPKEY_Device_DriverVersion,
		  SPDRP_MAXIMUM_PROPERTY },
	};
	TSetupDiGetDevicePropertyW pSetupDiGetDevicePropertyW =
		(TSetupDiGetDevicePropertyW)GetProcAddress(
			GetModuleHandleA("Setupapi.dll"),
			"SetupDiGetDevicePropertyW");
	TSetupDiGetDeviceRegistryPropertyW pSetupDiGetDeviceRegistryPropertyW =
		(TSetupDiGetDeviceRegistryPropertyW)GetProcAddress(
			GetModuleHandleA("Setupapi.dll"),
			"SetupDiGetDeviceRegistryPropertyW");

	wchar_t *s = _wcsdup(L"");
	size_t len = 0;
	for (size_t i = 0; i < _countof(list); i++) {
		const list_t *p = &list[i];
		BOOL r;
		wchar_t *str_prop = NULL;
		DWORD size;

		if (pSetupDiGetDevicePropertyW != NULL) {
			DEVPROPTYPE ulPropertyType;
			pSetupDiGetDevicePropertyW(hDevInfo, DeviceInfoData, p->PropertyKey,
									   &ulPropertyType, NULL, 0, &size, 0);
			str_prop = (wchar_t *)malloc(size);
			r = pSetupDiGetDevicePropertyW(hDevInfo, DeviceInfoData, p->PropertyKey,
										   &ulPropertyType, (PBYTE)str_prop, size, &size, 0);
		} else if (p->PropertyKey == &DEVPKEY_Device_InstanceId) {
			// InstanceIdはA系で決め打ち
			DWORD len_a;
			SetupDiGetDeviceInstanceIdA(hDevInfo,
										DeviceInfoData,
										NULL, 0,
										&len_a);
			char *str_instance_a = (char *)malloc(len_a);
			r = SetupDiGetDeviceInstanceIdA(hDevInfo,
											DeviceInfoData,
											str_instance_a, len_a,
											&len_a);
			if (r != FALSE) {
				int len_w = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
												str_instance_a, len_a,
												NULL, 0);
				str_prop = (wchar_t *)malloc(sizeof(wchar_t) * len_w);
				MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
									str_instance_a, len_a,
									str_prop, (int)len_w);
			}
			free(str_instance_a);
		} else if (p->Property == SPDRP_MAXIMUM_PROPERTY) {
			r = FALSE;
		} else if (pSetupDiGetDeviceRegistryPropertyW != NULL && !IsWindows9X()) {
			// 9x系ではうまく動作しない
			DWORD dwPropType;
			r = pSetupDiGetDeviceRegistryPropertyW(hDevInfo,
												   DeviceInfoData,
												   p->Property,
												   &dwPropType,
												   NULL, 0,
												   &size);
			str_prop = (wchar_t *)malloc(size);
			r = pSetupDiGetDeviceRegistryPropertyW(hDevInfo,
												   DeviceInfoData,
												   p->Property,
												   &dwPropType,
												   (LPBYTE)str_prop, size,
												   &size);
		} else {
			DWORD dwPropType;
			DWORD len_a;
			r = SetupDiGetDeviceRegistryPropertyA(hDevInfo,
												  DeviceInfoData,
												  p->Property,
												  &dwPropType,
												  NULL, 0,
												  &len_a);
			char *str_prop_a = (char *)malloc(len_a);
			r = SetupDiGetDeviceRegistryPropertyA(hDevInfo,
												  DeviceInfoData,
												  p->Property,
												  &dwPropType,
												  (PBYTE)str_prop_a, len_a,
												  &len_a);
			if (r != FALSE) {
				int len_w = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
												str_prop_a, len_a,
												NULL, 0);
				str_prop = (wchar_t *)malloc(sizeof(wchar_t) * len_w);
				MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
									str_prop_a, len_a,
									str_prop, (int)len_w);
			}
			free(str_prop_a);
		}
		if (r != FALSE) {
			/* Driver Dateは DEVPROP_TYPE_FILETIME であるため、
			 * FILETIME 構造体の8バイトで返るため、SYSTEMTIME に変換する。
			 */
			if (strcmp(list[i].key, DriverDateString) == 0) {
				FILETIME ftFileTime;
				SYSTEMTIME stFileTime;
				int wbuflen = 64;
				int buflen = sizeof(wchar_t) * wbuflen;

				ZeroMemory(&ftFileTime, sizeof(FILETIME));
				if (sizeof(ftFileTime) >= size) {
					// str_propはsizeバイト
					memcpy(&ftFileTime, str_prop, size);  
				}
				ZeroMemory(&stFileTime, sizeof(SYSTEMTIME));
				FileTimeToSystemTime(&ftFileTime , &stFileTime);
				str_prop = (wchar_t *)realloc(str_prop, buflen);
				_snwprintf_s(str_prop, wbuflen, _TRUNCATE, L"%d-%d-%d",
					stFileTime.wMonth, stFileTime.wDay, stFileTime.wYear				
					);
			}

			size_t name_len = wcslen(p->name);
			size_t prop_len = wcslen(str_prop);
			len = len + (name_len + (i==0?1:2) + 2 + 2 + prop_len);
			s = (wchar_t *)realloc(s, sizeof(wchar_t) * len);
			if (i != 0) 
				wcscat_s(s, len, L"\r\n");
			wcscat_s(s, len, p->name);
			wcscat_s(s, len, L": ");
			wcscat_s(s, len, str_prop);
		}

		if (strcmp(list[i].key, FriendlyNameString) == 0) {
			// str_propのメモリは ComPortInfoFree() で解放される。
			*friendly_name = str_prop;

		} else {
			// s にコピーしたのでstr_propのメモリは不要となる。
			if (str_prop != NULL) {
				free(str_prop);
			}
		}
	}

	*str = s;
}

/* 配列ソート用 */
static int sort_sub(const void *a_, const void *b_)
{
	const ComPortInfo_t *a = (ComPortInfo_t *)a_;
	const ComPortInfo_t *b = (ComPortInfo_t *)b_;
	if (wcsncmp(a->port_name, L"COM", 3) == 0 &&
		wcsncmp(b->port_name, L"COM", 3) == 0) {
		int a_no = _wtoi(&a->port_name[3]);
		int b_no = _wtoi(&b->port_name[3]);
		return a_no > b_no;
	}
	return wcscmp(a->port_name, b->port_name);
}

/**
 *	comポートの情報を取得する
 *
 *	@param[out]	count	 	情報数(0のときcomポートなし)
 *	@return		情報へのポインタ(配列)、ポート番号の小さい順
 *				NULLのときcomポートなし
 *				使用後ComPortInfoFree()を呼ぶこと
 */
ComPortInfo_t * WINAPI ComPortInfoGet(int *count, const char *lang)
{
	int comport_count = 0;
	ComPortInfo_t *comport_infos = NULL;
	const GUID *pClassGuid = &GUID_DEVCLASS_PORTS;

	// List all connected serial devices
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(pClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		*count = 0;
		return NULL;
	}

	// Find the ones that are driverless
	for (int i = 0; ; i++) {
		SP_DEVINFO_DATA DeviceInfoData;
		DeviceInfoData.cbSize = sizeof (DeviceInfoData);
		if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
			break;

		// ポート名取得 ("COM1"など)
		wchar_t *port_name;
		if (!GetComPortName(hDevInfo, &DeviceInfoData, &port_name)) {
			continue;
		}
		int port_no = 0;
		if (wcsncmp(port_name, L"COM", 3) == 0) {
			port_no = _wtoi(port_name+3);
		}

		// 情報取得
		wchar_t *str_friendly_name = NULL;
		wchar_t *str_prop = NULL;
		GetComPropartys(hDevInfo, &DeviceInfoData, lang, &str_friendly_name, &str_prop);

		comport_count++;
		comport_infos = (ComPortInfo_t *)realloc(comport_infos,
								sizeof(ComPortInfo_t) * comport_count);
		ComPortInfo_t *p = &comport_infos[comport_count-1];
		p->port_name = port_name;  // COMポート名
		p->port_no = port_no;  // COMポート番号
		p->friendly_name = str_friendly_name;  // Device Description
		p->property = str_prop;  // 全詳細情報

#if 0
		{
		char *a, *b, *c;
		a = ToCharW(p->port_name);
		b = ToCharW(p->friendly_name);
		c = ToCharW(p->property);
		OutputDebugPrintf("%s: [%s] [%d] [%s] [%s]\n", __FUNCTION__,
			a, p->port_no, b, c
			);
		free(a);
		free(b);
		free(c);
		}
#endif
	}

	/* ポート名順に並べる */
	qsort(comport_infos, comport_count, sizeof(comport_infos[0]), sort_sub);

	*count = comport_count;
	return comport_infos;
}

void WINAPI ComPortInfoFree(ComPortInfo_t *info, int count)
{
	for (int i=0; i< count; i++) {
		ComPortInfo_t *p = &info[i];
		free(p->port_name);
		free(p->friendly_name);
		free(p->property);
	}
	free(info);
}
