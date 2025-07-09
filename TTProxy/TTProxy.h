#ifndef _YEBISOCKS_TTX_H_
#define _YEBISOCKS_TTX_H_

#include "codeconv.h"

#include <YCL/DynamicLinkLibrary.h>
using namespace yebisuya;

#include "ProxyWSockHook.h"

extern "C" __declspec(dllexport) BOOL WINAPI TTXBind(WORD Version, TTXExports* exports);

wchar_t *UILanguageFileW;

class TTProxy : public DynamicLinkLibrary<TTProxy> {
	enum {
		ID_ABOUTMENU       = 53910,
		ID_PROXYSETUPMENU  = 53310,
		ID_ASYNCMESSAGEBOX = 63001,
	};
	enum {
		OPTION_NONE    = 0,
		OPTION_CLEAR   = 1,
		OPTION_REPLACE = 2,
	};
public:
	TTProxy():initialized(false), showing_error_message(false) {
	}
	bool processAttach() {
		DisableThreadLibraryCalls(GetInstanceHandle());
		return true;
	}
private:
	class Shower : public ProxyWSockHook::MessageShower {
	public:
		virtual void showMessage(const char* message)const {
			add_error_message(message);
		}
	} shower;
	friend class Shower;
	bool initialized;
	bool showing_error_message;
	String error_message;
	String realhost;
	PTTSet ts;
	PComVar cv;
	PReadIniFile ORIG_ReadIniFile;
	PWriteIniFile ORIG_WriteIniFile;
	PParseParam ORIG_ParseParam;

	static void add_error_message(const char* message) {
		if (getInstance().error_message != NULL) {
			StringBuffer buffer = (const TCHAR *)getInstance().error_message;
			buffer.append("\n\n");
			buffer.append(message);
			getInstance().error_message = buffer.toString();
		}else{
			getInstance().error_message = message;
		}
		::PostMessage(getInstance().cv->HWin, WM_COMMAND, ID_ASYNCMESSAGEBOX, 0);
	}
	static void read_options(const wchar_t* filename) {
		IniFile inifile(filename, "TTProxy");
		ProxyWSockHook::load(inifile);
		getInstance().initialized = true;
	}
	static void write_options(const wchar_t* filename) {
		IniFile inifile(filename, "TTProxy");
		ProxyWSockHook::save(inifile);
	}
	static wchar_t *get_home_dir_relative_nameW(const wchar_t *basename) {
		wchar_t *full_path = NULL, *dir;

		if (!IsRelativePathW(basename)) {
			return _wcsdup(basename);
		}

		dir = GetHomeDirW(NULL);
		awcscats(&full_path, dir, L"\\", basename, NULL);
		free(dir);
		return full_path;
	}

	static void PASCAL TTXReadINIFile(const wchar_t *fileName, PTTSet ts) {
		getInstance().ORIG_ReadIniFile(fileName, ts);
		read_options(fileName);
	}
	static void PASCAL TTXWriteINIFile(const wchar_t *fileName, PTTSet ts) {
		getInstance().ORIG_WriteIniFile(fileName, ts);
		write_options(fileName);
	}

