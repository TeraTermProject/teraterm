/*
 * (C) 2019 TeraTerm Project
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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DLL_GET_MODULE_HANDLE,
	DLL_LOAD_LIBRARY_SYSTEM,
	DLL_LOAD_LIBRARY_CURRENT,
} DLLLoadFlag;

typedef enum {
	DLL_ACCEPT_NOT_EXIST,
	DLL_ERROR_NOT_EXIST,
} DLLFuncFlag;

typedef struct {
	const char *ApiName;
	void **func;
} APIInfo;

typedef struct {
	const TCHAR *DllName;
	DLLLoadFlag LoadFlag;
	DLLFuncFlag FuncFlag;
	const APIInfo *APIInfoPtr;
} DllInfo;

void DLLInit();
void DLLExit();
void DLLGetApiAddressFromLists(const DllInfo *dllInfos);
DWORD DLLGetApiAddressFromList(const TCHAR *dllPath, DLLLoadFlag LoadFlag,
							   DLLFuncFlag FuncFlag, const APIInfo *ApiInfo);
DWORD DLLGetApiAddress(const TCHAR *dllPath, DLLLoadFlag LoadFlag,
					   const char *ApiName, void **pFunc);

#ifdef __cplusplus
}
#endif
