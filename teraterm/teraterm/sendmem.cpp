/*
 * (C) 2019- TeraTerm Project
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
#include <stdio.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttcommon.h"
#include "ftdlg_lite.h"
#include "checkeol.h"

#include "ttwinman.h"		// for ts
#include "codeconv.h"

#define	SENDMEM_USE_OLD_API	0

#if SENDMEM_USE_OLD_API
#include "filesys.h"		// for FileSendStart()
#else
#include "fileread.h"
#endif

#include "sendmem.h"

typedef enum {
	SendMemTypeText,
	SendMemTypeBinary,
} SendMemType;

// ���M����VTWIN�ɔr����������
#define	USE_ENABLE_WINDOW	0	// 1=�r������

typedef struct SendMemTag {
	const BYTE *send_ptr;  // ���M�f�[�^�ւ̃|�C���^
	size_t send_len;	   // ���M�f�[�^�T�C�Y
	SendMemType type;
	BOOL local_echo_enable;
	BOOL send_host_enable;
	DWORD delay_per_line;  // (ms)
	DWORD delay_per_char;
	DWORD delay_per_sendsize;
	DWORD delay_tick;
	size_t send_size_max;	// ���M�T�C�Y�f�B���C�A���M�T�C�Y
	SendMemDelayType delay_type;
	HWND hWnd;	 // �^�C�}�[���󂯂�window
	int timer_id;  // �^�C�}�[ID
	wchar_t *UILanguageFile;
	wchar_t *dialog_caption;
	wchar_t *filename;
	void (*callback)(void *data);
	void *callback_data;
	//
	size_t send_left;
	size_t send_index;
	BOOL waited;
	DWORD last_send_tick;
	//
	CFileTransLiteDlg *dlg;
	class SendMemDlgObserver *dlg_observer;
	HINSTANCE hInst_;
	HWND hWndParent_;
	//
	PComVar cv_;
	BOOL pause;
	CheckEOLData_t *ceol;
} SendMem;

typedef SendMem sendmem_work_t;

extern "C" IdTalk TalkStatus;
extern "C" HWND HVTWin;

static sendmem_work_t *sendmem_work;

class SendMemDlgObserver : public CFileTransLiteDlg::Observer {
public:
	SendMemDlgObserver(HWND hWndParent, void (*OnClose)(), void (*OnPause)(BOOL paused)) {
		hWndParent_ = hWndParent;
		OnClose_ = OnClose;
		OnPause_ = OnPause;
	}
private:
	void OnClose()
	{
		OnClose_();
	};
	void OnPause(BOOL paused)
	{
		OnPause_(paused);
	};
	void OnHelp()
	{
		MessageBoxA(hWndParent_, "no help topic", "Tera Term", MB_OK);
	}
	HWND hWndParent_;
	void (*OnClose_)();
	void (*OnPause_)(BOOL paused);
};

static wchar_t *wcsnchr(const wchar_t *str, size_t len, wchar_t chr)
{
	for (size_t i = 0; i < len; ++i) {
		wchar_t c = *str;
		if (c == 0) {
			return NULL;
		}
		if (c == chr) {
			return (wchar_t *)str;
		}
		str++;
	}
	return NULL;
}

static void EndPaste()
{
	sendmem_work_t *p = sendmem_work;

	if (p->callback != NULL) {
		p->callback(p->callback_data);
		p->callback = NULL;
	}

	free((void *)p->send_ptr);
	p->send_ptr = NULL;

	TalkStatus = IdTalkKeyb;
	if (p->dlg != NULL) {
		p->dlg->Destroy();
		delete p->dlg;
		delete p->dlg_observer;
	}
	free(p->dialog_caption);
	free(p->filename);
	SendMemFinish(p);

	sendmem_work = NULL;

	// ����ł���悤�ɂ���
#if USE_ENABLE_WINDOW
	EnableWindow(HVTWin, TRUE);
	SetFocus(HVTWin);
#endif
}

static void OnClose()
{
	EndPaste();
}

static void OnPause(BOOL paused)
{
	sendmem_work_t *p = sendmem_work;
	p->pause = paused;
	if (!paused) {
		// �|�[�Y������, �^�C�}�[�C�x���g�ōđ��̃g���K������
		SetTimer(p->hWnd, p->timer_id, 0, NULL);
	}
}

static void SendMemStart_i(SendMem *sm)
{
	sendmem_work = sm;
	sendmem_work_t *p = sm;

	p->send_left = p->send_len;
	p->send_index = 0;
	p->waited = FALSE;
	p->pause = FALSE;

	if (p->hWndParent_ != NULL) {
		// �_�C�A���O�𐶐�����
		p->dlg = new CFileTransLiteDlg();
		p->dlg->Create(NULL, NULL, sm->UILanguageFile);
		if (p->dialog_caption != NULL) {
			p->dlg->SetCaption(p->dialog_caption);
		}
		if (p->filename != NULL) {
			p->dlg->SetFilename(p->filename);
		}
		p->dlg_observer = new SendMemDlgObserver(p->hWndParent_, OnClose, OnPause);
		p->dlg->SetObserver(p->dlg_observer);
	}

	TalkStatus = IdTalkSendMem;

	// ���M�J�n���ɃE�B���h�E�𑀍�ł��Ȃ��悤�ɂ���
#if USE_ENABLE_WINDOW
	EnableWindow(HVTWin, FALSE);
#endif
}

/**
 *	���M�o�b�t�@�̏��擾
 *	@param[out]		use		�g�pbyte
 *	@param[out]		free	��byte
 */