	static void PASCAL TTXParseParam(wchar_t *param, PTTSet ts, PCHAR DDETopic) {
		size_t param_len = wcslen(param);
		wchar_t option[1024];
		int opt_len = sizeof(option);
		int action;
		wchar_t *start, *cur, *next;

		memset(&option, '\0', opt_len);

		/* the first term shuld be executable filename of Tera Term */
		start = GetParam(option, opt_len, param);

		cur = start;
		while (next = GetParam(option, opt_len, cur)) {
			DequoteParam(option, opt_len, option);
			action = OPTION_NONE;

			if ((option[0] == '-' || option[0] == '/')) {
				if ((option[1] == 'F' || option[1] == 'f') && option[2] == '=') {
					if (option[3] != NULL) {
						wchar_t *f = get_home_dir_relative_nameW(option + 3);
						read_options(f);
						free(f);
					}
				}
			}

			switch (action) {
				case OPTION_CLEAR:
					wmemset(cur, ' ', next-cur);
					break;
				case OPTION_REPLACE:
					wmemset(cur, ' ', next-cur);
					wmemcpy(cur+1, option, wcslen(option));
					break;
			}

			cur = next;
		}

		cur = start;
		while (next = GetParam(option, opt_len, cur)) {
			DequoteParam(option, opt_len, option);
			action = OPTION_NONE;

			if ((option[0] == '-' || option[0] == '/')) {
				if (wcslen(option + 1) >= 6 && option[6] == '=') {
					option[6] = '\0';
					if (_wcsicmp(option + 1, L"proxy") == 0) {
						char *url = ToCharW(option + 7);
						ProxyWSockHook::parseURL(url, TRUE);
						free(url);
						action = OPTION_CLEAR;
					}else{
						option[6] = '=';
					}
				}
				else if (_wcsicmp(option+1, L"noproxy") == 0) {
					// -noproxy �� -proxy=none:// �̕ʖ�
					ProxyWSockHook::parseURL("none://", TRUE);
					action = OPTION_CLEAR;
				}
			}else{
				char *url = ToCharW(option);
				String realhost = ProxyWSockHook::parseURL(url, FALSE);
				free(url);
				if (realhost != NULL) {
					getInstance().realhost = realhost;
					if (realhost.indexOf("://") == -1) {
						action = OPTION_CLEAR;
					}
					else {
						// -proxy= �Ȃ��ŁAproto://proxy:port/ �ȍ~�̎��z�X�g���܂܂�Ă��Ȃ�
						// ttermpro �ŏ������Ă��炤���߁ATTXParseParam �ŏ����Ȃ�
						action = OPTION_REPLACE;
					}
				}
			}

			switch (action) {
				case OPTION_CLEAR:
					wmemset(cur, ' ', next-cur);
					break;
				case OPTION_REPLACE:
					wmemset(cur, ' ', next-cur);
					wmemcpy(cur+1, option, wcslen(option));
					break;
			}

			cur = next;
		}

		getInstance().ORIG_ParseParam(param, ts, DDETopic);
		if (getInstance().ts->HostName[0] == '\0' && getInstance().realhost != NULL) {
			strcpy_s(getInstance().ts->HostName, sizeof getInstance().ts->HostName, getInstance().realhost);
		}
	}

	static void copy_UILanguageFile() {
		UILanguageFileW = getInstance().ts->UILanguageFileW;
	}

	static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
		getInstance().ts = ts;
		getInstance().cv = cv;
		Logger::set_folder(ts->LogDirW);

