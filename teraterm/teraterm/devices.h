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
 *
 *	The list is stored in devices.ini in the user settings folder.
 *	Passwords are never written there; they are kept in the Windows
 *	Credential Manager (generic credential "teratermbutcool/device/<id>").
 */

#pragma once

#include <windows.h>

typedef enum {
	DEVICE_SSH = 0,
	DEVICE_TELNET = 1,
	DEVICE_SERIAL = 2,
} DeviceProtocol;

typedef struct {
	wchar_t id[32];			// stable id, names the credential
	wchar_t name[128];
	wchar_t site[128];		// group in the sidebar, "" = no site
	wchar_t host[256];
	wchar_t user[128];
	int protocol;			// DeviceProtocol
	int port;				// TCP port (SSH, Telnet)
	int com_port;			// serial: COMn
	DWORD baud;				// serial
} Device;

void DevicesLoad(const wchar_t *settings_dir);
void DevicesSave(void);
int DevicesCount(void);
const Device *DevicesGet(int index);
int DevicesFind(const wchar_t *id);
int DevicesAdd(const Device *dev);			// a new id is assigned, returns the index
void DevicesUpdate(int index, const Device *dev);
void DevicesDelete(int index);				// also deletes the saved password
void DeviceInit(Device *dev, DeviceProtocol protocol);

BOOL DevicePasswordGet(const Device *dev, wchar_t *password, size_t count);
BOOL DevicePasswordSet(const Device *dev, const wchar_t *password);	// NULL or "" deletes it
BOOL DeviceHasPassword(const Device *dev);

// Tera Term command line that opens the device; free() it, it may hold a password
wchar_t *DeviceBuildCommand(const Device *dev, const wchar_t *password);
void DeviceFreeCommand(wchar_t *command);

// sidebar settings, stored in devices.ini too
int DevicesGetSidebarWidth(void);		// in 96 dpi pixels, 0 = default
void DevicesSetSidebarWidth(int width);
BOOL DevicesGetSidebarVisible(void);
void DevicesSetSidebarVisible(BOOL visible);
