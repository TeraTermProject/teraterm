
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <assert.h>

#include "ttlib.h"

#include "deviceope.h"

typedef struct comdata_st {
	wchar_t *port_name;
	HANDLE h;
	OVERLAPPED rol;
	OVERLAPPED wol;
	COMMPROP device_prop;
	bool dcb_setted;
	DCB dcb;
	bool commtimeouts_setted;
	COMMTIMEOUTS commtimeouts;
	bool read_requested;
	bool write_requested;
	DWORD write_left;
	enum {
		STATE_CLOSE,
		STATE_OPEN,
		STATE_ERROR,
	} state;
	bool check_line_state_before_send;
} comdata_t;

static void *init(void)
{
	comdata_t *p = (comdata_t *)calloc(sizeof(*p), 1);
	if (p == NULL) {
		// no memory
		return NULL;
	}
	p->dcb_setted = false;
	p->commtimeouts_setted = false;
	p->state = comdata_t::STATE_CLOSE;
	return p;
}

static DWORD destroy(device_t *device)
{
	comdata_t *p = (comdata_t *)device->private_data;
	if (p->port_name != NULL) {
		free(p->port_name);
	}
	free(p);

	free(device);
	return ERROR_SUCCESS;
}

#define CommInQueSize 8192
#define CommOutQueSize 2048

/**
 *	クローズ
 */
static DWORD close(device_t *device)
{
	comdata_t *p = (comdata_t *)device->private_data;
	if (p == NULL) {
		return ERROR_HANDLES_CLOSED;
	}

	if (p->state == comdata_t::STATE_CLOSE) {
		return ERROR_SUCCESS;
	}

	CloseHandle(p->h);
	p->h = NULL;
	CloseHandle(p->rol.hEvent);
	p->rol.hEvent = NULL;
	CloseHandle(p->wol.hEvent);
	p->wol.hEvent = NULL;

	p->state = comdata_t::STATE_CLOSE;

	return ERROR_SUCCESS;
}

