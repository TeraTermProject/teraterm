// based on PuTTY source code
/*
 * winjump.c: support for Windows 7 jump lists.
 *
 * The Windows 7 jumplist is a customizable list defined by the
 * application. It is persistent across application restarts: the OS
 * maintains the list when the app is not running. The list is shown
 * when the user right-clicks on the taskbar button of a running app
 * or a pinned non-running application. We use the jumplist to
 * maintain a list of recently started saved sessions, started either
 * by doubleclicking on a saved session, or with the command line
 * "-load" parameter.
 *
 * Since the jumplist is write-only: it can only be replaced and the
 * current list cannot be read, we must maintain the contents of the
 * list persistantly in the registry. The file winstore.h contains
 * functions to directly manipulate these registry entries. This file
 * contains higher level functions to manipulate the jumplist.
 */

/*
 * Copyright (C) 2011-2017 TeraTerm Project
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <shlobj.h>

#include "winjump.h"
#include "teraterm.h"
#include "tttypes.h"

#define MAX_JUMPLIST_ITEMS 30 /* PuTTY will never show more items in
                               * the jumplist than this, regardless of
                               * user preferences. */

/*
 * COM structures and functions.
 */
#ifndef PROPERTYKEY_DEFINED
#define PROPERTYKEY_DEFINED
typedef struct _tagpropertykey {
    GUID fmtid;
    DWORD pid;
} PROPERTYKEY;
#endif
#ifndef _REFPROPVARIANT_DEFINED
#define _REFPROPVARIANT_DEFINED
typedef PROPVARIANT *REFPROPVARIANT;
#endif
/* MinGW doesn't define this yet: */
#ifndef _PROPVARIANTINIT_DEFINED_
#define _PROPVARIANTINIT_DEFINED_
#define PropVariantInit(pvar) memset((pvar),0,sizeof(PROPVARIANT))
#endif

#ifndef __ICustomDestinationList_INTERFACE_DEFINED__
#define __ICustomDestinationList_INTERFACE_DEFINED__
// #if !(_MSC_VER >= 1600)  // VC2010(VC10.0) or later
typedef struct ICustomDestinationListVtbl {
    HRESULT ( __stdcall *QueryInterface ) (
        /* [in] ICustomDestinationList*/ void *This,
        /* [in] */  const GUID * const riid,
        /* [out] */ void **ppvObject);

    ULONG ( __stdcall *AddRef )(
        /* [in] ICustomDestinationList*/ void *This);

    ULONG ( __stdcall *Release )(
        /* [in] ICustomDestinationList*/ void *This);

    HRESULT ( __stdcall *SetAppID )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [string][in] */ LPCWSTR pszAppID);

    HRESULT ( __stdcall *BeginList )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [out] */ UINT *pcMinSlots,
        /* [in] */  const GUID * const riid,
        /* [out] */ void **ppv);

    HRESULT ( __stdcall *AppendCategory )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [string][in] */ LPCWSTR pszCategory,
        /* [in] IObjectArray*/ void *poa);

    HRESULT ( __stdcall *AppendKnownCategory )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [in] KNOWNDESTCATEGORY*/ int category);

    HRESULT ( __stdcall *AddUserTasks )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [in] IObjectArray*/ void *poa);

    HRESULT ( __stdcall *CommitList )(
        /* [in] ICustomDestinationList*/ void *This);

    HRESULT ( __stdcall *GetRemovedDestinations )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [in] */ const IID * const riid,
        /* [out] */ void **ppv);

    HRESULT ( __stdcall *DeleteList )(
        /* [in] ICustomDestinationList*/ void *This,
        /* [string][unique][in] */ LPCWSTR pszAppID);

    HRESULT ( __stdcall *AbortList )(
        /* [in] ICustomDestinationList*/ void *This);

} ICustomDestinationListVtbl;

typedef struct ICustomDestinationList
{
    ICustomDestinationListVtbl *lpVtbl;
} ICustomDestinationList;
#endif