static void GetOutBuffInfo(const TComVar *cv_, size_t *use, size_t *free)
{
	if (use != NULL) {
		*use = cv_->OutBuffCount;
	}
	if (free != NULL) {
		*free = OutBuffSize - cv_->OutBuffCount;
	}
}

/**
 *	��M�o�b�t�@�̏��擾
 *	@param[out]		use		�g�pbyte
 *	@param[out]		free	��byte
 */
static void GetInBuffInfo(const TComVar *cv_, size_t *use, size_t *free)
{
	if (use != NULL) {
		*use = cv_->InBuffCount;
	}
	if (free != NULL) {
		*free = InBuffSize - cv_->InBuffCount;
	}
}

/**
 *	�o�b�t�@�̋󂫃T�C�Y�𒲂ׂ�
 */
static size_t GetBufferFreeSpece(sendmem_work_t *p)
{
	size_t buff_len = 0;

	// ���M�o�b�t�@�̋󂫃T�C�Y
	if (p->send_host_enable) {
		size_t out_buff_free;
		GetOutBuffInfo(p->cv_, NULL, &out_buff_free);
		buff_len = out_buff_free;
	}

	if (p->local_echo_enable) {
		// ��M�o�b�t�@�̋󂫃T�C�Y
		size_t in_buff_free;
		GetInBuffInfo(p->cv_, NULL, &in_buff_free);

		if (p->send_host_enable) {
			// ���M+���[�J���G�R�[�̏ꍇ
			if (buff_len > in_buff_free) {
				// �o�b�t�@�̏��������ɍ��킹��
				buff_len = in_buff_free;
			}
		}
		else {
			// ���[�J���G�R�[�����̏ꍇ
			buff_len = in_buff_free;
		}
	}
	return buff_len;
}

/**
 * ���M
 */
