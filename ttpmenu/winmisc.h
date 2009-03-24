#ifndef WINMISC_H
#define	WINMISC_H
/* @(#)Copyright (C) NTT-IT 1998-2002   -- winmisc.h -- Ver1.00b2 */
/* ========================================================================
	Project  Name			: Universal Library
	Outline					: WinMisc function Header
	Create					: 1998-02-20(Wed)
	Update					: 2002-08-13(Tue)
	Copyright				: S.Hayakawa				NTT-IT
    Reference				: 
	======================================================================== */
#include	<windows.h>
#include	<tchar.h>

#include	<shlobj.h>
#include	<shellapi.h>

#include	<stdio.h>
#include	<time.h>
#include	"i18n.h"

// Window Position
#define		POSITION_LEFTTOP		0x00
#define		POSITION_LEFTBOTTOM		0x01
#define		POSITION_RIGHTTOP		0x02
#define		POSITION_RIGHTBOTTOM	0x03
#define		POSITION_CENTER			0x04
#define		POSITION_OUTSIDE		0x05

// misc
void	GetTime(TCHAR *cTimeStr);
void	GetTimeEx(LPTSTR lpszTimeStr);
void	SetDlgPos(HWND hWnd, int pos);
void	EncodePassword(TCHAR *cPassword, TCHAR *cEncodePassword);
BOOL	EnableItem(HWND hWnd, int idControl, BOOL flag);
BOOL	OpenFileDlg(HWND hWnd, UINT editCtl, TCHAR *title, TCHAR *filter, TCHAR *defaultDir);
BOOL	SaveFileDlg(HWND hWnd, UINT editCtl, TCHAR *title, TCHAR *filter, TCHAR *defaultDir);
BOOL	BrowseForFolder(HWND hWnd, TCHAR *szTitle, TCHAR *szPath);
BOOL	GetModulePath(TCHAR *szPath, DWORD dwMaxPath);
BOOL	SetForceForegroundWindow(HWND hWnd);
UINT	GetResourceType(LPCTSTR lpszPath);
int		CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
TCHAR	*PathTok(TCHAR *str, TCHAR *separator);
TCHAR	*lstrstri(TCHAR *s1, TCHAR *s2);
void	UTIL_get_lang_msg(PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile);
int		UTIL_get_lang_font(PCHAR key, HWND dlg, PLOGFONT logfont, HFONT *font, PCHAR iniFile);

#endif