#ifndef __IObjectArray_INTERFACE_DEFINED__
#define __IObjectArray_INTERFACE_DEFINED__
typedef struct IObjectArrayVtbl
{
    HRESULT ( __stdcall *QueryInterface )(
        /* [in] IObjectArray*/ void *This,
        /* [in] */ const GUID * const riid,
        /* [out] */ void **ppvObject);

    ULONG ( __stdcall *AddRef )(
        /* [in] IObjectArray*/ void *This);

    ULONG ( __stdcall *Release )(
        /* [in] IObjectArray*/ void *This);

    HRESULT ( __stdcall *GetCount )(
        /* [in] IObjectArray*/ void *This,
        /* [out] */ UINT *pcObjects);

    HRESULT ( __stdcall *GetAt )(
        /* [in] IObjectArray*/ void *This,
        /* [in] */ UINT uiIndex,
        /* [in] */ const GUID * const riid,
        /* [out] */ void **ppv);

} IObjectArrayVtbl;

typedef struct IObjectArray
{
    IObjectArrayVtbl *lpVtbl;
} IObjectArray;
#endif

#ifndef __IShellLink_INTERFACE_DEFINED__
#define __IShellLink_INTERFACE_DEFINED__
typedef struct IShellLinkVtbl
{
    HRESULT ( __stdcall *QueryInterface )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ const GUID * const riid,
        /* [out] */ void **ppvObject);

    ULONG ( __stdcall *AddRef )(
        /* [in] IShellLink*/ void *This);

    ULONG ( __stdcall *Release )(
        /* [in] IShellLink*/ void *This);

    HRESULT ( __stdcall *GetPath )(
        /* [in] IShellLink*/ void *This,
        /* [string][out] */ LPSTR pszFile,
        /* [in] */ int cch,
        /* [unique][out][in] */ WIN32_FIND_DATAA *pfd,
        /* [in] */ DWORD fFlags);

    HRESULT ( __stdcall *GetIDList )(
        /* [in] IShellLink*/ void *This,
        /* [out] LPITEMIDLIST*/ void **ppidl);

    HRESULT ( __stdcall *SetIDList )(
        /* [in] IShellLink*/ void *This,
        /* [in] LPITEMIDLIST*/ void *pidl);

    HRESULT ( __stdcall *GetDescription )(
        /* [in] IShellLink*/ void *This,
        /* [string][out] */ LPSTR pszName,
        /* [in] */ int cch);

    HRESULT ( __stdcall *SetDescription )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszName);

    HRESULT ( __stdcall *GetWorkingDirectory )(
        /* [in] IShellLink*/ void *This,
        /* [string][out] */ LPSTR pszDir,
        /* [in] */ int cch);

    HRESULT ( __stdcall *SetWorkingDirectory )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszDir);

    HRESULT ( __stdcall *GetArguments )(
        /* [in] IShellLink*/ void *This,
        /* [string][out] */ LPSTR pszArgs,
        /* [in] */ int cch);

    HRESULT ( __stdcall *SetArguments )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszArgs);

    HRESULT ( __stdcall *GetHotkey )(
        /* [in] IShellLink*/ void *This,
        /* [out] */ WORD *pwHotkey);

    HRESULT ( __stdcall *SetHotkey )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ WORD wHotkey);

    HRESULT ( __stdcall *GetShowCmd )(
        /* [in] IShellLink*/ void *This,
        /* [out] */ int *piShowCmd);

    HRESULT ( __stdcall *SetShowCmd )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ int iShowCmd);

    HRESULT ( __stdcall *GetIconLocation )(
        /* [in] IShellLink*/ void *This,
        /* [string][out] */ LPSTR pszIconPath,
        /* [in] */ int cch,
        /* [out] */ int *piIcon);

    HRESULT ( __stdcall *SetIconLocation )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszIconPath,
        /* [in] */ int iIcon);

    HRESULT ( __stdcall *SetRelativePath )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszPathRel,
        /* [in] */ DWORD dwReserved);

    HRESULT ( __stdcall *Resolve )(
        /* [in] IShellLink*/ void *This,
        /* [unique][in] */ HWND hwnd,
        /* [in] */ DWORD fFlags);

    HRESULT ( __stdcall *SetPath )(
        /* [in] IShellLink*/ void *This,
        /* [string][in] */ LPCSTR pszFile);

} IShellLinkVtbl;
#endif

