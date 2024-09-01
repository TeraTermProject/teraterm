/*
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

/* Tera Term Text */

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TTTextSt TTText;

/**
 *	TTTextURL
 *	�X�^�e�B�b�N�e�L�X�g�N���b�N��URL���J��
 *
 *	@param	hDlg		�_�C�A���O
 *	@param	id			static text id
 *	@param	text		�\������text, NULL�̂Ƃ��� static text �ɐݒ肵�Ă���text���g�p����
 *	@param	url			�N���b�N���ꂽ�Ƃ��J��URL, NULL�̂Ƃ��͕\��text���J��
 */
TTText *TTTextURL(HWND hDlg, int id, const wchar_t *text, const char *url);

/**
 *	TTTextMenu
 *	�X�^�e�B�b�N�e�L�X�g���烁�b�Z�[�W�𑗐M
 *
 *	@param	hDlg		�_�C�A���O
 *	@param	id			static text id
 *	@param	text		�\������text, NULL�̂Ƃ��� static text �ɐݒ肵�Ă���text���g�p����
 *	@param	menu_wnd	�N���b�N���ꂽ�Ƃ����M����E�B���h�E
 *	@param	menu_id		�N���b�N���ꂽ�Ƃ����M���郁�b�Z�[�WID
 */
TTText *TTTextMenu(HWND hDlg, int id, const wchar_t *text, HWND menu_wnd, int menu_id);

#ifdef __cplusplus
}
#endif
