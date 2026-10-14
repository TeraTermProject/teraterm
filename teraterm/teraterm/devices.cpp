/*
 * (C) 2026- TeraTerm Project
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


/*
 *	Saved devices (switches) for the sidebar
 */

#include <windows.h>
#include <wincred.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "devices.h"

#if defined(_MSC_VER)
#pragma comment(lib, "advapi32.lib")
#endif

#define CRED_PREFIX L"teratermbutcool/device/"

static Device *devices;
static int device_count;
static wchar_t *ini_path;

static int sidebar_width;		// 0 = default
static BOOL sidebar_visible = TRUE;

static void CopyStr(wchar_t *dst, size_t count, const wchar_t *src)
{
	wcsncpy_s(dst, count, src != NULL ? src : L"", _TRUNCATE);
}

/*
 *	devices.ini is UTF-16 so names in any language survive,
 *	WritePrivateProfileStringW keeps UTF-16 when the file starts with a BOM
 */
static BOOL CreateUnicodeIni(const wchar_t *path)
{
	HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	static const BYTE bom[2] = { 0xff, 0xfe };
	DWORD written;
	WriteFile(h, bom, sizeof(bom), &written, NULL);
	CloseHandle(h);
	return TRUE;
}

static void ReadStr(const wchar_t *section, const wchar_t *key, wchar_t *buf, DWORD count)
{
	GetPrivateProfileStringW(section, key, L"", buf, count, ini_path);
}

static void WriteInt(const wchar_t *path, const wchar_t *section, const wchar_t *key, int value)
{
	wchar_t buf[32];
	swprintf_s(buf, _countof(buf), L"%d", value);
	WritePrivateProfileStringW(section, key, buf, path);
}

static void NewId(wchar_t *id, size_t count)
{
	static LONG counter;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULONGLONG t = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	t ^= (ULONGLONG)GetCurrentProcessId() << 48;
	t += (ULONGLONG)InterlockedIncrement(&counter);
	swprintf_s(id, count, L"%016llx", t);
}

void DeviceInit(Device *dev, DeviceProtocol protocol)
{
	memset(dev, 0, sizeof(*dev));
	dev->protocol = protocol;
	dev->port = protocol == DEVICE_TELNET ? 23 : 22;
	dev->com_port = 1;
	dev->baud = 9600;
}

void DevicesLoad(const wchar_t *settings_dir)
{
	free(devices);
	devices = NULL;
	device_count = 0;
	free(ini_path);
	if (settings_dir == NULL) {
		settings_dir = L".";
	}

	size_t len = wcslen(settings_dir) + 16;
	ini_path = (wchar_t *)malloc(len * sizeof(wchar_t));
	swprintf_s(ini_path, len, L"%s\\devices.ini", settings_dir);

	const int count = GetPrivateProfileIntW(L"Devices", L"Count", 0, ini_path);
	for (int i = 0; i < count; i++) {
		wchar_t section[32];
		swprintf_s(section, _countof(section), L"Device%d", i);
		Device d;
		DeviceInit(&d, DEVICE_SSH);
		ReadStr(section, L"Id", d.id, _countof(d.id));
		ReadStr(section, L"Name", d.name, _countof(d.name));
		if (d.name[0] == 0) {
			continue;
		}
		ReadStr(section, L"Site", d.site, _countof(d.site));
		ReadStr(section, L"Host", d.host, _countof(d.host));
		ReadStr(section, L"User", d.user, _countof(d.user));
		d.protocol = GetPrivateProfileIntW(section, L"Protocol", DEVICE_SSH, ini_path);
		if (d.protocol < DEVICE_SSH || d.protocol > DEVICE_SERIAL) {
			d.protocol = DEVICE_SSH;
		}
		d.port = GetPrivateProfileIntW(section, L"Port", d.protocol == DEVICE_TELNET ? 23 : 22, ini_path);
		d.com_port = GetPrivateProfileIntW(section, L"ComPort", 1, ini_path);
		d.baud = (DWORD)GetPrivateProfileIntW(section, L"Baud", 9600, ini_path);
		if (d.id[0] == 0) {
			NewId(d.id, _countof(d.id));
		}
		devices = (Device *)realloc(devices, sizeof(Device) * (device_count + 1));
		devices[device_count++] = d;
	}

	sidebar_width = GetPrivateProfileIntW(L"Sidebar", L"Width", 0, ini_path);
	sidebar_visible = GetPrivateProfileIntW(L"Sidebar", L"Visible", 1, ini_path) != 0;
}