#ifndef __IObjectCollection_INTERFACE_DEFINED__
#define __IObjectCollection_INTERFACE_DEFINED__
typedef struct IObjectCollectionVtbl
{
    HRESULT ( __stdcall *QueryInterface )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ const GUID * const riid,
        /* [out] */ void **ppvObject);

    ULONG ( __stdcall *AddRef )(
        /* [in] IShellLink*/ void *This);

    ULONG ( __stdcall *Release )(
        /* [in] IShellLink*/ void *This);

    HRESULT ( __stdcall *GetCount )(
        /* [in] IShellLink*/ void *This,
        /* [out] */ UINT *pcObjects);

    HRESULT ( __stdcall *GetAt )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ UINT uiIndex,
        /* [in] */ const GUID * const riid,
        /* [iid_is][out] */ void **ppv);

    HRESULT ( __stdcall *AddObject )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ void *punk);

    HRESULT ( __stdcall *AddFromArray )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ IObjectArray *poaSource);

    HRESULT ( __stdcall *RemoveObjectAt )(
        /* [in] IShellLink*/ void *This,
        /* [in] */ UINT uiIndex);

    HRESULT ( __stdcall *Clear )(
        /* [in] IShellLink*/ void *This);

} IObjectCollectionVtbl;

typedef struct IObjectCollection
{
    IObjectCollectionVtbl *lpVtbl;
} IObjectCollection;
#endif

