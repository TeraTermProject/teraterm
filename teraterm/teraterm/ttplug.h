/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) Robert O'Callahan
 * (C) 2008- TeraTerm Project
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
#ifndef __TTPLUG_H
#define __TTPLUG_H

#include "hostname_rec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This function initializes the extensions and is called at the beginning
   of the program. */
void PASCAL TTXInit(PTTSet ts, PComVar cv);

/* This function is called when a TCP connection is about to be opened.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL TTXOpenTCP(void);

/* This function is called when a TCP connection has been closed.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL TTXCloseTCP(void);

void PASCAL TTXOpenFile(void);

void PASCAL TTXCloseFile(void);

/* This function is called after the TTDLG DLL has been loaded.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL TTXGetUIHooks(void);

/* This function is called after the TTSET DLL has been loaded.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL TTXGetSetupHooks(void);

/* This function is called when the window size has changed. */
void PASCAL TTXSetWinSize(int rows, int cols);

/* This function adds the extensions' entries to the menu, which is the
   handle for the program's menubar. */
void PASCAL TTXModifyMenu(HMENU menu);

/* This function is called when a popup menu is about to be displayed.
   The status of the entries is set appropriately. */
void PASCAL TTXModifyPopupMenu(HMENU menu);

/* This function calls on the extensions to handle a command. It returns
   TRUE if they handle it, otherwise FALSE. */
BOOL PASCAL TTXProcessCommand(HWND hWin, WORD cmd);

/* This function is called to see whether Telnet mode can be turned on when
   Tera Term thinks it has detected a telnetd */
void PASCAL TTXEnd(void);

/* This function is called when a new Tera Term is being started with certain
   settings and the extension may wish to add some options to the command line */
void PASCAL TTXSetCommandLine(wchar_t *cmd, int cmdlen, PGetHNRec rec);

/**
 * �v���O�C���̏��ɂ��Ă�API
 */

typedef enum {
	EXTENSION_DISABLE = 0,
	EXTENSION_ENABLE = 1,
	EXTENSION_UNSPECIFIED = 2,	// ���w��(�L��/�����̎w��Ȃ�,���[�h����)
} ExtensionEnable;

typedef enum {
	EXTENSION_UNLOADED = 0,		// ���[�h����Ă��Ȃ�
	EXTENSION_LOADED = 1,		// ���[�h����Ă���
} ExtensionLoaded;

/**
 * �v���O�C�����
 */
typedef struct {
	wchar_t *filename;			// �t�@�C����
	ExtensionEnable enable;
	ExtensionLoaded loaded;		// (Add���͖���)
	int load_order;				// ���[�h�� (Add���͖���)
} PluginInfo;

/**
 * �v���O�C�������擾����
 * @param index		�v���O�C���̃C���f�b�N�X
 * @param info		�v���O�C�������i�[����\���̂ւ̃|�C���^
 *					������͏��������Ȃ�����
 * @return			���������ꍇ��TRUE�A���s�����ꍇ��FALSE
 */
BOOL PluginGetInfo(int index, PluginInfo *info);

/**
 * �v���O�C������ݒ肷��
 *
 * �����t�@�C����������ΕύX����
 *
 * @param info		�v���O�C�������i�[����\���̂ւ̃|�C���^
 */
void PluginAddInfo(const PluginInfo *info);

/**
 * �v���O�C���̐ݒ��ۑ�
 *
 * @param SetupFNW	�ݒ�t�@�C����
 */
void PluginWriteList(const wchar_t *SetupFNW);

#ifdef __cplusplus
}
#endif

#endif
