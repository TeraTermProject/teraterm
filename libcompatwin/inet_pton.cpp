#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

/**
 * inet_pton
 * af:	 AF_INET (IPv4) または AF_INET6 (IPv6)
 * src:	 IPアドレス文字列
 * dst:	 バイナリ形式のアドレスを格納する構造体 (in_addr または in6_addr)
 */
extern "C" int WSAAPI _inet_pton(int af, const char* src, void* dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN + 1];

	// 引数チェック
	if (src == NULL || dst == NULL) return 0;

	// WSAStringToAddress は非 const の文字列を要求する場合があるためコピー
	strncpy(src_copy, src, INET6_ADDRSTRLEN);
	src_copy[INET6_ADDRSTRLEN] = '\0';

	// アドレス情報を格納する構造体を初期化
	memset(&ss, 0, sizeof(ss));
	ss.ss_family = af;

	// 文字列からバイナリ形式のアドレスに変換
	// 成功すれば 0 が返る
	int result = WSAStringToAddressA(
		src_copy,			// アドレス文字列
		af,					// アドレスファミリー
		NULL,				// プロトコル情報 (通常NULL)
		(LPSOCKADDR)&ss,	// 結果を受け取る構造体
		&size				// 構造体のサイズ
		);
	if (result == 0) {
		// エラー
		return 0;
	}

	// 成功時、適切な構造体部分をコピー
	if (af == AF_INET) {
		memcpy(dst, &(((struct sockaddr_in*)&ss)->sin_addr), sizeof(struct in_addr));
	} else if (af == AF_INET6) {
		memcpy(dst, &(((struct sockaddr_in6*)&ss)->sin6_addr), sizeof(struct in6_addr));
	}
	return 1;
}
