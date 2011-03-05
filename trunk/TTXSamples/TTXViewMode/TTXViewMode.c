#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "compat_w95.h"

#define ORDER 4000
#define ID_MENU_VIEWMODE 55200
#define ID_MENU_SETPASS  55210

#define SECTION "TTXViewMode"

static HANDLE hInst;

typedef struct {
  PTTSet ts;
  PComVar cv;
  Tsend origPsend;
  TWriteFile origPWriteFile;
  PReadIniFile origReadIniFile;
  PWriteIniFile origWriteIniFile;
  HMENU SetupMenu;
  HMENU ControlMenu;
  BOOL enable;
  char password[50];
} TInstVar;

typedef TInstVar FAR * PTInstVar;
PTInstVar pvar;
static TInstVar InstVar;

#define GetFileMenu(menu)	GetSubMenuByChildID(menu, ID_FILE_NEWCONNECTION)
#define GetEditMenu(menu)	GetSubMenuByChildID(menu, ID_EDIT_COPY2)
#define GetSetupMenu(menu)	GetSubMenuByChildID(menu, ID_SETUP_TERMINAL)
#define GetControlMenu(menu)	GetSubMenuByChildID(menu, ID_CONTROL_RESETTERMINAL)
#define GetHelpMenu(menu)	GetSubMenuByChildID(menu, ID_HELP_ABOUT)

HMENU GetSubMenuByChildID(HMENU menu, UINT id) {
  int i, j, items, subitems, cur_id;
  HMENU m;

  items = GetMenuItemCount(menu);

  for (i=0; i<items; i++) {
    if (m = GetSubMenu(menu, i)) {
      subitems = GetMenuItemCount(m);
      for (j=0; j<subitems; j++) {
        cur_id = GetMenuItemID(m, j);
	if (cur_id == id) {
	  return m;
	}
      }
    }
  }
  return NULL;
}

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->origPsend = NULL;
  pvar->origPWriteFile = NULL;
  pvar->origReadIniFile = NULL;
  pvar->origWriteIniFile = NULL;
}

static int PASCAL FAR TTXsend(SOCKET s, const char FAR *buf, int len, int flags) {
  if (pvar->enable) {
    return len;
  }
  else {
    return pvar->origPsend(s, buf, len, flags);
  }
}

static BOOL PASCAL FAR TTXWriteFile(HANDLE fh, LPCVOID buff, DWORD len, LPDWORD wbytes, LPOVERLAPPED wol) {
  if (pvar->enable) {
    *wbytes = len;
    return TRUE;
  }
  else {
    return pvar->origPWriteFile(fh, buff, len, wbytes, wol);
  }
}

static void PASCAL FAR TTXOpenTCP(TTXSockHooks FAR * hooks) {
  pvar->origPsend = *hooks->Psend;
  *hooks->Psend = TTXsend;
}

static void PASCAL FAR TTXCloseTCP(TTXSockHooks FAR * hooks) {
  if (pvar->origPsend) {
    *hooks->Psend = pvar->origPsend;
  }
}

static void PASCAL FAR TTXOpenFile(TTXFileHooks FAR * hooks) {
  pvar->origPWriteFile = *hooks->PWriteFile;
  *hooks->PWriteFile = TTXWriteFile;
}

static void PASCAL FAR TTXCloseFile(TTXFileHooks FAR * hooks) {
  if (pvar->origPWriteFile) {
    *hooks->PWriteFile = pvar->origPWriteFile;
  }
}

static void PASCAL FAR TTXReadIniFile(PCHAR fn, PTTSet ts) {
  pvar->origReadIniFile(fn, ts);
  GetPrivateProfileString(SECTION, "Password", "", pvar->password, sizeof(pvar->password), fn);
  return;
}

static void PASCAL FAR TTXWriteIniFile(PCHAR fn, PTTSet ts) {
  pvar->origWriteIniFile(fn, ts);
//  WritePrivateProfileString(SECTION, "Password", pvar->password, fn);
  return;
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = TTXReadIniFile;
  pvar->origWriteIniFile = *hooks->WriteIniFile;
  *hooks->WriteIniFile = TTXWriteIniFile;
}

static void PASCAL FAR TTXModifyMenu(HMENU menu) {
  UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

  pvar->SetupMenu = GetSetupMenu(menu);
  pvar->ControlMenu = GetControlMenu(menu);

  InsertMenu(pvar->SetupMenu, ID_SETUP_ADDITIONALSETTINGS, flag, ID_MENU_SETPASS, "&ViewMode password"); 
  if (pvar->enable) flag |= MF_CHECKED;
  InsertMenu(pvar->ControlMenu, ID_CONTROL_MACRO, flag,  ID_MENU_VIEWMODE, "&View mode");
  InsertMenu(pvar->ControlMenu, ID_CONTROL_MACRO, MF_BYCOMMAND | MF_SEPARATOR,  0, NULL);
}

static void PASCAL FAR TTXModifyPopupMenu(HMENU menu) {
  if (menu == pvar->ControlMenu) {
    if (pvar->cv->Ready)
      EnableMenuItem(pvar->ControlMenu, ID_MENU_VIEWMODE, MF_BYCOMMAND | MF_ENABLED);
    else
      EnableMenuItem(pvar->ControlMenu, ID_MENU_VIEWMODE, MF_BYCOMMAND | MF_GRAYED);
  }
}

