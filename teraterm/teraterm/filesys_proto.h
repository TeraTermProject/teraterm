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
	OpRawRcv  = 17,
} OpId_t;

struct Comm_;

typedef struct CommOp_ {
	// 1byte送信
	int (*BinaryOut)(struct Comm_ *comm, const CHAR *buf, size_t len);

	// 1byte受信
	int (*Read1Byte)(struct Comm_ *comm, BYTE *b);

	// 1byte送信,送信バッファの先頭に入れる
	void (*Insert1Byte)(struct Comm_ *comm, BYTE b);

	// 受信バッファをクリアする
	void (*FlashReceiveBuf)(struct Comm_ *comm);
} CommOp;

typedef struct Comm_ {
	const CommOp *op;
	void *private_data;
} TComm;

// プロトコルのオペレーション
//   各プロトコルの実装
struct FileVarProto;
struct Proto_;
typedef struct ProtoOp_ {
	// 初期化処理
	// メモリ確保、状態初期化等を行う
	//	@retval	TRUE	正常終了
	//	@retval	FALSE	異常終了、初期化失敗
	BOOL (*Init)(struct Proto_ *pv, PComVar cv, PTTSet ts);
	// 処理の継続
	//	@retval	TRUE	正常、処理を継続終了
	//	@retval	FALSE	終了、引き続きParse()を呼ぶ必要なし
	BOOL (*Parse)(struct Proto_ *pv);
	// タイムアウト通知
	//	タイムアウトが発生したことをプロトコル処理に通知
	void (*TimeOutProc)(struct Proto_ *pv);
	// キャンセル通知
	//	ユーザーがキャンセルしたことをプロトコル処理に通知
	void (*Cancel)(struct Proto_ *pv);
	// パラメータ設定
	//	プロトコルごとのパラメータ設定
	int (*SetOpt)(struct Proto_ *pv, int request, ...);
	int (*SetOptV)(struct Proto_ *pv, int request, va_list ap);
	// 終了処理
	//	メモリの開放などを行う
	void (*Destroy)(struct Proto_ *pv);
} TProtoOp;

typedef struct Proto_ {
	const TProtoOp *Op;
	void *PrivateData;
} TProto;

typedef struct FileVarProto {
	// ↓protosys_proto.cpp内のみ使用

	OpId_t OpId;

	HWND HMainWin;
	HWND HWin;
	wchar_t *DlgCaption;

	// 送信ファイル名配列
	//	フルパスのファイル名配列(一番最後はNULL)
	wchar_t **FileNames;
	int FNCount;		// 送信中ファイル名配列index(0...)

	// 受信
	wchar_t *RecievePath;		// 受信フォルダ(終端にパスセパレータ'\\'が付加されている)

	// ↑protosys_proto.cpp内のみ使用

	// ↓各プロトコルで使用するワーク
	BOOL OverWrite;
	BOOL Success;
	BOOL NoMsg;
	// ↑各プロトコルで使用するワーク

	// services

	// 送信するファイル名 UTF-8
	char *(*GetNextFname)(struct FileVarProto *fv);
	// 受信パス UTF-8
	char *(*GetRecievePath)(struct FileVarProto *fv);
	void (*FTSetTimeOut)(struct FileVarProto *fv, int T);
	void (*SetDialogCation)(struct FileVarProto *fv, const char *key, const wchar_t *default_caption);

	// protocol entrys, data
	TProto *Proto;

	// UI
	const struct InfoOp_ *InfoOp;

	// comm
	TComm *Comm;

	TFileIO *file_fv;
} TFileVarProto;
typedef TFileVarProto *PFileVarProto;

// UIなど情報表示用関数
typedef struct InfoOp_ {
	void (*InitDlgProgress)(struct FileVarProto *fv, int *CurProgStat);
	void (*SetDlgTime)(struct FileVarProto *fv, DWORD elapsed, int bytes);
	void (*SetDlgPacketNum)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgByteCount)(struct FileVarProto *fv, LONG Num);
	void (*SetDlgPercent)(struct FileVarProto *fv, LONG a, LONG b, int *p);
	void (*SetDlgProtoText)(struct FileVarProto *fv, const char *text);
	void (*SetDlgProtoFileName)(struct FileVarProto *fv, const char *text);
} TInfoOp;