/*
 *	write the whole list to a temporary file, then replace devices.ini
 */
void DevicesSave(void)
{
	if (ini_path == NULL) {
		return;
	}
	size_t len = wcslen(ini_path) + 8;
	wchar_t *tmp = (wchar_t *)malloc(len * sizeof(wchar_t));
	swprintf_s(tmp, len, L"%s.tmp", ini_path);
	if (!CreateUnicodeIni(tmp)) {
		free(tmp);
		return;
	}
	WriteInt(tmp, L"Devices", L"Count", device_count);
	for (int i = 0; i < device_count; i++) {
		const Device *d = &devices[i];
		wchar_t section[32];
		swprintf_s(section, _countof(section), L"Device%d", i);
		WritePrivateProfileStringW(section, L"Id", d->id, tmp);
		WritePrivateProfileStringW(section, L"Name", d->name, tmp);
		WritePrivateProfileStringW(section, L"Site", d->site, tmp);
		WriteInt(tmp, section, L"Protocol", d->protocol);
		WritePrivateProfileStringW(section, L"Host", d->host, tmp);
		WriteInt(tmp, section, L"Port", d->port);
		WritePrivateProfileStringW(section, L"User", d->user, tmp);
		WriteInt(tmp, section, L"ComPort", d->com_port);
		WriteInt(tmp, section, L"Baud", (int)d->baud);
	}
	WriteInt(tmp, L"Sidebar", L"Width", sidebar_width);
	WriteInt(tmp, L"Sidebar", L"Visible", sidebar_visible ? 1 : 0);
	WritePrivateProfileStringW(NULL, NULL, NULL, tmp);	// flush the profile cache
	MoveFileExW(tmp, ini_path, MOVEFILE_REPLACE_EXISTING);
	free(tmp);
}

int DevicesCount(void)
{
	return device_count;
}

const Device *DevicesGet(int index)
{
	if (index < 0 || index >= device_count) {
		return NULL;
	}
	return &devices[index];
}

int DevicesFind(const wchar_t *id)
{
	for (int i = 0; i < device_count; i++) {
		if (wcscmp(devices[i].id, id) == 0) {
			return i;
		}
	}
	return -1;
}

int DevicesAdd(const Device *dev)
{
	devices = (Device *)realloc(devices, sizeof(Device) * (device_count + 1));
	Device *d = &devices[device_count];
	*d = *dev;
	NewId(d->id, _countof(d->id));
	device_count++;
	DevicesSave();
	return device_count - 1;
}

void DevicesUpdate(int index, const Device *dev)
{
	if (index < 0 || index >= device_count) {
		return;
	}
	wchar_t id[32];
	CopyStr(id, _countof(id), devices[index].id);
	devices[index] = *dev;
	CopyStr(devices[index].id, _countof(devices[index].id), id);
	DevicesSave();
}

void DevicesDelete(int index)
{
	if (index < 0 || index >= device_count) {
		return;
	}
	DevicePasswordSet(&devices[index], NULL);
	memmove(&devices[index], &devices[index + 1], sizeof(Device) * (device_count - index - 1));
	device_count--;
	DevicesSave();
}

/*
 *	Credential Manager
 */
static void CredTarget(const Device *dev, wchar_t *buf, size_t count)
{
	swprintf_s(buf, count, CRED_PREFIX L"%s", dev->id);
}

BOOL DevicePasswordGet(const Device *dev, wchar_t *password, size_t count)
{
	password[0] = 0;
	wchar_t target[96];
	CredTarget(dev, target, _countof(target));
	PCREDENTIALW cred = NULL;
	if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &cred)) {
		return FALSE;
	}
	const size_t chars = cred->CredentialBlobSize / sizeof(wchar_t);
	BOOL ok = FALSE;
	if (chars < count) {
		memcpy(password, cred->CredentialBlob, chars * sizeof(wchar_t));
		password[chars] = 0;
		ok = TRUE;
	}
	SecureZeroMemory(cred->CredentialBlob, cred->CredentialBlobSize);
	CredFree(cred);
	return ok;
}

BOOL DevicePasswordSet(const Device *dev, const wchar_t *password)
{
	wchar_t target[96];
	CredTarget(dev, target, _countof(target));
	if (password == NULL || password[0] == 0) {
		CredDeleteW(target, CRED_TYPE_GENERIC, 0);
		return TRUE;
	}
	CREDENTIALW cred = {};
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = target;
	cred.Comment = (LPWSTR)L"Tera Term saved device";
	cred.CredentialBlobSize = (DWORD)(wcslen(password) * sizeof(wchar_t));
	cred.CredentialBlob = (LPBYTE)password;
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cred.UserName = (LPWSTR)dev->user;
	return CredWriteW(&cred, 0);
}