static LRESULT CALLBACK ViewModeInputPass(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  char password[50];
  
  switch (msg) {
    case WM_INITDIALOG:
      /* nothing to do */
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
	  GetDlgItemText(dlg, IDC_CURPASS, password, sizeof(password));
	  if (strcmp(pvar->password, password) == 0) {
	    EndDialog(dlg, IDOK);
	  }
	  else {
	    MessageBox(NULL, "Invalid Password", "Invalid Password", MB_OK | MB_ICONEXCLAMATION);
	    EndDialog(dlg, IDCANCEL);
	  }
	  return TRUE;
	case IDCANCEL:
	  EndDialog(dlg, IDCANCEL);
	  return TRUE;
      }
      break;
  }
  return FALSE;
}

static LRESULT CALLBACK ViewModeSetPass(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  char passwd1[50], passwd2[50];

  switch (msg) {
    case WM_INITDIALOG:
      /* nothing to do */
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
	  GetDlgItemText(dlg, IDC_CURPASS, passwd1, sizeof(passwd1));
	  if (strcmp(pvar->password, passwd1) == 0) {
	    GetDlgItemText(dlg, IDC_NEWPASS, passwd1, sizeof(passwd1));
	    GetDlgItemText(dlg, IDC_RETPASS, passwd2, sizeof(passwd2));
	    if (strcmp(passwd1, passwd2) == 0) {
	      strncpy_s(pvar->password, sizeof(pvar->password), passwd1, _TRUNCATE);
	      MessageBox(NULL, "Password changed", "TTXViewMode", MB_OK | MB_ICONEXCLAMATION);
	      EndDialog(dlg, IDOK);
	    }
	    else {
	      MessageBox(NULL, "New password not matched.", "TTXViewMode", MB_OK | MB_ICONEXCLAMATION);
	      EndDialog(dlg, IDCANCEL);
	    }
	  }
	  else {
	    MessageBox(NULL, "Invalid Password", "TTXViewMode", MB_OK | MB_ICONEXCLAMATION);
	    EndDialog(dlg, IDCANCEL);
	  }
	  return TRUE;
	case IDCANCEL:
	  EndDialog(dlg, IDCANCEL);
	  return TRUE;
      }
      break;
  }
  return FALSE;
}

static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
  switch (cmd) {
    case ID_MENU_VIEWMODE:
      if (pvar->enable) {
        if (strcmp(pvar->password, "") != 0) {
          switch (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_INPUT_PASSWORD), hWin, ViewModeInputPass, (LPARAM)NULL)) {
            case IDOK:
              pvar->enable = FALSE;
	      CheckMenuItem(pvar->ControlMenu, ID_MENU_VIEWMODE, MF_BYCOMMAND | MF_UNCHECKED);
	      break;
	    case IDCANCEL:
	      /* nothing to do */
	      break;
	    case -1:
	      MessageBox(hWin, "TTXViewMode Error", "Can't display dialog box.",
	                 MB_OK | MB_ICONEXCLAMATION);
	      break;
	  }
        }
	else {
          pvar->enable = FALSE;
	  CheckMenuItem(pvar->ControlMenu, ID_MENU_VIEWMODE, MF_BYCOMMAND | MF_UNCHECKED);
	}
      }
      else {
        pvar->enable = TRUE;
	CheckMenuItem(pvar->ControlMenu, ID_MENU_VIEWMODE, MF_BYCOMMAND | MF_CHECKED);
      }
      return 1;
    case ID_MENU_SETPASS:
      switch (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SET_PASSWORD), hWin, ViewModeSetPass, (LPARAM)NULL)) {
	case IDOK:
	  break;
	case IDCANCEL:
	  break;
	case -1:
	  MessageBox(hWin, "TTXViewMode Error", "Can't display dialog box.",
	    MB_OK | MB_ICONEXCLAMATION);
	  break;
      }
      return 1;
  }
  return 0;
}

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  NULL, // TTXGetUIHooks,
  TTXGetSetupHooks,
  TTXOpenTCP,
  TTXCloseTCP,
  NULL, // TTXSetWinSize,
  TTXModifyMenu,
  TTXModifyPopupMenu,
  TTXProcessCommand,
  NULL, // TTXEnd,
  NULL, // TTXSetCommandLine,
  TTXOpenFile,
  TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL FAR TTXBind(WORD Version, TTXExports FAR * exports) {
  int size = sizeof(Exports) - sizeof(exports->size);
  /* do version checking if necessary */
  /* if (Version!=TTVERSION) return FALSE; */

  if (size > exports->size) {
    size = exports->size;
  }
  memcpy((char FAR *)exports + sizeof(exports->size),
    (char FAR *)&Exports + sizeof(exports->size),
    size);
  return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance, 
		    ULONG ul_reason_for_call,
		    LPVOID lpReserved)
{
  switch( ul_reason_for_call ) {
    case DLL_THREAD_ATTACH:
      /* do thread initialization */
      break;
    case DLL_THREAD_DETACH:
      /* do thread cleanup */
      break;
    case DLL_PROCESS_ATTACH:
      /* do process initialization */
      DoCover_IsDebuggerPresent();
      hInst = hInstance;
      pvar = &InstVar;
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
