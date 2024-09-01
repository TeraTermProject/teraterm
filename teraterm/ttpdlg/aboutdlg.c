/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2024- TeraTerm Project
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

/* about dialog */

#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "tt-version.h"
#include "ttlib.h"
#include "dlglib.h"
#include "dlg_res.h"
#include "asprintf.h"
#include "ttwinman.h"
#include "tttext.h"

// Oniguruma: Regular expression library
#define ONIG_STATIC
#include "oniguruma.h"

// SFMT: SIMD-oriented Fast Mersenne Twister
#include "SFMT_version_for_teraterm.h"

#include "ttdlg.h"

#undef EFFECT_ENABLED	// �G�t�F�N�g�̗L����
#undef TEXTURE_ENABLED	// �e�N�X�`���̗L����

#if defined(_MSC_VER)
// �r���h�����Ƃ��Ɏg��ꂽVisual C++�̃o�[�W�������擾����(2009.3.3 yutaka)
static void GetCompilerInfo(char *buf, size_t buf_size)
{
	char tmpbuf[128];
	int msc_ver, vs_ver, msc_low_ver;

	strcpy_s(buf, buf_size, "Microsoft Visual C++");
#ifdef _MSC_FULL_VER
	// _MSC_VER  VS Ver.  VS internal Ver.  MSVC++ Ver.
	// 1400      2005     8.0               8.0
	// 1500      2008     9.0               9.0
	// 1600      2010     10.0              10.0
	// 1700      2012     11.0              11.0
	// 1800      2013     12.0              12.0
	// 1900      2015     14.0              14.0
	// 1910      2017     15.0              14.10
	// 1910      2017     15.1              14.10
	// 1910      2017     15.2              14.10
	// 1911      2017     15.3.x            14.11
	// 1911      2017     15.4.x            14.11
	// 1912      2017     15.5.x            14.12
	// 1913      2017     15.6.x            14.13
	// 1914      2017     15.7.x            14.14
	// 1915      2017     15.8.x            14.15
	// 1916      2017     15.9.x            14.16
	// 1920      2019     16.0.x            14.20
	// 1921      2019     16.1.x            14.21
	// 1929      2019     16.11.x           14.29
	// 1930      2022     17.0.x            14.30
	// 1936      2022     17.6.x            14.36
	msc_ver = (_MSC_FULL_VER / 10000000);
	msc_low_ver = (_MSC_FULL_VER / 100000) % 100;
	if (msc_ver < 19) {
		vs_ver = msc_ver - 6;
	}
	else {
		vs_ver = msc_ver - 5;
	}

	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				vs_ver,
				msc_low_ver);
	strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
	if (_MSC_FULL_VER % 100000) {
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " build %d",
					_MSC_FULL_VER % 100000);
		strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
	}
#elif defined(_MSC_VER)
	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				(_MSC_VER / 100) - 6,
				_MSC_VER % 100);
	strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
#endif
}

#elif defined(__MINGW32__)
static void GetCompilerInfo(char *buf, size_t buf_size)
{
#if defined(__GNUC__) || defined(__clang__)
	_snprintf_s(buf, buf_size, _TRUNCATE,
				"mingw " __MINGW64_VERSION_STR " "
#if defined(__clang__)
				"clang " __clang_version__
#elif defined(__GNUC__)
				"gcc " __VERSION__
#endif
		);
#else
	strncat_s(buf, buf_size, "mingw", _TRUNCATE);
#endif
}

#else
static void GetCompilerInfo(char *buf, size_t buf_size)
{
	strncpy_s(buf, buf_size, "unknown compiler");
}
#endif