static DWORD open(device_t *device)
{
	comdata_t *p = (comdata_t *)device->private_data;
	HANDLE h;
	h = CreateFileW(p->port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		DWORD e = GetLastError();
		return e;
	}

	BOOL r;
	r = GetCommProperties(h, &p->device_prop);
	assert(r == TRUE);
	// p->device_prop.dwMaxTxQueue == 0 のときは、最大値なし(ドライバがうまくやってくれるらしい)

	DWORD DErr;
	ClearCommError(h, &DErr, NULL);
	SetupComm(h, CommInQueSize, CommOutQueSize);
	PurgeComm(h, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	if (p->commtimeouts_setted) {
		r = SetCommTimeouts(h, &p->commtimeouts);
		assert(r == TRUE);
	}

	r = GetCommTimeouts(h, &p->commtimeouts);
	assert(r == TRUE);

	if (p->dcb_setted) {
		if (p->dcb.XonChar == p->dcb.XoffChar) {
			p->dcb.XonChar = 0x11;
			p->dcb.XoffChar = 0x13;
		}
		r = SetCommState(h, &p->dcb);
		if (r == FALSE) {
			close(device);
			DWORD e = GetLastError();
			return e;
		}
	}

	r = GetCommState(h, &p->dcb);
	assert(r == TRUE);

	SetCommMask(h, 0);
	SetCommMask(h, EV_RXCHAR);

	memset(&p->rol, 0, sizeof(p->rol));
	p->rol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	assert(p->rol.hEvent != NULL);
	memset(&p->wol, 0, sizeof(p->wol));
	p->wol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	assert(p->wol.hEvent != NULL);
	p->h = h;

	p->state = comdata_t::STATE_OPEN;
	p->read_requested = false;
	p->write_requested = false;
	p->write_left = 0;
	p->check_line_state_before_send = true;

	return ERROR_SUCCESS;
}

/**
 *	ペンディング状態をチェックする
 *
 *	@param	readed				読み込んだバイト数
 *								0	読み込まれていない
 *	@return	ERROR_SUCCESS		読み込んだ (ペンディング状態終了)
 *	@return	ERROR_IO_PENDING	読み込み待ち(正常)
 *	@return	etc					エラー
 */
static DWORD wait_read(device_t *device, size_t *readed)
{
	comdata_t *p = (comdata_t *)device->private_data;
	HANDLE h = p->h;

	if (p->state != comdata_t::STATE_OPEN) {
		return ERROR_NOT_READY;
	}
	if (p->read_requested == false) {
		// リクエストしていないのに待ち状態になった
		return ERROR_INVALID_OPERATION;
	}

	DWORD dwMilliseconds = 0;
	DWORD wait = WaitForSingleObject(p->rol.hEvent, dwMilliseconds);
	if (wait == WAIT_TIMEOUT) {
		// まだ受信していない
		*readed = 0;
		return ERROR_IO_PENDING;
	}
	else if (wait == WAIT_OBJECT_0) {
		// イベント発生(受信した/エラー…)
		DWORD readed_;
		BOOL b = GetOverlappedResult(h, &p->rol, &readed_, TRUE);
		if (b) {
			// 読み込めた
			*readed = readed_;
			p->read_requested = false;
			return ERROR_SUCCESS;
		}
		else {
			DWORD e = GetLastError();
			p->state = comdata_t::STATE_ERROR;
			*readed = 0;
			return e;
		}
	}
	else if (wait == WAIT_ABANDONED) {
		// どんなとき発生する? event異常?
		p->state = comdata_t::STATE_ERROR;
		*readed = 0;
		return ERROR_INVALID_OPERATION;
	}
	else {
		// WAIT_FAILED
		p->state = comdata_t::STATE_ERROR;
		*readed = 0;
		DWORD e = GetLastError();
		return e;
	}
#if 0
	BOOL r = WaitCommEvent(h,&Evt, p->rol);
	if (r == FALSE) {
		// error
		DWORD DErr = GetLastError();
		if (DErr == ERROR_OPERATION_ABORTED) {
			// USB com port is removed
			printf("unpluged\n");
			return ERROR_OPERATION_ABORTED;
		}
		// クリアしていいの?
		ClearCommError(h,&DErr,NULL);
	}
#endif
}

/**
 *	データ読み込み、ブロックしない
 *
 *	@param	buf					読み込むアドレス
 *								後で読まれるかもしれないので注意
 *	@param	readed				読み込んだバイト数
 *	@return	ERROR_SUCCESS		読み込んだ
 *								(readed = 0 の場合も考慮すること)
 *	@return	ERROR_IO_PENDING	読み込み待ち
 *								wait_read() で完了を待つ
 */
static DWORD read(device_t *device, uint8_t *buf, size_t buf_len, size_t *readed)
{
	comdata_t *p = (comdata_t *)device->private_data;
	HANDLE h = p->h;
	DWORD err = ERROR_SUCCESS;

	if (p->state != comdata_t::STATE_OPEN) {
		return ERROR_NOT_READY;
	}
	if (p->read_requested) {
		// エラー、リクエスト中
		return ERROR_INVALID_OPERATION;
	}

	DWORD readed_;
	BOOL r = ReadFile(h, buf, (DWORD)buf_len, &readed_, &p->rol);
	if (!r) {
		*readed = 0;
		err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			p->read_requested = true;
			return ERROR_IO_PENDING;
		}
		else {
			// 何かエラー
			p->state = comdata_t::STATE_ERROR;
			p->read_requested = false;
			readed_ = 0;
			return err;
		}
	}

	*readed = readed_;
	return ERROR_SUCCESS;
}

/**
 *	ペンディング状態をチェックする
 *
 *	@param		writed				書き込まれたバイト数
 *									0	読み込まれていない
 *	@return	ERROR_SUCCESS		読み込んだ (ペンディング状態終了)
 *	@return	ERROR_IO_PENDING	読み込み待ち(正常)
 *	@return	etc					エラー
 */