BOOL DeviceHasPassword(const Device *dev)
{
	wchar_t target[96];
	CredTarget(dev, target, _countof(target));
	PCREDENTIALW cred = NULL;
	if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &cred)) {
		return FALSE;
	}
	SecureZeroMemory(cred->CredentialBlob, cred->CredentialBlobSize);
	CredFree(cred);
	return TRUE;
}

/*
 *	command line
 */
typedef struct {
	wchar_t *buf;
	size_t len, size;
} StrBuf;

static void Append(StrBuf *s, const wchar_t *str)
{
	const size_t add = wcslen(str);
	if (s->len + add + 1 > s->size) {
		size_t size = (s->len + add + 1) * 2;
		wchar_t *buf = (wchar_t *)malloc(size * sizeof(wchar_t));
		if (s->buf != NULL) {
			wmemcpy(buf, s->buf, s->len);
			SecureZeroMemory(s->buf, s->size * sizeof(wchar_t));
			free(s->buf);
		}
		s->buf = buf;
		s->size = size;
	}
	wmemcpy(s->buf + s->len, str, add);
	s->len += add;
	s->buf[s->len] = 0;
}

// "value" with " doubled (Tera Term and TTSSH command line quoting)
static void AppendQuoted(StrBuf *s, const wchar_t *value)
{
	Append(s, L"\"");
	for (const wchar_t *p = value; *p != 0; p++) {
		wchar_t c[2] = { *p, 0 };
		Append(s, c);
		if (*p == L'"') {
			Append(s, c);
		}
	}
	Append(s, L"\"");
}

static void AppendHostPort(StrBuf *s, const Device *dev)
{
	wchar_t buf[300];
	if (wcschr(dev->host, L':') != NULL && dev->host[0] != L'[') {
		// IPv6 address
		swprintf_s(buf, _countof(buf), L" [%s]:%d", dev->host, dev->port);
	}
	else {
		swprintf_s(buf, _countof(buf), L" %s:%d", dev->host, dev->port);
	}
	Append(s, buf);
}

wchar_t *DeviceBuildCommand(const Device *dev, const wchar_t *password)
{
	StrBuf s = {};
	Append(&s, L"ttermpro");
	wchar_t buf[64];
	switch (dev->protocol) {
	case DEVICE_SERIAL:
		swprintf_s(buf, _countof(buf), L" /C=%d /BAUD=%lu", dev->com_port, (unsigned long)dev->baud);
		Append(&s, buf);
		break;
	case DEVICE_TELNET:
		AppendHostPort(&s, dev);
		Append(&s, L" /nossh /T=1");
		break;
	case DEVICE_SSH:
	default:
		AppendHostPort(&s, dev);
		Append(&s, L" /ssh2");
		if (dev->user[0] != 0) {
			Append(&s, L" /auth=password /user=");
			AppendQuoted(&s, dev->user);
			if (password != NULL && password[0] != 0) {
				Append(&s, L" /passwd=");
				AppendQuoted(&s, password);
			}
			else {
				Append(&s, L" /ask4passwd");
			}
		}
		break;
	}
	return s.buf;
}

void DeviceFreeCommand(wchar_t *command)
{
	if (command != NULL) {
		SecureZeroMemory(command, wcslen(command) * sizeof(wchar_t));
		free(command);
	}
}

/*
 *	sidebar settings
 */
static void SavePref(const wchar_t *key, int value)
{
	if (ini_path == NULL) {
		return;
	}
	if (GetFileAttributesW(ini_path) == INVALID_FILE_ATTRIBUTES) {
		CreateUnicodeIni(ini_path);
	}
	WriteInt(ini_path, L"Sidebar", key, value);
}

int DevicesGetSidebarWidth(void)
{
	return sidebar_width;
}

void DevicesSetSidebarWidth(int width)
{
	sidebar_width = width;
	SavePref(L"Width", width);
}

BOOL DevicesGetSidebarVisible(void)
{
	return sidebar_visible;
}

void DevicesSetSidebarVisible(BOOL visible)
{
	sidebar_visible = visible;
	SavePref(L"Visible", visible ? 1 : 0);
}