#if defined(WDK_NTDDI_VERSION)
// �r���h�����Ƃ��Ɏg��ꂽ SDK �̃o�[�W�������擾����
// URL: https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/
// �o�[�W�����ԍ��ɂ�
// (1) Visual Studio �Ńv���W�F�N�g�̃v���p�e�B�� "Windows SDK �o�[�W����" �ɗ񋓂����o�[�W����
// (2) �C���X�g�[�����ꂽ SDK ���u�A�v���Ƌ@�\�v�ŕ\�������o�[�W����
// (3) ��L URL �ł̕\���o�[�W����
// (4) �C���X�g�[����t�H���_��
// �����邪�A�Ō�̃u���b�N�̐����͓����ɂȂ�Ȃ����Ƃ������B
// e.g. (1) 10.0.18362.0, (2) 10.0.18362.1, (3) 10.0.18362.1, (4) 10.0.18362.0
//      (1) 10.0.22000.0, (2) 10.0.22000.832, (3) 10.0.22000.832, (4) 10.0.22000.0
static void GetSDKInfo(char *buf, size_t buf_size)
{
	if (WDK_NTDDI_VERSION >= 0x0A00000B) {
		strncpy_s(buf, buf_size, "Windows SDK", _TRUNCATE);
		switch (WDK_NTDDI_VERSION) {
			case 0x0A00000B: // NTDDI_WIN10_CO
				             // 10.0.22000.194 �����݂��邪����ł��Ȃ�
				strncat_s(buf, buf_size, " for Windows 11 (10.0.22000.832)", _TRUNCATE);
				break;
			case 0x0A00000C: // NTDDI_WIN10_NI
				             // 10.0.22621.1 �����݂��邪����ł��Ȃ�
				             // (2)�C���X�g�[��, (3)URL �ł͕ʈ����� 10.0.22621.1778 ����������ɂȂ�
				             // AppVeyor �Ŏg���Ă���͂��̃o�[�W�����ԍ���Ԃ�
				strncat_s(buf, buf_size, " for Windows 11 (10.0.22621.755)", _TRUNCATE);
				break;
			default: {
				char str[32];
				sprintf_s(str, sizeof(str), " (NTDDI_VERSION=0x%08X)",
				          WDK_NTDDI_VERSION);
				strncat_s(buf, buf_size, str, _TRUNCATE);
				break;
			}
		}
	}
	else if (WDK_NTDDI_VERSION >= 0x0A000000) {
		strncpy_s(buf, buf_size, "Windows 10 SDK", _TRUNCATE);
		switch (WDK_NTDDI_VERSION) {
			case 0x0A000000: // NTDDI_WIN10, 1507
				strncat_s(buf, buf_size, " (10.0.10240.0)", _TRUNCATE);
				break;
			case 0x0A000001: // NTDDI_WIN10_TH2, 1511
				strncat_s(buf, buf_size, " Version 1511 (10.0.10586.212)", _TRUNCATE);
				break;
			case 0x0A000002: // NTDDI_WIN10_RS1, 1607
				strncat_s(buf, buf_size, " Version 1607 (10.0.14393.795)", _TRUNCATE);
				break;
			case 0x0A000003: // NTDDI_WIN10_RS2, 1703
				strncat_s(buf, buf_size, " Version 1703 (10.0.15063.468)", _TRUNCATE);
				break;
			case 0x0A000004: // NTDDI_WIN10_RS3, 1709
				strncat_s(buf, buf_size, " Version 1709 (10.0.16299.91)", _TRUNCATE);
				break;
			case 0x0A000005: // NTDDI_WIN10_RS4, 1803
				strncat_s(buf, buf_size, " Version 1803 (10.0.17134.12)", _TRUNCATE);
				break;
			case 0x0A000006: // NTDDI_WIN10_RS5, 1809
				strncat_s(buf, buf_size, " Version 1809 (10.0.17763.132)", _TRUNCATE);
				break;
			case 0x0A000007: // NTDDI_WIN10_19H1, 1903
				strncat_s(buf, buf_size, " Version 1903 (10.0.18362.1)", _TRUNCATE);
				break;
			case 0x0A000008: // NTDDI_WIN10_VB, 2004
				strncat_s(buf, buf_size, " Version 2004 (10.0.19041.685)", _TRUNCATE);
				break;
			case 0x0A000009: // NTDDI_WIN10_MN, 2004? cf. _PCW_REGISTRATION_INFORMATION
				strncat_s(buf, buf_size, " Version 2004 (10.0.19645.0)", _TRUNCATE);
				break;
			case 0x0A00000A: // NTDDI_WIN10_FE, 2104
				strncat_s(buf, buf_size, " Version 2104 (10.0.20348.1)", _TRUNCATE);
				break;
			default:
				strncat_s(buf, buf_size, " (unknown)", _TRUNCATE);
				break;
		}
	}
	else {
		strncpy_s(buf, buf_size, "Windows SDK unknown", _TRUNCATE);
	}
}
#else
static void GetSDKInfo(char *buf, size_t buf_size)
{
	strncpy_s(buf, buf_size, "Windows SDK unknown", _TRUNCATE);
}
#endif

static INT_PTR CALLBACK AboutDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_ABOUT_TITLE" },
	};
	char buf[128];
#if 0
	HDC hdc;
	HWND hwnd;
#endif
	static HICON dlghicon = NULL;

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
	// for animation
	static HDC dlgdc = NULL;
	static int dlgw, dlgh;
	static HBITMAP dlgbmp = NULL, dlgprevbmp = NULL;
	static LPDWORD dlgpixel = NULL;
	static HICON dlghicon = NULL;
	const int icon_x = 15, icon_y = 10, icon_w = 32, icon_h = 32;
	const int ID_EFFECT_TIMER = 1;
	RECT dlgrc = {0};
	BITMAPINFO       bmi;
	BITMAPINFOHEADER bmiHeader;
	int x, y;