static DWORD wait_write(device_t *device, size_t *writed)
{
	comdata_t *p = (comdata_t *)device->private_data;
	HANDLE h = p->h;

	if (p->state != comdata_t::STATE_OPEN) {
		return ERROR_NOT_READY;
	}

	if (p->write_requested == false) {
		// リクエストしていないのに待ち状態になった
		return ERROR_INVALID_OPERATION;
	}

	//const DWORD timeout_ms = INFINITE;
	const DWORD timeout_ms = 0;
	DWORD wait = WaitForSingleObject(p->wol.hEvent, timeout_ms);
	if (wait == WAIT_TIMEOUT) {
		// まだ送信していない
		*writed = 0;
		return ERROR_IO_PENDING;
	}
	else if (wait == WAIT_OBJECT_0) {
		// イベント発生(書き込んだ/エラー…)
		DWORD writed_;
		DWORD r = GetOverlappedResult(h, &p->wol, &writed_, FALSE);
		if (r) {
			*writed = writed_;
			p->write_left -= writed_;
			if (p->write_left == 0) {
				// 送信完了
				p->write_requested = false;
				return ERROR_SUCCESS;
			}
			else {
				// まだペンディング中
				return ERROR_IO_PENDING;
			}
		}
		else {
			DWORD e = GetLastError();
			p->state = comdata_t::STATE_ERROR;
			*writed = 0;
			return e;
		}
	}
	else {
		// WAIT_FAILED
		p->state = comdata_t::STATE_ERROR;
		*writed = 0;
		DWORD e = GetLastError();
		return e;
	}
}

/**
 *	書き込み
 *
 *	@return	ERROR_SUCCESS		書き込み完了(又は書き込みできなかった)
 *									writed == 0 のとき書き込みできなかった
 *	@return	ERROR_IO_PENDING	書き込み中
 *									wait_write() で完了を待つ
 *	@return	etc				エラー
 */
static DWORD write(device_t *device, const void *buf, size_t buf_len, size_t *writed)
{
	comdata_t *p = (comdata_t *)device->private_data;
	HANDLE h = p->h;
	DWORD err = ERROR_SUCCESS;

	if (p->state != comdata_t::STATE_OPEN) {
		return ERROR_NOT_READY;
	}

	if (buf_len == 0) {
		return ERROR_SUCCESS;
	}

	if (p->write_requested == true) {
		// エラー、リクエスト中
		return ERROR_INVALID_OPERATION;
	}
	assert(p->write_left == 0);

#if 0
	DWORD Errors;
	COMSTAT Comstat;
	ClearCommError(h, &Errors, &Comstat);
	if (Comstat.fCtsHold != 0) {
		return ERROR_SUCCESS;
	}
#endif

	if (p->check_line_state_before_send) {
		DWORD modem_state;
		GetCommModemStatus(h, &modem_state);
		if ((modem_state & MS_CTS_ON) == 0) {
			*writed = 0;
			return ERROR_SUCCESS;
		}
	}

#if 0
	// 送信サイズを調整
	if (buf_len > Comstat.cbOutQue) {
		buf_len = Comstat.cbOutQue;
	}
#endif

	if (writed != NULL) {
		*writed = 0;
	}

	DWORD writed_;
	BOOL r = WriteFile(h, buf, (DWORD)buf_len, &writed_, &p->wol);
	if (!r) {
		err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			//const DWORD timeout_ms = INFINITE;
			const DWORD timeout_ms = 0;
			p->write_left += (DWORD)buf_len;
			DWORD wait = WaitForSingleObject(p->wol.hEvent, timeout_ms);
			if (wait == WAIT_TIMEOUT) {
				// まだ送信していない
				p->write_requested = true;
				return ERROR_IO_PENDING;
			}
			else if (wait == WAIT_OBJECT_0) {
				// イベント発生(書き込み完了/エラー…)
				r = GetOverlappedResult(h, &p->wol, &writed_, FALSE);
				if (r) {
					// 書き込み完了
					goto write_complete;
				}
				else {
					err = GetLastError();
					p->state = comdata_t::STATE_ERROR;
					return err;
				}
			}
			else {	// wait == WAIT_ABANDONED || WAIT_FAILED
				p->state = comdata_t::STATE_ERROR;
				DWORD e = GetLastError();
				return e;
			}
		}
		else {
			p->state = comdata_t::STATE_ERROR;
			DWORD e = GetLastError();
			return e;
		}
	}

	// 書き込み完了