void SendMemContinuously(void)
{
	sendmem_work_t *p = sendmem_work;

	if (p->send_ptr == NULL) {
		EndPaste();
		return;
	}

	if (p->pause) {
		return;
	}

	// �I�[?
	if (p->send_left == 0) {
		// �I��, ���M�o�b�t�@����ɂȂ�܂ő҂�
		size_t out_buff_use;
		GetOutBuffInfo(p->cv_, &out_buff_use, NULL);

		if (p->dlg != NULL) {
			p->dlg->RefreshNum(p->send_index, p->send_len - out_buff_use);
		}

		if (out_buff_use == 0) {
			// ���M�o�b�t�@����ɂȂ���
			EndPaste();
			return;
		}
	}

	if (p->waited) {
		if (GetTickCount() - p->last_send_tick < p->delay_tick) {
			// �E�G�C�g����
			return;
		}
	}

	// ���M�ł���o�b�t�@�T�C�Y(�o�b�t�@�̋󂫃T�C�Y)
	size_t buff_len = GetBufferFreeSpece(p);
	if (buff_len == 0) {
		// �o�b�t�@�ɋ󂫂��Ȃ�
		return;
	}

	// ���M��
	BOOL need_delay = FALSE;
	size_t send_len;
	if (p->delay_per_char > 0) {
		// 1�L�����N�^���M
		need_delay = TRUE;
		if (p->type == SendMemTypeBinary) {
			send_len = 1;
		}
		else {
			const wchar_t *send_ptr = (wchar_t *)&p->send_ptr[p->send_index];
			if (!IsHighSurrogate(*send_ptr)) {
				send_len = sizeof(wchar_t);
			}
			else {
				if (IsLowSurrogate(*(send_ptr + 1))) {
					send_len = 2 * sizeof(wchar_t);
				}
				else {
					// TODO, �T���Q�[�g�y�A�ɂȂ��Ă��Ȃ�����
					send_len = sizeof(wchar_t);
				}
			}
		}
	}
	else if (p->delay_per_line > 0) {
		// 1���C�����M
		need_delay = TRUE;

		const wchar_t *line_top = (wchar_t *)&p->send_ptr[p->send_index];
		const size_t send_left_char = p->send_left / sizeof(wchar_t);
		BOOL eos = TRUE;

		// ���s��T��
		const wchar_t *s = line_top;
		for (size_t i = 0; i < send_left_char; ++i) {
			const wchar_t c = *s;
			CheckEOLRet r = CheckEOLCheck(p->ceol, c);
			if ((r & CheckEOLOutputEOL) != 0) {
				// ���s����������
				if ((r & CheckEOLOutputChar) != 0) {
					// ���s�̎��̕����܂Ői��
					s--;
				}
				eos = FALSE;
				break;
			}
			s++;
		}

		// ���M������
		if (eos == FALSE) {
			// ���s�܂ő��M
			send_len = (s - line_top + 1) * sizeof(wchar_t);
		}
		else {
			// ���s��������Ȃ������A�Ō�܂ő��M
			send_len = p->send_left;
		}

		// ���M�����������M�o�b�t�@���傫��
		if (buff_len < send_len) {
			// ���M�o�b�t�@���܂Ő؂�l�߂�
			send_len = buff_len;
			CheckEOLClear(p->ceol);
			return;
		}
	}
	else if (p->send_size_max != 0) {
		// ���M�T�C�Y���
		send_len = p->send_left;
		if (send_len > p->send_size_max) {
			need_delay = TRUE;
			send_len = p->send_size_max;
		}
		if (p->type == SendMemTypeText) {
			// ���M�f�[�^��������(wchar_t��)�ɂ���
			send_len = send_len & (~1);
		}
	}
	else {
		// �S�͑��M
		send_len = p->send_left;
		if (buff_len < send_len) {
			send_len = buff_len;
		}
		if (p->type == SendMemTypeText) {
			// ���M�f�[�^��������(wchar_t��)�ɂ���
			send_len = send_len & (~1);
		}
	}

	// ���M����
	if (p->type == SendMemTypeBinary) {
		const BYTE *send_ptr = (BYTE *)&p->send_ptr[p->send_index];
		if (p->send_host_enable) {
			CommBinaryBuffOut(p->cv_, (PCHAR)send_ptr, (int)send_len);
		}
		if (p->local_echo_enable) {
			CommBinaryEcho(p->cv_, (PCHAR)send_ptr, (int)send_len);
		}
	}
	else {
		const wchar_t *str_ptr = (wchar_t *)&p->send_ptr[p->send_index];
		int str_len = (int)(send_len / sizeof(wchar_t));
		if (p->send_host_enable) {
			CommTextOutW(p->cv_, str_ptr, str_len);
		}
		if (p->local_echo_enable) {
			CommTextEchoW(p->cv_, str_ptr, str_len);
		}
	}

	// ���M����ɂ����߂�
	assert(send_len <= p->send_left);
	p->send_index += send_len;
	p->send_left -= send_len;

	// �_�C�A���O�X�V
	if (p->dlg != NULL) {
		size_t out_buff_use;
		GetOutBuffInfo(p->cv_, &out_buff_use, NULL);
		p->dlg->RefreshNum(p->send_index - out_buff_use, p->send_len);
	}

	if (p->send_left != 0 && need_delay) {
		// wait�ɓ���
		p->waited = TRUE;
		p->last_send_tick = GetTickCount();
		// �^�C�}�[��idle�𓮍삳���邽�߂Ɏg�p���Ă���
		SetTimer(p->hWnd, p->timer_id, p->delay_tick, NULL);
	}
}

/**
 *	������
 */
static SendMem *SendMemInit_()
{
	if (sendmem_work != NULL) {
		// ���M��
		return NULL;
	}
	SendMem *p = (SendMem *)calloc(sizeof(*p), 1);
	if (p == NULL) {
		return NULL;
	}

	p->send_ptr = NULL;
	p->send_len = 0;

	p->type = SendMemTypeBinary;
	p->local_echo_enable = FALSE;
	p->send_host_enable = TRUE;
	p->delay_type = SENDMEM_DELAYTYPE_NO_DELAY;
	p->send_size_max = 0;
	p->delay_per_char = 0;  // (ms)
	p->delay_per_line = 0;  // (ms)
	p->delay_per_sendsize = 0;
	p->cv_ = NULL;
	p->hWnd = HVTWin;		// delay���Ɏg�p����^�C�}�[�p
	p->timer_id = IdPasteDelayTimer;
	p->hWndParent_ = NULL;
	p->ceol = CheckEOLCreate(CheckEOLTypeFile);
	return p;
}