#define POS(x,y) ((x) + (y)*dlgw)
	static short *wavemap = NULL;
	static short *wavemap_old = NULL;
	static LPDWORD dlgorgpixel = NULL;
	static int waveflag = 0;
	static int fullcolor = 0;
	int bitspixel;
#endif

	switch (Message) {
		case WM_INITDIALOG:
			// �A�C�R���𓮓I�ɃZ�b�g
			{
#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
				int fuLoad = LR_DEFAULTCOLOR;
				HICON hicon;
				if (IsWindowsNT4()) {
					fuLoad = LR_VGACOLOR;
				}
				hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_TTERM),
				                  IMAGE_ICON, icon_w, icon_h, fuLoad);
				// Picture Control �ɕ`�悷��ƁA�Ȃ������ߐF�����߂ɂȂ炸�A���ƂȂ��Ă��܂����߁A
				// WM_PAINT �ŕ`�悷��B
				dlghicon = hicon;
#else
				SetDlgItemIcon(Dialog, IDC_TT_ICON, MAKEINTRESOURCEW(IDI_TTERM), 0, 0);
#endif
			}

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

			// Tera Term �{�̂̃o�[�W����
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Version %d.%d ", TT_VERSION_MAJOR, TT_VERSION_MINOR);
			{
				char *substr = GetVersionSubstr();
				strncat_s(buf, sizeof(buf), substr, _TRUNCATE);
				free(substr);
			}
			SetDlgItemTextA(Dialog, IDC_TT_VERSION, buf);

			// Oniguruma�̃o�[�W����
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Oniguruma %s", onig_version());
			SetDlgItemTextA(Dialog, IDC_ONIGURUMA_LABEL, buf);

			// SFMT�̃o�[�W������ݒ肷��
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "SFMT %s", SFMT_VERSION);
			SetDlgItemTextA(Dialog, IDC_SFMT_VERSION, buf);

			// build info
			{
				// �R���p�C���A�����ASDK
				char *info;
				char tmpbuf[128];
				char sdk[128];
				GetCompilerInfo(tmpbuf, sizeof(tmpbuf));
				GetSDKInfo(sdk, _countof(sdk));
				asprintf(&info,
						 "Build info:\r\n"
						 "  Compiler: %s\r\n"
						 "  Date and Time: %s %s\r\n"
						 "  SDK: %s",
						 tmpbuf,
						 __DATE__, __TIME__,
						 sdk);

				SetDlgItemTextA(Dialog, IDC_BUILDTOOL, info);
				free(info);
			}

			TTTextURL(Dialog, IDC_AUTHOR_URL, NULL, NULL);

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
			/*
			 * �_�C�A���O�̃r�b�g�}�b�v�����s���A�w�i�ɃG�t�F�N�g����������悤�ɂ���B
			 * (2011.5.7 yutaka)
			 */
			// �_�C�A���O�̃T�C�Y
			GetWindowRect(Dialog, &dlgrc);
			dlgw = dlgrc.right - dlgrc.left;
			dlgh = dlgrc.bottom - dlgrc.top;
			// �r�b�g�}�b�v�̍쐬
			dlgdc = CreateCompatibleDC(NULL);
			ZeroMemory(&bmiHeader, sizeof(BITMAPINFOHEADER));
			bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
			bmiHeader.biWidth     = dlgw;
			bmiHeader.biHeight    = -dlgh;
			bmiHeader.biPlanes    = 1;
			bmiHeader.biBitCount  = 32;
			bmi.bmiHeader = bmiHeader;
			dlgbmp = CreateDIBSection(NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &dlgpixel, NULL, 0);
			dlgprevbmp = (HBITMAP)SelectObject(dlgdc, dlgbmp);
			// �r�b�g�}�b�v�̔w�i�F�i���Ă����ۂ��j�����B
			for (y = 0 ; y < dlgh ; y++) {
				double dx = (double)(255 - 180) / dlgw;
				double dy = (double)255/dlgh;
				BYTE r, g, b;
				for (x = 0 ; x < dlgw ; x++) {
					r = min((int)(180+dx*x), 255);
					g = min((int)(180+dx*x), 255);
					b = max((int)(255-y*dx), 0);
					// ��f�̕��т́A���ʃo�C�g����B, G, R, A�ƂȂ�B
					dlgpixel[POS(x, y)] = b | g << 8 | r << 16;
				}
			}
			// 2D Water effect �p
			wavemap = calloc(sizeof(short), dlgw * dlgh);
			wavemap_old = calloc(sizeof(short), dlgw * dlgh);
			dlgorgpixel = calloc(sizeof(DWORD), dlgw * dlgh);
			memcpy(dlgorgpixel, dlgpixel, dlgw * dlgh * sizeof(DWORD));

			srand((unsigned int)time(NULL));


