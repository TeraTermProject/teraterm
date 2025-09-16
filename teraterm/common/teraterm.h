/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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
/* IPv6 modification is Copyright(C) 2000, 2001 Jun-ya kato <kato@win6.jp> */

#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */
#endif /* _WINSOCKAPI_ */
#include <windows.h>

/* _MSC_VER の値
Visual C++ 8.0 (Visual Studio 2005)  1400
Visual C++ 9.0 (Visual Studio 2008)  1500
Visual C++ 10.0 (Visual Studio 2010) 1600
Visual C++ 11.0 (Visual Studio 2012) 1700
Visual C++ 12.0 (Visual Studio 2013) 1800
Visual C++ 14.0 (Visual Studio 2015) 1900
*/

/* VS2015(VC14.0)だと、WSASocketA(), inet_ntoa() などのAPIがdeprecatedであると
 * 警告するために、警告を抑止する。代替関数に置換すると、VS2005(VC8.0)でビルド
 * できなくなるため、警告を抑止するだけとする。
 */
#if _MSC_VER >= 1800  // VSC2013(VC12.0) or later
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
		#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#endif
#endif

#include "i18n.h"
#include "ttlib.h"

#define MAXPATHLEN 256

#define DEBUG_PRINT(val) { \
	FILE *fp; \
	fp = fopen("debugmsg.txt", "a+"); \
	if (fp != NULL) { \
		fprintf(fp, "%s = %d\n", #val, val); \
		fclose(fp); \
	} \
}
