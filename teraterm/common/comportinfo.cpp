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

#include "comportinfo.h"

/*
 *	devpkey.h �������?
 *		HAS_DEVPKEY_H �� define �����
 */
#if	defined(_MSC_VER)
#if	(_MSC_VER > 1400)

// VS2019�̂Ƃ�(VS2005���傫���Ƃ��Ă���)
#define HAS_DEVPKEY_H	1

#else // _MSC_VER > 1400

// VS2008�̂Ƃ�
#if defined(_INC_SDKDDKVER)

// VS2008 + SDK 7.0�ł͂Ȃ��Ƃ�(SDK 7.1�̂Ƃ�)
//   SDK 7.0 �̏ꍇ�� sdkddkver.h �� include ����Ă��Ȃ�
#define HAS_DEVPKEY_H	1

#endif  //  defined(_INC_SDKDDKVER)
#endif
#elif defined(__MINGW32__)

#if	__MINGW64_VERSION_MAJOR >= 8
// mingw64 8+ �̂Ƃ�
#define HAS_DEVPKEY_H	1
#endif

#endif  // defined(_MSC_VER)

/*
 *	devpkey.h �� include
 */
#if defined(HAS_DEVPKEY_H)

#define INITGUID
#include <devpkey.h>

#else //  defined(HAS_DEVPKEY_H)

#include "devpkey_teraterm.h"

#endif //  defined(HAS_DEVPKEY_H)

#define INITGUID
#include <guiddef.h>

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

static wchar_t *ToWcharA(const char *strA_ptr, size_t strA_len)
{
	size_t strW_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
										  strA_ptr, (int)strA_len,
										  NULL, 0);
	wchar_t *strW_ptr = (wchar_t*)malloc(sizeof(wchar_t) * strW_len);
	MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
						strA_ptr, (int)strA_len,
						strW_ptr, (int)strW_len);
	return strW_ptr;
}

/**
 *	�|�[�g�����擾
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
		// �|�[�g�����擾�ł��Ȃ�?
		*str = NULL;
		return FALSE;
	}

	wchar_t* port_name = NULL;
	long r;
	if (pRegQueryValueExW != NULL && !IsWindows9X()) {
		// 9x�n�ł͂��܂����삵�Ȃ�
		r = pRegQueryValueExW(hKey, L"PortName", 0,
			&dwType, NULL, &byte_len);
		if (r != ERROR_FILE_NOT_FOUND) {
			port_name = (wchar_t*)malloc(byte_len);
			r = pRegQueryValueExW(hKey, L"PortName", 0,
				&dwType, (LPBYTE)port_name, &byte_len);
		}
	} else {
		r = RegQueryValueExA(hKey, "PortName", 0,
								&dwType, (LPBYTE)NULL, &byte_len);
		if (r != ERROR_FILE_NOT_FOUND) {
			char* port_name_a = (char*)malloc(byte_len);
			r = RegQueryValueExA(hKey, "PortName", 0,
				&dwType, (LPBYTE)port_name_a, &byte_len);
			if (r == ERROR_SUCCESS) {
				port_name = ToWcharA(port_name_a, byte_len);
			}
			free(port_name_a);
		}
	}
	if (r == ERROR_SUCCESS) {
		RegCloseKey(hKey);
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
 *	�v���p�e�B�擾
 *
 * ���W�X�g���̏ꏊ(Windows10)
 * HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{GUID}\0000
 *
 */
