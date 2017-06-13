/*
 * WSAAsyncGetAddrInfo.c -- asynchronous version of getaddrinfo
 * Copyright(C) 2000-2003 Jun-ya Kato <kato@win6.jp>
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include "WSAASyncGetAddrInfo.h"
#include "ttwsk.h"

static unsigned __stdcall getaddrinfo_thread(void * p);

HANDLE PASCAL WSAAsyncGetAddrInfo(HWND hWnd, unsigned int wMsg,
                           const char *hostname,
                           const char *portname,
                           struct addrinfo *hints,
                           struct addrinfo **res)
{
	HANDLE thread;
	unsigned tid;
	struct getaddrinfo_args * ga;

	/*
	* allocate structure to pass args to sub-thread dynamically
	* WSAAsyncGetAddrInfo() is reentrant
	*/
	if ((ga = (struct getaddrinfo_args *)malloc(sizeof(struct getaddrinfo_args))) == NULL)
		return NULL;

	/* packing arguments struct addrinfo_args */
	ga->hWnd = hWnd;
	ga->wMsg = wMsg;
	ga->hostname = _strdup(hostname); // ポインタだけ渡すと、スレッド先で不定となる。(2012.11.7 yutaka)
	ga->portname = _strdup(portname);
	ga->hints = *hints; // ポインタだけ渡すと、スレッド先で不定となる。(2016.3.11 yutaka)
	ga->res = res;

	ga->lpHandle = (HANDLE *)malloc(sizeof(HANDLE));
	if (ga->lpHandle == NULL) {
		free(ga->hostname);
		free(ga->portname);
		return NULL;
	}

	/* create sub-thread running getaddrinfo() */
	thread = (HANDLE)_beginthreadex(NULL, 0, getaddrinfo_thread, ga, CREATE_SUSPENDED, &tid);
	*ga->lpHandle = (HANDLE)thread;
	ResumeThread(thread);

	/* return thread handle */
	if (thread == 0) {
		free(ga->lpHandle);
		free(ga->hostname);
		free(ga->portname);
		free(ga);
		return NULL;
	} else
		return (HANDLE)thread;
}

static unsigned __stdcall getaddrinfo_thread(void * p)
{
	int gai;
	HWND hWnd;
	unsigned int wMsg;
	const char *hostname;
	const char *portname;
	struct addrinfo *hints;
	struct addrinfo **res;
	struct getaddrinfo_args *ga;

	/* unpacking arguments */
	ga = (struct getaddrinfo_args *)p;
	hWnd = ga->hWnd;
	wMsg = ga->wMsg;
	hostname = ga->hostname;
	portname = ga->portname;
	hints = &ga->hints;
	res = ga->res;

	/* call getaddrinfo */
//	gai = Pgetaddrinfo(hostname, portname, hints, res);
	gai = getaddrinfo(hostname, portname, hints, res);

	/* send value of gai as message to window hWnd */
	PostMessage(hWnd, wMsg, (WPARAM)*ga->lpHandle, MAKELPARAM(0, gai));

	free(ga->lpHandle);
	free(ga->hostname);
	free(ga->portname);
	free(p);

	return 0;
}