#ifdef EFFECT_ENABLED
			// �G�t�F�N�g�^�C�}�[�̊J�n
			SetTimer(Dialog, ID_EFFECT_TIMER, 100, NULL);
#endif

			// ��ʂ̐F���𒲂ׂ�B
			hwnd = GetDesktopWindow();
			hdc = GetDC(hwnd);
			bitspixel = GetDeviceCaps(hdc, BITSPIXEL);
			fullcolor = (bitspixel == 32 ? 1 : 0);
			ReleaseDC(hwnd, hdc);
#endif

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
#ifdef EFFECT_ENABLED
			switch (LOWORD(wParam)) {
				int val;
				case IDOK:
					val = 1;
				case IDCANCEL:
					val = 0;
					KillTimer(Dialog, ID_EFFECT_TIMER);

					SelectObject(dlgdc, dlgprevbmp);
					DeleteObject(dlgbmp);
					DeleteDC(dlgdc);
					dlgdc = NULL;
					dlgprevbmp = dlgbmp = NULL;

					free(wavemap);
					free(wavemap_old);
					free(dlgorgpixel);

					TTEndDialog(Dialog, val);
					return TRUE;
			}
#else
			switch (LOWORD(wParam)) {
				case IDOK:
					TTEndDialog(Dialog, 1);
					return TRUE;
				case IDCANCEL:
					TTEndDialog(Dialog, 0);
					return TRUE;
			}
#endif
			break;

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
		// static text�̔w�i�𓧉߂�����B
		case WM_CTLCOLORSTATIC:
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (BOOL)GetStockObject( NULL_BRUSH );
			break;

#ifdef EFFECT_ENABLED
		case WM_ERASEBKGND:
			return 0;
#endif

		case WM_PAINT:
			if (dlgdc) {
				PAINTSTRUCT ps;
				hdc = BeginPaint(Dialog, &ps);

				if (fullcolor) {
					BitBlt(hdc,
						ps.rcPaint.left, ps.rcPaint.top,
						ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
						dlgdc,
						ps.rcPaint.left, ps.rcPaint.top,
						SRCCOPY);
				}

				DrawIconEx(hdc, icon_x, icon_y, dlghicon, icon_w, icon_h, 0, 0, DI_NORMAL);

				EndPaint(Dialog, &ps);
			}
			break;

		case WM_MOUSEMOVE:
			{
				int xpos, ypos;
				static int idx = 0;
				short amplitudes[4] = {250, 425, 350, 650};

				xpos = LOWORD(lParam);
				ypos = HIWORD(lParam);

				wavemap[POS(xpos,ypos)] = amplitudes[idx++];
				idx %= 4;
			}
			break;

		case WM_TIMER:
			if (wParam == ID_EFFECT_TIMER)
			{
				int x, y;
				short height, xdiff;
				short *p_new, *p_old;

				if (waveflag == 0) {
					p_new = wavemap;
					p_old = wavemap_old;
				} else {
					p_new = wavemap_old;
					p_old = wavemap;
				}
				waveflag ^= 1;

				// ���ʂ̌v�Z
				// �A���S���Y���͉��L�T�C�g(2D Water)���B
				// cf. http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
				for (y = 1; y < dlgh - 1 ; y++) {
					for (x = 1; x < dlgw - 1 ; x++) {
						height = (p_new[POS(x,y-1)] +
							      p_new[POS(x-1,y)] +
							      p_new[POS(x+1,y)] +
							      p_new[POS(x,y+1)]) / 2 - p_old[POS(x,y)];
						height -= (height >> 5);
						p_old[POS(x,y)] = height;
					}
				}

				// ���ʂ̕`��
				for (y = 1; y < dlgh - 1 ; y++) {
					for (x = 1; x < dlgw - 1 ; x++) {
						xdiff = p_old[POS(x+1,y)] - p_old[POS(x,y)];
						dlgpixel[POS(x,y)] = dlgorgpixel[POS(x + xdiff, y)];
					}
				}

#if 0
				hdc = GetDC(Dialog);
				BitBlt(hdc,
					0, 0, dlgw, dlgh,
					dlgdc,
					0, 0,
					SRCCOPY);
				ReleaseDC(Dialog, hdc);
#endif

				InvalidateRect(Dialog, NULL, FALSE);
			}
			break;
#endif
		case WM_DPICHANGED:
			SendDlgItemMessage(Dialog, IDC_TT_ICON, Message, wParam, lParam);
			break;

		case WM_DESTROY:
			DestroyIcon(dlghicon);
			dlghicon = NULL;
			break;
	}
	return FALSE;
}

BOOL WINAPI _AboutDialog(HWND WndParent)
{
	return
		(BOOL)TTDialogBox(hInst,
						  MAKEINTRESOURCE(IDD_ABOUTDLG),
						  WndParent, AboutDlg);
}
