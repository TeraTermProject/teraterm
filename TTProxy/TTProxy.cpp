// ttx.cpp : DLL アプリケーション用のエントリ ポイントを定義します。
//

#include "stdafx.h"

#include "resource.h"

#include "TTProxy.h"

#include "compat_w95.h"

static HINSTANCE myInstance = NULL;

namespace yebisuya {
	HINSTANCE GetInstanceHandle() {
		return myInstance;
	}
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		myInstance = instance;
		return TTProxy::getInstance().processAttach();
	case DLL_PROCESS_DETACH:
		return TTProxy::getInstance().processDetach();
	case DLL_THREAD_ATTACH:
		return TTProxy::getInstance().threadAttach();
	case DLL_THREAD_DETACH:
		return TTProxy::getInstance().threadDetach();
	}
	return FALSE;
}

