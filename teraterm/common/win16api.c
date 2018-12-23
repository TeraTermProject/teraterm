#include <windows.h>
#include <assert.h>

/**
 *	@param[in]	iAttribute	teratermでは0しか使用しない
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lcreat(const char *FileName, int iAttribute)
{
	HANDLE handle;
	assert(iAttribute == 0);
	handle = CreateFileA(FileName,
						 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return handle;
}

/**
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) オープンできなかった
 *				(実際のAPIはHFILE_ERROR((HFILE)-1)を返す)
 */
HANDLE win16_lopen(const char *FileName, int iReadWrite)
{
	HANDLE handle;
	switch(iReadWrite) {
	case OF_READ:
		// read only
		handle = CreateFileA(FileName,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_WRITE:
		// write
		handle = CreateFileA(FileName,
							 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_READWRITE:
		// read/write (teratermではttpmacro/ttl.c内の1箇所のみで使用されている
		handle = CreateFileA(FileName,
							 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	default:
		assert(FALSE);
		handle = INVALID_HANDLE_VALUE;
		break;
	}
	return handle;
}

/**
 *	@retval なし
 *			(実際のAPIはオープンしていたHFILEを返す)
 */
void win16_lclose(HANDLE hFile)
{
	CloseHandle(hFile);
}

/**
 *	@retval 読み込みバイト数
 */
UINT win16_lread(HANDLE hFile, LPVOID lpBuffer, UINT uBytes)
{
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

/**
 *	@retval 書き込みバイト数
 */
UINT win16_lwrite(HANDLE hFile, const char*buf, UINT length)
{
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

/*
 *	@param[in]	iOrigin
 *				@arg 0(FILE_BEGIN)
 *				@arg 1(FILE_CURRENT)
 *				@arg 2(FILE_END)
 *	@retval ファイル位置
 *	@retval HFILE_ERROR((HFILE)-1)	エラー
 *	@retval INVALID_SET_FILE_POINTER((DWORD)-1) エラー
 */
LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin)
{
	DWORD pos = SetFilePointer(hFile, lOffset, NULL, iOrigin);
	if (pos == INVALID_SET_FILE_POINTER) {
		return HFILE_ERROR;
	}
	return pos;
}