#ifndef __IPropertyStore_INTERFACE_DEFINED__
#define __IPropertyStore_INTERFACE_DEFINED__
// #if !(_MSC_VER >= 1500)  // VC2008(VC9.0) or later
typedef struct IPropertyStoreVtbl
{
    HRESULT ( __stdcall *QueryInterface )(
        /* [in] IPropertyStore*/ void *This,
        /* [in] */ const GUID * const riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( __stdcall *AddRef )(
        /* [in] IPropertyStore*/ void *This);

    ULONG ( __stdcall *Release )(
        /* [in] IPropertyStore*/ void *This);

    HRESULT ( __stdcall *GetCount )(
        /* [in] IPropertyStore*/ void *This,
        /* [out] */ DWORD *cProps);

    HRESULT ( __stdcall *GetAt )(
        /* [in] IPropertyStore*/ void *This,
        /* [in] */ DWORD iProp,
        /* [out] */ PROPERTYKEY *pkey);

    HRESULT ( __stdcall *GetValue )(
        /* [in] IPropertyStore*/ void *This,
        /* [in] */ const PROPERTYKEY * const key,
        /* [out] */ PROPVARIANT *pv);

    HRESULT ( __stdcall *SetValue )(
        /* [in] IPropertyStore*/ void *This,
        /* [in] */ const PROPERTYKEY * const key,
        /* [in] */ REFPROPVARIANT propvar);

    HRESULT ( __stdcall *Commit )(
        /* [in] IPropertyStore*/ void *This);
} IPropertyStoreVtbl;

typedef struct IPropertyStore
{
    IPropertyStoreVtbl *lpVtbl;
} IPropertyStore;
#endif

static const CLSID CLSID_DestinationList = {
    0x77f10cf0, 0x3db5, 0x4966, {0xb5,0x20,0xb7,0xc5,0x4f,0xd3,0x5e,0xd6}
};
static const CLSID CLSID_ShellLink = {
    0x00021401, 0x0000, 0x0000, {0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}
};
static const CLSID CLSID_EnumerableObjectCollection = {
    0x2d3468c1, 0x36a7, 0x43b6, {0xac,0x24,0xd3,0xf0,0x2f,0xd9,0x60,0x7a}
};
static const IID IID_IObjectCollection = {
    0x5632b1a4, 0xe38a, 0x400a, {0x92,0x8a,0xd4,0xcd,0x63,0x23,0x02,0x95}
};
static const IID IID_IShellLink = {
    0x000214ee, 0x0000, 0x0000, {0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}
};
static const IID IID_ICustomDestinationList = {
    0x6332debf, 0x87b5, 0x4670, {0x90,0xc0,0x5e,0x57,0xb4,0x08,0xa4,0x9e}
};
static const IID IID_IObjectArray = {
    0x92ca9dcd, 0x5622, 0x4bba, {0xa8,0x05,0x5e,0x9f,0x54,0x1b,0xd8,0xc9}
};
static const IID IID_IPropertyStore = {
    0x886d8eeb, 0x8cf2, 0x4446, {0x8d,0x02,0xcd,0xba,0x1d,0xbd,0xcf,0x99}
};
static const PROPERTYKEY PKEY_Title = {
    {0xf29f85e0, 0x4ff9, 0x1068, {0xab,0x91,0x08,0x00,0x2b,0x27,0xb3,0xd9}},
    0x00000002
};

/* Type-checking macro to provide arguments for CoCreateInstance() etc.
 * The pointer arithmetic is a compile-time pointer type check that 'obj'
 * really is a 'type **', but is intended to have no effect at runtime. */
#define COMPTR(type, obj) &IID_##type, \
    (void **)(void *)((obj) + (sizeof((obj)-(type **)(obj))) \
		            - (sizeof((obj)-(type **)(obj))))

// LPCWSTR AppID = L"TeraTermProject.TeraTerm.ttermpro";

static char *IniFile = NULL;

BOOL isJumpListSupported(void)
{
	return IsWindows7OrLater();
}

/*
 * Function to make an IShellLink describing a particular PuTTY
 * command. If 'appname' is null, the command run will be the one
 * returned by GetModuleFileName, i.e. our own executable; if it's
 * non-null then it will be assumed to be a filename in the same
 * directory as our own executable, and the return value will be NULL
 * if that file doesn't exist.
 *
 * If 'sessionname' is null then no command line will be passed to the
 * program. If it's non-null, the command line will be that text
 * prefixed with an @ (to load a PuTTY saved session).
 *
 * Hence, you can launch a saved session using make_shell_link(NULL,
 * sessionname), and launch another app using e.g.
 * make_shell_link("puttygen.exe", NULL).
 */
static IShellLink *make_shell_link(const char *appname,
                                   const char *sessionname)
{
	IShellLink *ret;
	char *app_path, *param_string, *desc_string, *tmp_ptr;
	static char tt_path[2048];
	//void *psettings_tmp;
	IPropertyStore *pPS;
	PROPVARIANT pv;
	int len;
	HRESULT result;

	/* Retrieve path to executable. */
	if (!tt_path[0])
		GetModuleFileName(NULL, tt_path, sizeof(tt_path) - 1);

	if (appname) {
		tmp_ptr = strrchr(tt_path, '\\');
		len = (tmp_ptr - tt_path) + strlen(appname) + 2;
		app_path = malloc(len);
		strncpy_s(app_path, len, tt_path, (tmp_ptr - tt_path + 1));
		strcat_s(app_path, len, appname);
	}
	else {
		app_path = _strdup(tt_path);
	}

	/* Create the new item. */
	if (!SUCCEEDED(CoCreateInstance(&CLSID_ShellLink, NULL,
		CLSCTX_INPROC_SERVER,
		COMPTR(IShellLink, &ret))))
		return NULL;

	/* Set path, parameters, icon and description. */
	result = ret->lpVtbl->SetPath(ret, app_path);
	if (result != S_OK)
		OutputDebugPrintf("SetPath failed. (%ld)\n", result);

	param_string = _strdup(sessionname);
	result = ret->lpVtbl->SetArguments(ret, param_string);
	if (result != S_OK)
		OutputDebugPrintf("SetArguments failed. (%ld)\n", result);
	free(param_string);

	desc_string = _strdup("Connect to Tera Term session");
	result = ret->lpVtbl->SetDescription(ret, desc_string);
	if (result != S_OK)
		OutputDebugPrintf("SetDescription failed. (%ld)\n", result);
	free(desc_string);

	result = ret->lpVtbl->SetIconLocation(ret, app_path, 0);
	if (result != S_OK)
		OutputDebugPrintf("SetIconLocation failed. (%ld)\n", result);

	/* To set the link title, we require the property store of the link. */
	if (SUCCEEDED(ret->lpVtbl->QueryInterface(ret, COMPTR(IPropertyStore, &pPS)))) {
		PropVariantInit(&pv);
		pv.vt = VT_LPSTR;
		pv.pszVal = _strdup(sessionname);
		result = pPS->lpVtbl->SetValue(pPS, &PKEY_Title, &pv);
		if (result != S_OK)
			OutputDebugPrintf("SetValue failed. (%ld)\n", result);
		free(pv.pszVal);
		result = pPS->lpVtbl->Commit(pPS);
		if (result != S_OK)
			OutputDebugPrintf("Commit failed. (%ld)\n", result);
		result = pPS->lpVtbl->Release(pPS);
		if (result != S_OK)
			OutputDebugPrintf("Release shell link failed. (%ld)\n", result);
	}

	return ret;
}

/* Updates jumplist from registry. */
static void update_jumplist_from_registry(void)
{
	char EntName[128];
	char TempHost[1024];

	const char *piterator = TempHost;
	UINT num_items;
	UINT nremoved;
	int i;
	HRESULT result;

	/* Variables used by the cleanup code must be initialised to NULL,
	 * so that we don't try to free or release them if they were never
	 * set up. */
	ICustomDestinationList *pCDL = NULL;
	IObjectCollection *collection = NULL;
	IObjectArray *array = NULL;
	IShellLink *link = NULL;
	IObjectArray *pRemoved = NULL;
	int need_abort = FALSE;

	/*
	 * Create an ICustomDestinationList: the top-level object which
	 * deals with jump list management.
	 */
	if (!SUCCEEDED(CoCreateInstance(&CLSID_DestinationList, NULL,
		CLSCTX_INPROC_SERVER,
		COMPTR(ICustomDestinationList, &pCDL))))
		goto cleanup;

	/*
	 * Call its BeginList method to start compiling a list. This gives
	 * us back 'num_items' (a hint derived from systemwide
	 * configuration about how many things to put on the list) and
	 * 'pRemoved' (user configuration about things to leave off the
	 * list).
	 */
	if (!SUCCEEDED(pCDL->lpVtbl->BeginList(pCDL, &num_items,
		COMPTR(IObjectArray, &pRemoved))))
		goto cleanup;
	need_abort = TRUE;
	if (!SUCCEEDED(pRemoved->lpVtbl->GetCount(pRemoved, &nremoved)))
		nremoved = 0;

	/*
	 * Create an object collection to form the 'Recent Sessions'
	 * category on the jump list.
	 */
	if (!SUCCEEDED(CoCreateInstance(&CLSID_EnumerableObjectCollection,
		NULL, CLSCTX_INPROC_SERVER,
		COMPTR(IObjectCollection, &collection))))
		goto cleanup;

	/*
	 * Go through the jump list entries from the registry and add each
	 * one to the collection.
	 */
	for (i = 1; i <= MAX_JUMPLIST_ITEMS; i++) {
		_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Host%d", i);
		GetPrivateProfileString("Hosts", EntName, "", TempHost, sizeof(TempHost), IniFile);
		if (strlen(TempHost) == 0) {
			break;
		}

		OutputDebugPrintf("%s\n", piterator);
		link = make_shell_link(NULL, piterator);
		if (link) {
			UINT j;
			int found;

			/*
			 * Check that the link isn't in the user-removed list.
			 */
			for (j = 0, found = FALSE; j < nremoved && !found; j++) {
				IShellLink *rlink;
				if (SUCCEEDED(pRemoved->lpVtbl->GetAt
					(pRemoved, j, COMPTR(IShellLink, &rlink)))) {
					char desc1[2048], desc2[2048];
					if (SUCCEEDED(link->lpVtbl->GetDescription
						(link, desc1, sizeof(desc1) - 1)) &&
						SUCCEEDED(rlink->lpVtbl->GetDescription
							(rlink, desc2, sizeof(desc2) - 1)) &&
						!strcmp(desc1, desc2)) {
						found = TRUE;
					}
					result = rlink->lpVtbl->Release(rlink);
					if (result != S_OK)
						OutputDebugPrintf("Release rlink failed. (%ld)\n", result);
				}
			}

			if (!found) {
				result = collection->lpVtbl->AddObject(collection, (IUnknown *)link);
				if (result != S_OK)
					OutputDebugPrintf("AddObject link failed. (%ld)\n", result);
			}

			result = link->lpVtbl->Release(link);
			if (result != S_OK)
				OutputDebugPrintf("Release link failed. (%ld)\n", result);
			link = NULL;
		}
	}

	/*
	 * Get the array form of the collection we've just constructed,
	 * and put it in the jump list.
	 */
	if (!SUCCEEDED(collection->lpVtbl->QueryInterface
		(collection, COMPTR(IObjectArray, &array))))
		goto cleanup;

	result = pCDL->lpVtbl->AppendCategory(pCDL, L"Recent Sessions", array);
	if (result != S_OK)
		OutputDebugPrintf("AppendCategory array failed. (%ld)\n", result);

	/*
	 * Create an object collection to form the 'Tasks' category on the
	 * jump list.
	 */
	if (!SUCCEEDED(CoCreateInstance(&CLSID_EnumerableObjectCollection,
		NULL, CLSCTX_INPROC_SERVER,
		COMPTR(IObjectCollection, &collection))))
		goto cleanup;

	/*
	 * Get the array form of the collection we've just constructed,
	 * and put it in the jump list.
	 */
	if (!SUCCEEDED(collection->lpVtbl->QueryInterface
		(collection, COMPTR(IObjectArray, &array))))
		goto cleanup;

	result = pCDL->lpVtbl->AddUserTasks(pCDL, array);
	if (result != S_OK)
		OutputDebugPrintf("AddUserTasks array failed. (0x%x)\n", result);

	/*
	 * Now we can clean up the array and collection variables, so as
	 * to be able to reuse them.
	 */
	result = array->lpVtbl->Release(array);
	if (result != S_OK)
		OutputDebugPrintf("Release array failed. (%ld)\n", result);
	array = NULL;
	result = collection->lpVtbl->Release(collection);
	if (result != S_OK)
		OutputDebugPrintf("Release collection failed. (%ld)\n", result);
	collection = NULL;

	/*
	 * Create another object collection to form the user tasks
	 * category.
	 */
	if (!SUCCEEDED(CoCreateInstance(&CLSID_EnumerableObjectCollection,
		NULL, CLSCTX_INPROC_SERVER,
		COMPTR(IObjectCollection, &collection))))
		goto cleanup;

	/*
	 * Get the array form of the collection we've just constructed,
	 * and put it in the jump list.
	 */
	if (!SUCCEEDED(collection->lpVtbl->QueryInterface
		(collection, COMPTR(IObjectArray, &array))))
		goto cleanup;

	result = pCDL->lpVtbl->AddUserTasks(pCDL, array);
	if (result != S_OK)
		OutputDebugPrintf("AddUserTasks array2 failed. (0x%x)\n", result);

	/*
	 * Now we can clean up the array and collection variables, so as
	 * to be able to reuse them.
	 */
	result = array->lpVtbl->Release(array);
	if (result != S_OK)
		OutputDebugPrintf("Release array2 failed. (%ld)\n", result);
	array = NULL;
	result = collection->lpVtbl->Release(collection);
	if (result != S_OK)
		OutputDebugPrintf("Release collection2 failed. (%ld)\n", result);
	collection = NULL;

	/*
	 * Commit the jump list.
	 */
	result = pCDL->lpVtbl->CommitList(pCDL);
	if (result != S_OK)
		OutputDebugPrintf("CommitList failed. (%ld)\n", result);
	need_abort = FALSE;

	/*
	 * Clean up.
	 */
cleanup:
	if (pRemoved) pRemoved->lpVtbl->Release(pRemoved);
	if (pCDL && need_abort) pCDL->lpVtbl->AbortList(pCDL);
	if (pCDL) pCDL->lpVtbl->Release(pCDL);
	if (collection) collection->lpVtbl->Release(collection);
	if (array) array->lpVtbl->Release(array);
	if (link) link->lpVtbl->Release(link);
}

void add_to_recent_docs(const char * const sessionname)
{
    IShellLink *link = NULL;

    link = make_shell_link(NULL, sessionname);

    SHAddToRecentDocs(6 /* SHARD_LINK */, link);

    return;
}

/* Clears the entire jumplist. */
void clear_jumplist(void)
{
	ICustomDestinationList *pCDL;

	if (CoCreateInstance(&CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER,
		COMPTR(ICustomDestinationList, &pCDL)) == S_OK) {
		pCDL->lpVtbl->DeleteList(pCDL, NULL);
		pCDL->lpVtbl->Release(pCDL);
	}

}

/* Adds a saved session to the Windows 7 jumplist. */
void add_session_to_jumplist(const char * const sessionname, char *inifile)
{
	if (!isJumpListSupported())
		return;                        /* do nothing on pre-Win7 systems */

	//    add_to_recent_docs(sessionname);

	IniFile = inifile;

	update_jumplist_from_registry();
	return;
}

/* Removes a saved session from the Windows jumplist. */
void remove_session_from_jumplist(const char * const sessionname)
{
	if (!isJumpListSupported())
		return;                        /* do nothing on pre-Win7 systems */

	update_jumplist_from_registry();
	return;
}
