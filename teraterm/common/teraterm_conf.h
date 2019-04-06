/*
 * (C) 2019 TeraTerm Project
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

/* teraterm_conf.h */

/*
 * windows.h などを include する前に include するファイル
 * 必要な定義を行う
 */

#pragma once

/* 使用する Windows SDK のバージョンを指定する
 *		・Windows SDK(header)内の各種定義のバージョンを指定する
 *		・Tera Term 内で使用している各種定義がSDKになければ
 *		  compat_win.h で定義される
 */
#if !defined(_WIN32_WINNT)
//#define	_WIN32_WINNT 0x0a00		// _WINNT_WIN10	Windows 10
#define		_WIN32_WINNT 0x0501		// _WINNT_WINXP	Windows XP ↑build ok
//#define	_WIN32_WINNT 0x0500		// _WINNT_WIN2K	Windows 2000 ↓build ng
//#define	_WIN32_WINNT 0x0400		// _WINNT_NT4	Windows NT 4.0(95)
#endif

/*
 *	_WIN32_WINNTから次のdefineを適切に設定する
 *		NTDDI_VERSION
 *		WINVER
 *		_WIN32_IE
 *	このdefineは定義されない
 *		_WIN32_WINDOWS
 */
//#include <sdkddkver.h>


/*
 *	SDK 7.0
 *		Windows Server 2003 R2 Platform SDK
 *		(Microsoft Windows SDK for Windows 7 and .NET Framework 3.5 SP1)
 *	SDK 7.1
 *		Microsoft Windows SDK for Windows 7 and .NET Framework 4
 */
/*
 * SDK 7.0 対策
 *	7.0 は 7.1 以降と次の違いがある
 *	- _WIN32_WINNT が定義されていない場合、自動で設定しない
 *		_WIN32_IEなども自動で設定されない
 *	- sdkddkver.h が存在しない
 *		_WIN32_WINNT から _WIN32_IE などを適切な値に自動設定するヘッダ
 *		手動で設定する
 *	- WinSDKVer.h が存在しない
 *		_WIN32_WINNT_MAXVER がない
 *		使用している SDK のバージョンのヒントを得られない
 */
#if !defined(_WIN32_IE)
#define _WIN32_IE       0x0600		// _WIN32_IE_XP
#endif