/**
 *	�������ɂ���e�L�X�g�𑗐M����
 *	�e�L�X�g�͑��M�I�����free()�����
 *  �����̕ϊ��͍s���Ȃ�
 *	CR�͉��̑w�ő��M���ɐݒ肵�����s�R�[�h�ɕϊ������
 *  LF�͂��̂܂ܑ��M�����
 *
 *	@param	str		�e�L�X�g�փ|�C���^(malloc()���ꂽ�̈�)
 *					���M��(���f��)�A�����I��free()�����
 *	@param	len		������(wchar_t�P��)
 *					L'\0' �͑��M����Ȃ�
 */
SendMem *SendMemTextW(wchar_t *str, size_t len)
{
	SendMem *p = SendMemInit_();
	if (p == NULL) {
		return NULL;
	}

	if (len == 0) {
		// L'\0' �͑��M���Ȃ��̂� +1 �͕s�v
		len = wcslen(str);
	}

	// remove EOS
	if (str[len-1] == 0) {
		len--;
		if (len == 0){
			SendMemFinish(p);
			return NULL;
		}
	}
	p->send_ptr = (BYTE *)str;
	p->send_len = len * sizeof(wchar_t);
	p->type = SendMemTypeText;
	return p;
}

/**
 *	�������ɂ���f�[�^�𑗐M����
 *	�f�[�^�͑��M�I�����free()�����
 *
 *	@param	ptr		�f�[�^�փ|�C���^(malloc()���ꂽ�̈�)
 *					���M��(���f��)�A�����I��free()�����
 *	@param	len		�f�[�^��(byte)
 */
SendMem *SendMemBinary(void *ptr, size_t len)
{
	SendMem *p = SendMemInit_();
	if (p == NULL) {
		return NULL;
	}

	p->send_ptr = (BYTE *)ptr;
	p->send_len = len;
	p->type = SendMemTypeBinary;
	return p;
}

/**
 *	���[�J���G�R�[
 *
 *	@param	echo	FALSE	�G�R�[���Ȃ�(�f�t�H���g)
 *					TRUE	�G�R�[����
 */
void SendMemInitEcho(SendMem *sm, BOOL echo)
{
	sm->local_echo_enable = echo;
}

/**
 *	�z�X�g(�ڑ���)�֑��M����
 *
 *	@param	send_host	TRUE	���M����(�f�t�H���g)
 *						FALSE	���M���Ȃ�
 */
void SendMemInitSend(SendMem *sm, BOOL send_host)
{
	sm->send_host_enable = send_host;
}

void SendMemInitDelay(SendMem *sm, SendMemDelayType type, DWORD delay_tick, size_t send_max)
{
	switch (type) {
	case SENDMEM_DELAYTYPE_NO_DELAY:
	default:
		sm->delay_per_char = 0;
		sm->delay_per_line = 0;
		sm->send_size_max = 0;
		break;
	case SENDMEM_DELAYTYPE_PER_CHAR:
		sm->delay_per_char = delay_tick;
		sm->delay_per_line = 0;
		sm->delay_per_sendsize = 0;
		sm->send_size_max = 0;
		break;
	case SENDMEM_DELAYTYPE_PER_LINE:
		sm->delay_per_char = 0;
		sm->delay_per_line = delay_tick;
		sm->delay_per_sendsize = 0;
		sm->send_size_max = 0;
		break;
	case SENDMEM_DELAYTYPE_PER_SENDSIZE:
		sm->delay_per_char = 0;
		sm->delay_per_line = 0;
		sm->delay_per_sendsize = delay_tick;
		sm->send_size_max = send_max;
		break;
	}
	sm->delay_tick = delay_tick;
}

/**
 *	���M�����ŃR�[���o�b�N
 */
void SendMemInitSetCallback(SendMem *sm, void (*callback)(void *data), void *callback_data)
{
	sm->callback = callback;
	sm->callback_data = callback_data;
}