static void GetComProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA *DeviceInfoData,
						   wchar_t **friendly_name, wchar_t **prop_str)
{
	typedef struct {
		const wchar_t *name;
		const DEVPROPKEY *PropertyKey;	// for SetupDiGetDeviceProperty() Vista+
		DWORD Property;					// for SetupDiGetDeviceRegistryProperty() 2000+
	} list_t;
	static const list_t list[] = {
		{ L"Device Friendly Name",
		  &DEVPKEY_Device_FriendlyName,
		  SPDRP_FRIENDLYNAME },
		{ L"Device Instance ID",
		  &DEVPKEY_Device_InstanceId,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Device Manufacturer",
		  &DEVPKEY_Device_Manufacturer,
		  SPDRP_MFG },
		{ L"Provider Name",
		  &DEVPKEY_Device_DriverProvider,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Driver Date",
		  &DEVPKEY_Device_DriverDate,
		  SPDRP_MAXIMUM_PROPERTY },
		{ L"Driver Version",
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

	*friendly_name = NULL;
	*prop_str = NULL;
	wchar_t *p_ptr = NULL;
	size_t p_len = 0;
	for (size_t i = 0; i < _countof(list); i++) {
		const list_t *p = &list[i];
		BOOL r;
		wchar_t *prop = NULL;

		if (pSetupDiGetDevicePropertyW != NULL) {
			// vista�ȏ�͂��ׂĂ����ɓ���
			DEVPROPTYPE ulPropertyType;
			DWORD size;
			r = pSetupDiGetDevicePropertyW(hDevInfo, DeviceInfoData, p->PropertyKey,
										   &ulPropertyType, NULL, 0, &size, 0);
			if (r == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				BYTE *buf = (BYTE *)malloc(size);

				r = pSetupDiGetDevicePropertyW(hDevInfo, DeviceInfoData, p->PropertyKey,
											   &ulPropertyType, buf, size, &size, 0);
				if (ulPropertyType == DEVPROP_TYPE_STRING) {
					// ������Ȃ̂ł��̂܂�
					prop = (wchar_t *)buf;
				} else if (ulPropertyType ==  DEVPROP_TYPE_FILETIME) {
					// buf = FILETIME �\���̂�8�o�C�g
					SYSTEMTIME stFileTime = {};
					FileTimeToSystemTime((FILETIME *)buf , &stFileTime);
					int wbuflen = 64;
					int buflen = sizeof(wchar_t) * wbuflen;
					prop = (wchar_t *)malloc(buflen);
					_snwprintf_s(prop, wbuflen, _TRUNCATE, L"%d-%d-%d",
								 stFileTime.wMonth, stFileTime.wDay, stFileTime.wYear
						);
					free(buf);
				}
				else {
					assert(FALSE);
				}
			}
		} else if (p->PropertyKey == &DEVPKEY_Device_InstanceId) {
			// InstanceId��A�n�Ō��ߑł�
			DWORD len_a;
			r = SetupDiGetDeviceInstanceIdA(hDevInfo,
											DeviceInfoData,
											NULL, 0,
											&len_a);
			if (r == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				char *str_instance_a = (char *)malloc(len_a);
				r = SetupDiGetDeviceInstanceIdA(hDevInfo,
												DeviceInfoData,
												str_instance_a, len_a,
												&len_a);
				if (r != FALSE) {
					prop = ToWcharA(str_instance_a, len_a);
				}
				free(str_instance_a);
			}
		} else if (p->Property == SPDRP_MAXIMUM_PROPERTY) {
			// SetupDiGetDeviceRegistryProperty() �n�ɂ͑��݂��Ȃ��v���p�e�B
			r = FALSE;
		} else if (pSetupDiGetDeviceRegistryPropertyW != NULL && !IsWindows9X()) {
			// 9x�n�ł͂��܂����삵�Ȃ�
			DWORD dwPropType;
			DWORD size;
			r = pSetupDiGetDeviceRegistryPropertyW(hDevInfo,
												   DeviceInfoData,
												   p->Property,
												   &dwPropType,
												   NULL, 0,
												   &size);
			if (r == FALSE) {
				prop = (wchar_t *)malloc(size);
				r = pSetupDiGetDeviceRegistryPropertyW(hDevInfo,
													   DeviceInfoData,
													   p->Property,
													   &dwPropType,
													   (LPBYTE)prop, size,
													   &size);
			}
		} else {
			DWORD dwPropType;
			DWORD len_a;
			r = SetupDiGetDeviceRegistryPropertyA(hDevInfo,
												  DeviceInfoData,
												  p->Property,
												  &dwPropType,
												  NULL, 0,
												  &len_a);
			if (r != FALSE) {
				char *prop_a = (char *)malloc(len_a);
				r = SetupDiGetDeviceRegistryPropertyA(hDevInfo,
													  DeviceInfoData,
													  p->Property,
													  &dwPropType,
													  (PBYTE)prop_a, len_a,
													  &len_a);
				if (r != FALSE) {
					prop = ToWcharA(prop_a, len_a);
				}
				free(prop_a);
			}
		}

		// prop
		if (r != FALSE && prop != NULL) {
			if (i == 0) {
				// �t�����h���[�l�[���̂�
				*friendly_name = prop;
			}

			// �t�����h���[�l�[�����܂߂����ׂẴv���p�e�B
			const size_t name_len = wcslen(p->name);
			const size_t prop_len = wcslen(prop);

			if (p_len == 0) {
				p_len = p_len + (name_len + 2 + prop_len + 1);
				p_ptr = (wchar_t *)malloc(sizeof(wchar_t) * p_len);
				p_ptr[0] = L'\0';
			}
			else {
				p_len = p_len + (2 + name_len + 2 + prop_len);
				p_ptr = (wchar_t *)realloc(p_ptr, sizeof(wchar_t) * p_len);
				wcscat_s(p_ptr, p_len, L"\r\n");
			}
			wcscat_s(p_ptr, p_len, p->name);
			wcscat_s(p_ptr, p_len, L": ");
			wcscat_s(p_ptr, p_len, prop);

			if (i != 0) {
				free(prop);
			}
		}
	}

	*prop_str = p_ptr;
}

/* �z��\�[�g�p */
static int sort_sub(const void *a_, const void *b_)
{
	const ComPortInfo_t *a = (ComPortInfo_t *)a_;
	const ComPortInfo_t *b = (ComPortInfo_t *)b_;
	const int a_no = a->port_no;
	const int b_no = b->port_no;
	return (a_no == b_no) ? 0 : (a_no > b_no) ? 1 : -1;
}

/**
 *	���ۂɃf�o�C�X���I�[�v�����邱�Ƃ�com�|�[�g���o
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

			comport_count++;
			comport_infos = (ComPortInfo_t *)realloc(comport_infos, sizeof(ComPortInfo_t) * comport_count);

			ComPortInfo_t *p = &comport_infos[comport_count - 1];
			wchar_t com_name[12];
			_snwprintf_s(com_name, _countof(com_name), _TRUNCATE, L"COM%d", i);
			p->port_name = _wcsdup(com_name);  // COM�|�[�g��
			p->port_no = i;  // COM�|�[�g�ԍ�
			p->friendly_name = NULL;
			p->property = NULL;
		}
	}

	*count = comport_count;
	return comport_infos;
}

static ComPortInfo_t *ComPortInfoGetByGetSetupAPI(int *count)
{
	static const GUID *pClassGuids[] = {
		&GUID_DEVCLASS_PORTS,
		&GUID_DEVCLASS_MODEM,
	};
	int comport_count = 0;
	ComPortInfo_t *comport_infos = NULL;

	for (int i = 0; i < _countof(pClassGuids); i++) {
		// List all connected devices
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(pClassGuids[i], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			continue;
		}

		// Find the ones that are driverless
		for (DWORD j = 0; ; j++) {
			SP_DEVINFO_DATA DeviceInfoData = {};
			DeviceInfoData.cbSize = sizeof (DeviceInfoData);
			if (!SetupDiEnumDeviceInfo(hDevInfo, j, &DeviceInfoData)) {
				break;
			}

			// check status
#if !defined(SUPPORT_OLD_WINDOWS)
			ULONG status  = 0;
			ULONG problem = 0;
			CONFIGRET cr = CM_Get_DevNode_Status(&status, &problem, DeviceInfoData.DevInst, 0);
			if (cr != CR_SUCCESS) {
				continue;
			}
			if (problem != 0) {
				// ���炩�̖�肪������?
				continue;
			}
#endif

			wchar_t *port_name;
			if (!GetComPortName(hDevInfo, &DeviceInfoData, &port_name)) {
				continue;
			}
			int port_no = 0;
			if (wcsncmp(port_name, L"COM", 3) == 0) {
				port_no = _wtoi(port_name+3);
			}

			// ���擾
			wchar_t *str_friendly_name = NULL;
			wchar_t *str_prop = NULL;
			GetComProperty(hDevInfo, &DeviceInfoData, &str_friendly_name, &str_prop);

			ComPortInfo_t *p =
				(ComPortInfo_t *)realloc(comport_infos,
										 sizeof(ComPortInfo_t) * (comport_count + 1));
			if (p == NULL) {
				break;
			}
			comport_infos = p;
			comport_count++;
			p = &comport_infos[comport_count-1];
			p->port_name = port_name;  // COM�|�[�g��
			p->port_no = port_no;  // COM�|�[�g�ԍ�
			p->friendly_name = str_friendly_name;  // Device Description
			p->property = str_prop;  // �S�ڍ׏��
		}
	}

	/* �|�[�g�����ɕ��ׂ� */
	qsort(comport_infos, comport_count, sizeof(comport_infos[0]), sort_sub);

	*count = comport_count;
	return comport_infos;
}

/**
 *	com�|�[�g�̏����擾����
 *
 *	@param[out]	count		���(0�̂Ƃ�com�|�[�g�Ȃ�)
 *	@return		���ւ̃|�C���^(�z��)�A�|�[�g�ԍ��̏�������
 *				NULL�̂Ƃ�com�|�[�g�Ȃ�
 *				�g�p��ComPortInfoFree()���ĂԂ���
 */
ComPortInfo_t *ComPortInfoGet(int *count)
{
#if defined(_MSC_VER) && _MSC_VER > 1400
	// VS2005��肠���炵���ꍇ�� 9x �ŋN�����Ȃ��o�C�i���ƂȂ�
	const bool is_setupapi_supported = true;
#else
	// VS2005 or MinGW
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
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
		// setupapi �̓��삪�����OS�̂Ƃ�
		return ComPortInfoGetByCreatefile(count);
	}
}

/**
 *	com�|�[�g�̏�����������j������
 */
void ComPortInfoFree(ComPortInfo_t *info, int count)
{
	for (int i=0; i< count; i++) {
		ComPortInfo_t *p = &info[i];
		free(p->port_name);
		free(p->friendly_name);
		free(p->property);
	}
	free(info);
}