		ProxyWSockHook::setMessageShower(&getInstance().shower);
	}

	static void PASCAL TTXGetSetupHooks(TTXSetupHooks* hooks) {
		getInstance().ORIG_ReadIniFile = *hooks->ReadIniFile;
		getInstance().ORIG_WriteIniFile = *hooks->WriteIniFile;
		getInstance().ORIG_ParseParam = *hooks->ParseParam;
		*hooks->ReadIniFile = TTXReadINIFile;
		*hooks->WriteIniFile = TTXWriteINIFile;
		*hooks->ParseParam = TTXParseParam;
		copy_UILanguageFile();
	}

	static void PASCAL TTXOpenTCP(TTXSockHooks* hooks) {
		if (!getInstance().initialized) {
			read_options(getInstance().ts->SetupFNameW);
		}
		(FARPROC&) *hooks->Pconnect = ProxyWSockHook::hook_connect((FARPROC) *hooks->Pconnect);
		(FARPROC&) *hooks->PWSAAsyncGetHostByName = ProxyWSockHook::hook_WSAAsyncGetHostByName((FARPROC) *hooks->PWSAAsyncGetHostByName);
		(FARPROC&) *hooks->PWSAAsyncGetAddrInfo = ProxyWSockHook::hook_WSAAsyncGetAddrInfo((FARPROC) *hooks->PWSAAsyncGetAddrInfo);
		(FARPROC&) *hooks->Pfreeaddrinfo = ProxyWSockHook::hook_freeaddrinfo((FARPROC) *hooks->Pfreeaddrinfo);
		(FARPROC&) *hooks->PWSAAsyncSelect = ProxyWSockHook::hook_WSAAsyncSelect((FARPROC) *hooks->PWSAAsyncSelect);
		(FARPROC&) *hooks->PWSACancelAsyncRequest = ProxyWSockHook::hook_WSACancelAsyncRequest((FARPROC) *hooks->PWSACancelAsyncRequest);
	}

	static void PASCAL TTXCloseTCP(TTXSockHooks* hooks) {
		// unhook functions
		ProxyWSockHook::unhook_connect((FARPROC) *hooks->Pconnect);
		ProxyWSockHook::unhook_WSAAsyncGetHostByName((FARPROC) *hooks->PWSAAsyncGetHostByName);
		ProxyWSockHook::unhook_WSAAsyncGetAddrInfo((FARPROC) *hooks->PWSAAsyncGetAddrInfo);
		ProxyWSockHook::unhook_freeaddrinfo((FARPROC) *hooks->Pfreeaddrinfo);
		ProxyWSockHook::unhook_WSAAsyncSelect((FARPROC) *hooks->PWSAAsyncSelect);
		ProxyWSockHook::unhook_WSACancelAsyncRequest((FARPROC) *hooks->PWSACancelAsyncRequest);
	}

	static void PASCAL TTXModifyMenu(HMENU menu) {
		const UINT ID_HELP_ABOUT = 50990;
		const UINT ID_SETUP_TCPIP = 50360;
		InsertMenu(menu, ID_HELP_ABOUT, MF_BYCOMMAND | MF_ENABLED, ID_ABOUTMENU, "About TT&Proxy...");
		InsertMenu(menu, ID_SETUP_TCPIP, MF_BYCOMMAND | MF_ENABLED, ID_PROXYSETUPMENU, "&Proxy...");

		static const DlgTextInfo MenuTextInfo[] = {
			{ ID_ABOUTMENU, "MENU_ABOUT" },
			{ ID_PROXYSETUPMENU, "MENU_SETUP" },
		};
		SetI18nMenuStrsW(menu, "TTProxy", MenuTextInfo, _countof(MenuTextInfo), getInstance().ts->UILanguageFileW);
	}

	static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
		switch (cmd) {
		case ID_ABOUTMENU:
			copy_UILanguageFile();
			SetDialogFont(getInstance().ts->DialogFontNameW, getInstance().ts->DialogFontPoint, getInstance().ts->DialogFontCharSet,
						  getInstance().ts->UILanguageFileW, "TTProxy", "DLG_TAHOMA_FONT");
			ProxyWSockHook::aboutDialog(hWin);
			return 1;
		case ID_PROXYSETUPMENU:
			copy_UILanguageFile();
			SetDialogFont(getInstance().ts->DialogFontNameW, getInstance().ts->DialogFontPoint, getInstance().ts->DialogFontCharSet,
				getInstance().ts->UILanguageFileW, "TTProxy", "DLG_TAHOMA_FONT");
			ProxyWSockHook::setupDialog(hWin);
			return 1;
		case ID_ASYNCMESSAGEBOX:
			if (getInstance().error_message != NULL) {
				String msg = getInstance().error_message;
				getInstance().showing_error_message = true;
				getInstance().error_message = NULL;
				::MessageBox(hWin, msg, "TTProxy", MB_OK | MB_ICONERROR);
				getInstance().showing_error_message = false;
				if (getInstance().error_message != NULL) {
					::PostMessage(hWin, WM_COMMAND, ID_ASYNCMESSAGEBOX, 0);
				}
			}
			return 1;
		default:
			return 0;
		}
	}

	static void PASCAL TTXSetCommandLine(wchar_t *cmd, int cmdlen, PGetHNRec rec) {
		String urlA = ProxyWSockHook::generateURL();
		wchar_t *urlW = ToWcharA(urlA);
		WString url = urlW;
		free(urlW);
		if (url != NULL) {
			if (wcslen(cmd) + 8 + url.length() >= (unsigned) cmdlen)
				return;
			wcscat_s(cmd, cmdlen, L" -proxy=");
			wcscat_s(cmd, cmdlen, url);
		}
	}

	static void PASCAL TTXEnd() {
		if (getInstance().error_message != NULL) {
			::MessageBox(NULL, getInstance().error_message, "TTProxy", MB_OK | MB_ICONERROR);
			getInstance().error_message = NULL;
		}
	}

	friend BOOL PASCAL TTXBind(WORD Version, TTXExports* exports) {
		static const TTXExports EXPORTS = {
			/* This must contain the size of the structure. See below for its usage. */
			sizeof EXPORTS,
			/* load first */
			10,

			/* Now we just list the functions that we've implemented. */
			TTProxy::TTXInit,
			NULL,
			TTProxy::TTXGetSetupHooks,
			TTProxy::TTXOpenTCP,
			TTProxy::TTXCloseTCP,
			NULL,
			TTProxy::TTXModifyMenu,
			NULL,
			TTProxy::TTXProcessCommand,
			TTProxy::TTXEnd,
			TTProxy::TTXSetCommandLine
		};

		int size = sizeof EXPORTS - sizeof exports->size;
		/* do version checking if necessary */
		/* if (Version!=TTVERSION) return FALSE; */

		if (size > exports->size) {
			size = exports->size;
		}
		memcpy((char*) exports + sizeof exports->size,
			(char*) &EXPORTS + sizeof exports->size,
			size);
		return TRUE;
	}
};

#endif//_YEBISOCKS_TTX_H_