// �Z�b�g����ƃ_�C�A���O���o��
void SendMemInitDialog(SendMem *sm, HINSTANCE hInstance, HWND hWndParent, const wchar_t *UILanguageFile)
{
	sm->hInst_ = hInstance;
	sm->hWndParent_ = hWndParent;
	sm->UILanguageFile = _wcsdup(UILanguageFile);
}

void SendMemInitDialogCaption(SendMem *sm, const wchar_t *caption)
{
	if (sm->dialog_caption != NULL)
		free(sm->dialog_caption);
	sm->dialog_caption = _wcsdup(caption);
}

void SendMemInitDialogFilename(SendMem *sm, const wchar_t *filename)
{
	if (sm->filename != NULL)
		free(sm->filename);
	sm->filename = _wcsdup(filename);
}

extern "C" TComVar cv;
BOOL SendMemStart(SendMem *sm)
{
	// ���M���`�F�b�N
	if (sendmem_work != NULL) {
		return FALSE;
	}

	sm->cv_ = &cv;		// TODO �Ȃ�������
	SendMemStart_i(sm);
	return TRUE;
}

void SendMemFinish(SendMem *sm)
{
	CheckEOLDestroy(sm->ceol);
	sm->ceol = NULL;
	free(sm->UILanguageFile);
	free(sm);
}

/**
 *	�e�L�X�g�t�@�C���𑗐M����
 *	�����R�[�h,���s�R�[�h�͓K�؂ɕϊ������
 *
 *	@param[in]	filename	�t�@�C����
 *	@param[in]	binary		FALSE	text file
 *							TRUE	binary file
 */
#if SENDMEM_USE_OLD_API
BOOL SendMemSendFile(const wchar_t *filename, BOOL binary, SendMemDelayType delay_type, DWORD delay_tick, size_t send_max)
{
	(void)delay_type;
	(void)delay_tick;
	(void)send_max;

	BOOL r = FileSendStart(filename, binary == FALSE ? 0 : 1);
	return r;
}
#else
SendMem *SendMemSendFileCom(const wchar_t *filename, BOOL binary, SendMemDelayType delay_type, DWORD delay_tick, size_t send_max)
{
	SendMem *sm;
	if (!binary) {
		size_t str_len;
		wchar_t *str_ptr = LoadFileWW(filename, &str_len);
		assert(str_ptr != NULL);
		if (str_ptr == NULL) {
			return NULL;
		}

		// ���s�� CR �݂̂ɐ��K��
		wchar_t *dest = NormalizeLineBreakCR(str_ptr, &str_len);
		free(str_ptr);
		str_ptr = dest;

		sm = SendMemTextW(str_ptr, str_len);
	}
	else {
		size_t data_len;
		unsigned char *data_ptr = LoadFileBinary(filename, &data_len);
		assert(data_ptr != NULL);
		if (data_ptr == NULL) {
			return NULL;
		}
		sm = SendMemBinary(data_ptr, data_len);
	}
	SendMemInitDialog(sm, hInst, HVTWin, ts.UILanguageFileW);
	SendMemInitDialogCaption(sm, L"send file");			// title
	SendMemInitDialogFilename(sm, filename);
	SendMemInitDelay(sm, delay_type, delay_tick, send_max);
	SendMemStart(sm);
	return sm;
}
#endif

BOOL SendMemSendFile(const wchar_t *filename, BOOL binary, SendMemDelayType delay_type, DWORD delay_tick, size_t send_max)
{
	SendMem *sm = SendMemSendFileCom(filename, binary, delay_type, delay_tick, send_max);
	return (sm != NULL) ? TRUE : FALSE;
}

BOOL SendMemSendFile2(const wchar_t *filename, BOOL binary, SendMemDelayType delay_type, DWORD delay_tick, size_t send_max, void (*callback)(void *data), void *callback_data)
{
	SendMem *sm = SendMemSendFileCom(filename, binary, delay_type, delay_tick, send_max);
	if (sm == NULL) {
		return FALSE;
	}

	SendMemInitSetCallback(sm, callback, callback_data);
	return TRUE;
}

/**
 *	�Z��������𑗐M����
 *	@param[in]	str		malloc���ꂽ�̈�̕�����
 *						���M�� free() �����
 *	TODO
 *		CBSendStart() @clipboar.c �Ɠ���
 */
BOOL SendMemPasteString(wchar_t *str)
{
	const size_t len = wcslen(str);
	CommTextOutW(&cv, str, (int)len);
	if (ts.LocalEcho > 0) {
		CommTextEchoW(&cv, str, (int)len);
	}

	free(str);
	return TRUE;
}