write_complete:
	if (writed != NULL) {
		*writed = writed_;
	}
	if (writed_ == 0) {
		// 操作が完了したが,送信バイト数==0
		// = 送信バッファがfull (デバッガで動作させていた時しか発生したことがない)
		return ERROR_INSUFFICIENT_BUFFER;
	}
	p->write_left -= writed_;
	if (p->write_left != 0) {
		// すべて書き込み済みではない,pending
		p->write_requested = true;
		return ERROR_IO_PENDING;
	}
	return ERROR_SUCCESS;
}

static DWORD ctrl(device_t *device, device_ctrl_request request, ...)
{
	comdata_t *p = (comdata_t *)device->private_data;
	va_list ap;
	va_start(ap, request);
	DWORD retval = ERROR_CALL_NOT_IMPLEMENTED;

	switch (request) {
	case SET_PORT_NAME: {
		const wchar_t *s = (wchar_t *)va_arg(ap, wchar_t *);
		if (p->port_name != NULL) {
			free(p->port_name);
		}
		p->port_name = _wcsdup(s);
		retval = ERROR_SUCCESS;
		break;
	}
	case GET_RAW_HANDLE: {
		HANDLE *handle = va_arg(ap, HANDLE *);
		*handle = p->h;
		retval = ERROR_SUCCESS;
		break;
	}
	case SET_COM_DCB: {
		DCB *dcb = va_arg(ap, DCB *);
		p->dcb = *dcb;
		p->dcb_setted = true;
		retval = ERROR_SUCCESS;
		break;
	}
	case GET_COM_DCB: {
		DCB *dcb = va_arg(ap, DCB *);
		*dcb = p->dcb;
		retval = ERROR_SUCCESS;
		break;
	}
	case SET_COM_TIMEOUTS: {
		COMMTIMEOUTS *commtimeouts = va_arg(ap, COMMTIMEOUTS *);
		p->commtimeouts = *commtimeouts;
		p->commtimeouts_setted = true;
		retval = ERROR_SUCCESS;
		break;
	}
	case GET_COM_TIMEOUTS: {
		COMMTIMEOUTS *commtimeouts = va_arg(ap, COMMTIMEOUTS *);
		*commtimeouts = p->commtimeouts;
		retval = ERROR_SUCCESS;
		break;
	}
	case SET_CHECK_LINE_STATE_BEFORE_SEND: {
		int check_line_state = va_arg(ap, int);
		p->check_line_state_before_send = check_line_state;
		retval = ERROR_SUCCESS;
		break;
	}
	case GET_CHECK_LINE_STATE_BEFORE_SEND: {
		int *check_line_state = va_arg(ap, int *);
		*check_line_state = (int)p->check_line_state_before_send;
		retval = ERROR_SUCCESS;
		break;
	}
	case OPEN_CONFIG_DIALOG: {
		COMMCONFIG cc;
		DWORD size = sizeof(cc);
		BOOL r = GetCommConfig(p->h, &cc, &size);
		assert(r == TRUE);
		r = CommConfigDialogW(p->port_name, NULL, &cc);
		if (r == TRUE) {
			r = SetCommConfig(p->h, &cc, size);
			assert(r == TRUE);
			r = GetCommState(p->h, &p->dcb);
			assert(r == TRUE);
		}
		break;
	}
	}

	va_end(ap);
	return retval;
}

static const device_ope ope = {
	destroy,
	open,
	close,
	read,
	wait_read,
	write,
	wait_write,
	ctrl,
};

/**
 *	comポートデバイス作成
 */
DWORD com_init(device_t **device)
{
	device_t *dev = (device_t *)calloc(sizeof(*dev), 1);
	if (dev == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	void *data = init();
	if (data == NULL) {
		free(dev);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	dev->ope = &ope;
	dev->private_data = data;
	*device = dev;
	return ERROR_SUCCESS;
}
