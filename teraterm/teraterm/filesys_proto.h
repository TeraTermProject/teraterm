/*
 * (C) 2020 TeraTerm Project
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

#pragma once

#include "filesys_io.h"

typedef enum {
	PROTO_KMT = 1,
	PROTO_XM = 2,
	PROTO_ZM = 3,
	PROTO_BP = 4,
	PROTO_QV = 5,
	PROTO_YM = 6,
} ProtoId_t;

typedef enum {
	OpKmtRcv  = 3,
	OpKmtGet  = 4,
	OpKmtSend = 5,
	OpKmtFin  = 6,
	OpXRcv    = 7,
	OpXSend   = 8,
	OpZRcv    = 9,
	OpZSend   = 10,
	OpBPRcv   = 11,
	OpBPSend  = 12,
	OpQVRcv   = 13,
	OpQVSend  = 14,
	OpYRcv    = 15,
	OpYSend   = 16,
} OpId_t;

typedef struct FileVarProto {
	// ��protosys_proto.cpp���̂ݎg�p

	ProtoId_t ProtoId;
	OpId_t OpId;

	HWND HMainWin;
	HWND HWin;
	wchar_t *DlgCaption;

	// ���M�t�@�C�����z��
	//	�t���p�X�̃t�@�C�����z��(��ԍŌ��NULL)
	wchar_t **FileNames;
	int FNCount;		// ���M���t�@�C�����z��index(0...)

	// ��M
	wchar_t *RecievePath;		// ��M�t�H���_(�I�[�Ƀp�X�Z�p���[�^'\\'���t������Ă���)

	// ��protosys_proto.cpp���̂ݎg�p

	// ���e�v���g�R���Ŏg�p���郏�[�N
	BOOL OverWrite;
	BOOL Success;
	BOOL NoMsg;
	// ���e�v���g�R���Ŏg�p���郏�[�N

	// services
	char *(*GetNextFname)(struct FileVarProto *fv);
	char *(*GetRecievePath)(struct FileVarProto *fv);
	void (*FTSetTimeOut)(struct FileVarProto *fv, int T);
	void (*SetDialogCation)(struct FileVarProto *fv, const char *key, const wchar_t *default_caption);

	// protocol entrys, data
	const struct ProtoOp_ *ProtoOp;
	void *data;

	// UI
	const struct InfoOp_ *InfoOp;

	TFileIO *file;
} TFileVarProto;
typedef TFileVarProto *PFileVarProto;

// �v���g�R���̃I�y���[�V����
//   �e�v���g�R���̎���
typedef struct ProtoOp_ {
	// ����������
	// �������m�ہA��ԏ����������s��
	//	@retval	TRUE	����I��
	//	@retval	FALSE	�ُ�I���A���������s
	BOOL (*Init)(struct FileVarProto *fv, PComVar cv, PTTSet ts);
	// �����̌p��
	//	@retval	TRUE	����A�������p���I��
	//	@retval	FALSE	�I���A��������Parse()���ĂԕK�v�Ȃ�
	BOOL (*Parse)(struct FileVarProto *fv, PComVar cv);
	// �^�C���A�E�g�ʒm
	//	�^�C���A�E�g�������������Ƃ��v���g�R�������ɒʒm
	void (*TimeOutProc)(struct FileVarProto *fv, PComVar cv);
	// �L�����Z���ʒm
	//	���[�U�[���L�����Z���������Ƃ��v���g�R�������ɒʒm
	void (*Cancel)(struct FileVarProto *fv, PComVar cv);
	// �p�����[�^�ݒ�
	//	�v���g�R�����Ƃ̃p�����[�^�ݒ�
	int (*SetOptV)(struct FileVarProto *fv, int request, va_list ap);
	// �I������
	//	�������̊J���Ȃǂ��s��
	void (*Destroy)(struct FileVarProto *fv);
} TProtoOp;

// UI�ȂǏ��\���p�֐�
typedef struct InfoOp_ {
	void (*InitDlgProgress)(struct FileVarProto *fv, int *CurProgStat);
	void (*SetDlgTime)(struct FileVarProto *fv, DWORD elapsed, int bytes);
	void (*SetDlgPacketNum)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgByteCount)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgPercent)(struct FileVarProto *fv, LONG a, LONG b, int *p);
	void (*SetDlgProtoText)(struct FileVarProto *fv, const char *text);
	void (*SetDlgProtoFileName)(struct FileVarProto *fv, const char *text);
} TInfoOp;
