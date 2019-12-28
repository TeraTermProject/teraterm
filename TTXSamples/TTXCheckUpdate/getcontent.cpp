
#include <windows.h>
#include <wininet.h>
#include <string>
#include <sstream>
#include <locale.h>
#include <conio.h>
#include <tchar.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#if (defined(_MSC_VER) && (_MSC_VER >= 1600)) || !defined(_MSC_VER)
#include <stdint.h>
#else
typedef unsigned char uint8_t;
#endif

#include "codeconv.h"

#include "getcontent.h"

/**
 *	urlから情報を取得する
 *
 *	@param[in]	url
 *	@param[in]	agent	エージェント名
 *	@param[in]	ptr		取得した情報
 *						不要になったらfree()すること
 *	@param[in]	size	取得した情報サイズ
 *
 */
BOOL GetContent(const wchar_t *url, const wchar_t *agent, void **ptr, size_t *size)
{
	// URL解析
	size_t url_length = wcslen(url);
	if (url_length == 0) {
		return false;
	}
	URL_COMPONENTSW urlcomponents = {};
	urlcomponents.dwStructSize = sizeof(URL_COMPONENTS);
	wchar_t szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
	wchar_t szUrlPath[INTERNET_MAX_PATH_LENGTH];
	urlcomponents.lpszHostName = szHostName;
	urlcomponents.lpszUrlPath = szUrlPath;
	urlcomponents.dwHostNameLength = _countof(szHostName);
	urlcomponents.dwUrlPathLength = _countof(szUrlPath);
	if (InternetCrackUrlW(url, (DWORD)url_length, 0, &urlcomponents) == FALSE) {
		return false;
	}
	INTERNET_PORT nPort = urlcomponents.nPort;

	// HTTPかHTTPSかそれ以外か
	DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT;
	if (INTERNET_SCHEME_HTTP == urlcomponents.nScheme) {
		// HTTP
	}
	else if (INTERNET_SCHEME_HTTPS == urlcomponents.nScheme) {
		// HTTPS
		dwFlags |= INTERNET_FLAG_SECURE;
	}
	else {
		return false;
	}

	HINTERNET hInternetOpen = NULL;
	HINTERNET hInternetConnect = NULL;
	HINTERNET hInternetRequest = NULL;
	// try
	{
		hInternetOpen = InternetOpenW(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInternetOpen == NULL) {
			goto finish;
		}

		hInternetConnect = InternetConnectW(hInternetOpen, szHostName, nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (hInternetConnect == NULL) {
			goto finish;
		}

		// HTTP接続を開く
		hInternetRequest = HttpOpenRequestW(hInternetConnect, L"GET", szUrlPath, NULL, NULL, NULL, dwFlags, 0);
		if (hInternetRequest == NULL) {
			// net接続がない?
			goto finish;
		}

		// HTTP要求送信
		if (HttpSendRequestW(hInternetRequest, NULL, 0, NULL, 0) == FALSE) {
			goto finish;
		}

		// status code
		DWORD dwStatusCode;
		DWORD dwLength = sizeof(DWORD);
		if (!HttpQueryInfo(hInternetRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwLength,
						   0)) {
			goto finish;
		}
		if (dwStatusCode != HTTP_STATUS_OK) {
			goto finish;
		}

		// contents read
		const size_t READBUFFER_SIZE = 4096;
		uint8_t *buf_ptr = NULL;
		size_t buf_size = 0;
		while (1) {
			uint8_t *buf_ptr_tmp = (uint8_t *)realloc(buf_ptr, buf_size + READBUFFER_SIZE);
			if (buf_ptr_tmp == NULL) {
				free(buf_ptr);
				goto finish;
			}
			buf_ptr = buf_ptr_tmp;

			uint8_t *buf_ptr_read = &buf_ptr[buf_size];
			DWORD dwRead = 0;
			if (!InternetReadFile(hInternetRequest, buf_ptr_read, READBUFFER_SIZE, &dwRead)) {
				free(buf_ptr);
				goto finish;
			}
			buf_size += dwRead;
			buf_ptr = (uint8_t *)realloc(buf_ptr, buf_size);  // fit size
			if (dwRead == 0) {
				break;
			}
		}

		*ptr = buf_ptr;
		*size = buf_size;
		InternetCloseHandle(hInternetRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetOpen);
		return true;
	}

	// catch
finish:
	InternetCloseHandle(hInternetRequest);
	InternetCloseHandle(hInternetConnect);
	InternetCloseHandle(hInternetOpen);
	return false;
}
