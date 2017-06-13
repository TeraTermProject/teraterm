#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"

// #include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* for _findXXXX() functions */
#include <io.h>
#include "ttwinman.h"
#include "ttplugin.h"
#include "ttplug.h"
#undef TTXOpenTCP
#undef TTXCloseTCP
#undef TTXOpenFile
#undef TTXCloseFile
#undef TTXGetUIHooks
#undef TTXGetSetupHooks

#define MAXNUMEXTENSIONS 32
static HANDLE LibHandle[MAXNUMEXTENSIONS];
static int NumExtensions = 0;
static TTXExports * * Extensions;

typedef struct _ExtensionList {
  TTXExports * exports;
  struct _ExtensionList * next;
} ExtensionList;

static int compareOrder(const void * e1, const void * e2) {
  TTXExports * * exports1 = (TTXExports * *)e1;
  TTXExports * * exports2 = (TTXExports * *)e2;

  return (*exports1)->loadOrder - (*exports2)->loadOrder;
}

static void loadExtension(ExtensionList * * extensions, char const * fileName) {
  char buf[1024];
  DWORD err;
  char uimsg[MAX_UIMSG];

  if (NumExtensions>=MAXNUMEXTENSIONS) return;
  LibHandle[NumExtensions] = LoadLibrary(fileName);
  if (LibHandle[NumExtensions] != NULL) {
    TTXBindProc bind = (TTXBindProc)GetProcAddress(LibHandle[NumExtensions], "_TTXBind@8");
    if (bind==NULL)
      bind = (TTXBindProc)GetProcAddress(LibHandle[NumExtensions], "TTXBind");
    if (bind != NULL) {
      ExtensionList * newExtension =
        (ExtensionList *)malloc(sizeof(ExtensionList));

      newExtension->exports = (TTXExports *)malloc(sizeof(TTXExports));
      memset(newExtension->exports, 0, sizeof(TTXExports));
      newExtension->exports->size = sizeof(TTXExports);
      if (bind(TTVERSION,(TTXExports *)newExtension->exports)) {
        newExtension->next = *extensions;
        *extensions = newExtension;
        NumExtensions++;
        return;
      } else {
	free(newExtension->exports);
	free(newExtension);
      }
    }
    FreeLibrary(LibHandle[NumExtensions]);
  }

  err = GetLastError();
  // 言語ファイルによるメッセージの国際化を行っているが、この時点では設定が
  // まだ読み込まれていない為、メッセージが英語のままとなる。要検討。
  get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg), "Tera Term: Error", ts.UILanguageFile);
  get_lang_msg("MSG_LOAD_EXT_ERROR", ts.UIMsg, sizeof(ts.UIMsg), "Cannot load extension %s (%d)", ts.UILanguageFile);
  _snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, fileName, err);
  MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONEXCLAMATION);
}

void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  ExtensionList * extensionList = NULL;
  int i;

  // 環境変数の設定有無に関わらず、TTXを有効にする。
  //if (getenv("TERATERM_EXTENSIONS") != NULL) {
  if (1) {
    char buf[1024];
    struct _finddata_t searchData;
    long searchHandle;

    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s\\TTX*.DLL", ts->HomeDir);

    searchHandle = _findfirst(buf, &searchData);
    if (searchHandle != -1L) {
      do {
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s\\%s", ts->HomeDir, searchData.name);
	loadExtension(&extensionList, buf);
      } while (_findnext(searchHandle, &searchData)==0);
      _findclose(searchHandle);
    }

    if (NumExtensions==0) return;

    Extensions = (TTXExports * *)malloc(sizeof(TTXExports *)*NumExtensions);
    for (i = 0; i < NumExtensions; i++) {
      ExtensionList * old;

      Extensions[i] = extensionList->exports;
      old = extensionList;
      extensionList = extensionList->next;
      free(old);
    }

    qsort(Extensions, NumExtensions, sizeof(Extensions[0]), compareOrder);

    for (i = 0; i < NumExtensions; i++) {
      if (Extensions[i]->TTXInit != NULL) {
        Extensions[i]->TTXInit(ts, cv);
      }
    }
  }
}

void PASCAL TTXInternalOpenTCP(TTXSockHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXOpenTCP != NULL) {
      Extensions[i]->TTXOpenTCP(hooks);
    }
  }
}

void PASCAL TTXInternalCloseTCP(TTXSockHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXCloseTCP != NULL) {
      Extensions[i]->TTXCloseTCP(hooks);
    }
  }
}

void PASCAL TTXInternalOpenFile(TTXFileHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXOpenFile != NULL) {
      Extensions[i]->TTXOpenFile(hooks);
    }
  }
}

void PASCAL TTXInternalCloseFile(TTXFileHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXCloseFile != NULL) {
      Extensions[i]->TTXCloseFile(hooks);
    }
  }
}

void PASCAL TTXInternalGetUIHooks(TTXUIHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXGetUIHooks != NULL) {
      Extensions[i]->TTXGetUIHooks(hooks);
    }
  }
}

void PASCAL TTXInternalGetSetupHooks(TTXSetupHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXGetSetupHooks != NULL) {
      Extensions[i]->TTXGetSetupHooks(hooks);
    }
  }
}

void PASCAL TTXSetWinSize(int rows, int cols) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXSetWinSize != NULL) {
      Extensions[i]->TTXSetWinSize(rows, cols);
    }
  }
}

void PASCAL TTXModifyMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXModifyMenu != NULL) {
      Extensions[i]->TTXModifyMenu(menu);
    }
  }
}

void PASCAL TTXModifyPopupMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXModifyPopupMenu != NULL) {
      Extensions[i]->TTXModifyPopupMenu(menu);
    }
  }
}

BOOL PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXProcessCommand != NULL) {
      if (Extensions[i]->TTXProcessCommand(hWin,cmd)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

void PASCAL TTXEnd(void) {
  int i;

  if (NumExtensions==0) return;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXEnd != NULL) {
      Extensions[i]->TTXEnd();
    }
  }

  for (i=0; i<NumExtensions; i++)
    FreeLibrary(LibHandle[i]);

  for (i = 0; i < NumExtensions; i++) {
	  free(Extensions[i]);
  }

  free(Extensions);
  NumExtensions = 0;
}

void PASCAL TTXSetCommandLine(PCHAR cmd, int cmdlen, PGetHNRec rec) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXSetCommandLine != NULL) {
      Extensions[i]->TTXSetCommandLine(cmd, cmdlen, rec);
    }
  }
}
