/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004-2017 TeraTerm Project
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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "fwdui.h"

#include "resource.h"
#include "x11util.h"
#include "util.h"
#include "dlglib.h"

#include "servicenames.h"
#include "helpid.h"

#undef DialogBoxParam
#define DialogBoxParam(p1,p2,p3,p4,p5) \
	TTDialogBoxParam(p1,p2,p3,p4,p5)
#undef EndDialog
#define EndDialog(p1,p2) \
	TTEndDialog(p1, p2)

typedef struct {
	FWDRequestSpec *spec;
	PTInstVar pvar;
} FWDEditClosure;

static void make_X_forwarding_spec(FWDRequestSpec *spec, PTInstVar pvar)
{
	spec->type = FWD_REMOTE_X11_TO_LOCAL;
	spec->from_port = -1;
	X11_get_DISPLAY_info(pvar, spec->to_host, sizeof(spec->to_host),
	                     &spec->to_port, &spec->x11_screen);
	UTIL_get_lang_msg("MSG_FWD_REMOTE_XSERVER", pvar, "remote X server");
	strncpy_s(spec->from_port_name, sizeof(spec->from_port_name),
	          pvar->ts->UIMsg, _TRUNCATE);
	UTIL_get_lang_msg("MSG_FWD_REMOTE_XSCREEN", pvar, "X server (display %d:%d)");
	_snprintf_s(spec->to_port_name, sizeof(spec->to_port_name), _TRUNCATE,
	            pvar->ts->UIMsg, spec->to_port - 6000, spec->x11_screen);
}

static BOOL parse_request(FWDRequestSpec *request, char *str, PTInstVar pvar)
{
	char *tmp, *ch;
	int len, i, argc = 0, bracketed = 0;
	char *argv[4];
	char hostname[256];
	int start;

	if ((tmp = strchr(str, ';')) != NULL) {
		len = tmp - str;
	}
	else {
		len = strlen(str);
	}
	tmp = _malloca(sizeof(char) * (len+1));
	strncpy_s(tmp, sizeof(char) * (len+1), str, _TRUNCATE);

	if (*tmp == 'L' || *tmp == 'l') {
		request->type = FWD_LOCAL_TO_REMOTE;
	} else if (*tmp == 'R' || *tmp == 'r') {
		request->type = FWD_REMOTE_TO_LOCAL;
	} else if (*tmp == 'D' || *tmp == 'd') {
		request->type = FWD_LOCAL_DYNAMIC;
	} else if (*tmp == 'X' || *tmp == 'x') {
		make_X_forwarding_spec(request, pvar);
		return TRUE;
	} else {
		return FALSE;
	}
	tmp++;

	argv[argc++] = tmp;
	for (i=0; i<len; i++ ) {
		ch = (tmp+i);
		if (*ch == ':' && !bracketed) {
			if (argc >= 4) {
				argc++;
				break;
			}
			*ch = '\0';
			argv[argc++] = tmp+i+1;
		}
		else if (*ch == '[' && !bracketed) {
			bracketed = 1;
		}
		else if (*ch == ']' && bracketed) {
			bracketed = 0;
		}
	}

	strncpy_s(request->bind_address, sizeof(request->bind_address), "localhost", _TRUNCATE);
	i=0;
	switch (argc) {
		case 4:
			if (*argv[i] == '\0' || strcmp(argv[i], "*") == 0) {
				strncpy_s(request->bind_address, sizeof(request->bind_address), "0.0.0.0", _TRUNCATE);
			}
			else {
				// IPv6 アドレスの "[", "]" があれば削除
				start = 0;
				strncpy_s(hostname, sizeof(hostname), argv[i], _TRUNCATE);
				if (strlen(hostname) > 0 &&
				    hostname[strlen(hostname)-1] == ']') {
					hostname[strlen(hostname)-1] = '\0';
				}
				if (hostname[0] == '[') {
					start = 1;
				}
				strncpy_s(request->bind_address, sizeof(request->bind_address),
				          hostname + start, _TRUNCATE);
			}
			i++;
			// don't break here

		case 3:
			request->from_port = parse_port(argv[i], request->from_port_name,
			                                sizeof(request->from_port_name));
			if (request->from_port < 0) {
				return FALSE;
			}
			i++;

			// IPv6 アドレスの "[", "]" があれば削除
			start = 0;
			strncpy_s(hostname, sizeof(hostname), argv[i], _TRUNCATE);
			if (strlen(hostname) > 0 &&
			    hostname[strlen(hostname)-1] == ']') {
				hostname[strlen(hostname)-1] = '\0';
			}
			if (hostname[0] == '[') {
				start = 1;
			}
			strncpy_s(request->to_host, sizeof(request->to_host),
			          hostname + start, _TRUNCATE);
			i++;

			request->to_port = parse_port(argv[i], request->to_port_name,
			                              sizeof(request->to_port_name));
			if (request->to_port < 0) {
				return FALSE;
			}

			break;

		case 2:
			if (request->type != FWD_LOCAL_DYNAMIC) {
				return FALSE;
			}
			if (*argv[i] == '\0' || strcmp(argv[i], "*") == 0) {
				strncpy_s(request->bind_address, sizeof(request->bind_address),
				          "0.0.0.0", _TRUNCATE);
			}
			else {
				// IPv6 アドレスの "[", "]" があれば削除
				start = 0;
				strncpy_s(hostname, sizeof(hostname), argv[i], _TRUNCATE);
				if (strlen(hostname) > 0 &&
				    hostname[strlen(hostname)-1] == ']') {
					hostname[strlen(hostname)-1] = '\0';
				}
				if (hostname[0] == '[') {
					start = 1;
				}
				strncpy_s(request->bind_address, sizeof(request->bind_address),
				          hostname + start, _TRUNCATE);
			}
			i++;
			// FALLTHROUGH
		case 1:
			if (request->type != FWD_LOCAL_DYNAMIC) {
				return FALSE;
			}
			request->from_port = parse_port(argv[i], request->from_port_name,
			                                sizeof(request->from_port_name));
			if (request->from_port < 0) {
				return FALSE;
			}
			i++;

			request->to_host[0] = '\0';
			request->to_port = parse_port("0", request->to_port_name,
			                              sizeof(request->to_port_name));
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

static void FWDUI_save_settings(PTInstVar pvar)
{
	int num_specs = FWD_get_num_request_specs(pvar);
	FWDRequestSpec *requests =
		(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * num_specs);
	int i;
	char *str = pvar->settings.DefaultForwarding;
	int str_remaining = sizeof(pvar->settings.DefaultForwarding) - 1;
	char format[20];

	FWD_get_request_specs(pvar, requests, num_specs);

	*str = 0;
	str[str_remaining] = 0;

	for (i = 0; i < num_specs; i++) {
		if (str_remaining > 0 && i > 0) {
			str[0] = ';';
			str[1] = 0;
			str++;
			str_remaining--;
		}

		if (str_remaining > 0) {
			FWDRequestSpec *spec = requests + i;
			int chars;

			// IPv6 アドレスなら "[", "]" を付加して文字列化
			switch (spec->type) {
			case FWD_LOCAL_TO_REMOTE:
				if (strcmp(spec->bind_address,"localhost") == 0) {
					if (strchr(spec->to_host, ':') == NULL) {
						strncpy_s(format, sizeof(format), "L%s:%s:%s", _TRUNCATE);
					}
					else {
						strncpy_s(format, sizeof(format), "L%s:[%s]:%s", _TRUNCATE);
					}
					_snprintf_s(str, str_remaining, _TRUNCATE, format,
					            spec->from_port_name, spec->to_host,
					            spec->to_port_name);
				}
				else {
					if (strchr(spec->bind_address, ':') == NULL) {
						if (strchr(spec->to_host, ':') == NULL) {
							strncpy_s(format, sizeof(format), "L%s:%s:%s:%s", _TRUNCATE);
						}
						else {
							strncpy_s(format, sizeof(format), "L%s:%s:[%s]:%s", _TRUNCATE);
						}
					}
					else {
						if (strchr(spec->to_host, ':') == NULL) {
							strncpy_s(format, sizeof(format), "L[%s]:%s:%s:%s", _TRUNCATE);
						}
						else {
							strncpy_s(format, sizeof(format), "L[%s]:%s:[%s]:%s", _TRUNCATE);
						}
					}
					_snprintf_s(str, str_remaining, _TRUNCATE, format,
					            spec->bind_address, spec->from_port_name,
					            spec->to_host, spec->to_port_name);
				}
				break;
			case FWD_REMOTE_TO_LOCAL:
				if (strcmp(spec->bind_address,"localhost") == 0) {
					if (strchr(spec->to_host, ':') == NULL) {
						strncpy_s(format, sizeof(format), "R%s:%s:%s", _TRUNCATE);
					}
					else {
						strncpy_s(format, sizeof(format), "R%s:[%s]:%s", _TRUNCATE);
					}
					_snprintf_s(str, str_remaining, _TRUNCATE, format,
					            spec->from_port_name, spec->to_host,
					            spec->to_port_name);
				}
				else {
					if (strchr(spec->bind_address, ':') == NULL) {
						if (strchr(spec->to_host, ':') == NULL) {
							strncpy_s(format, sizeof(format), "R%s:%s:%s:%s", _TRUNCATE);
						}
						else {
							strncpy_s(format, sizeof(format), "R%s:%s:[%s]:%s", _TRUNCATE);
						}
					}
					else {
						if (strchr(spec->to_host, ':') == NULL) {
							strncpy_s(format, sizeof(format), "R[%s]:%s:%s:%s", _TRUNCATE);
						}
						else {
							strncpy_s(format, sizeof(format), "R[%s]:%s:[%s]:%s", _TRUNCATE);
						}
					}
					_snprintf_s(str, str_remaining, _TRUNCATE, format,
					            spec->bind_address, spec->from_port_name,
					            spec->to_host, spec->to_port_name);
				}
				break;
			case FWD_REMOTE_X11_TO_LOCAL:
				_snprintf_s(str, str_remaining, _TRUNCATE, "X");
				break;
			case FWD_LOCAL_DYNAMIC:
				_snprintf_s(str, str_remaining, _TRUNCATE, "D%s:%s",
				            spec->bind_address, spec->from_port_name);
			}

			chars = strlen(str);
			str += chars;
			str_remaining -= chars;
		}
	}

	free(requests);
}

void FWDUI_load_settings(PTInstVar pvar)
{
	char *str = pvar->settings.DefaultForwarding;

	if (str[0] != 0) {
		int i, ch, j;
		FWDRequestSpec *requests;

		j = 1;
		for (i = 0; (ch = str[i]) != 0; i++) {
			if (ch == ';') {
				j++;
			}
		}

		requests =
			(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * j);

		j = 0;
		if (parse_request(requests, str, pvar)) {
			j++;
		}
		for (i = 0; (ch = str[i]) != 0; i++) {
			if (ch == ';') {
				if (parse_request(requests + j, str + i + 1, pvar)) {
					j++;
				}
			}
		}

		qsort(requests, j, sizeof(FWDRequestSpec), FWD_compare_specs);

		for (i = 0; i < j - 1; i++) {
			if (FWD_compare_specs(requests + i, requests + i + 1) == 0) {
				memmove(requests + i, requests + i + 1,
				        sizeof(FWDRequestSpec) * (j - 1 - i));
				i--;
				j--;
			}
		}

		FWD_set_request_specs(pvar, requests, j);
		FWDUI_save_settings(pvar);

		free(requests);
	}
}

void FWDUI_init(PTInstVar pvar)
{
	FWDUI_load_settings(pvar);
}

void FWDUI_end(PTInstVar pvar)
{
}

void FWDUI_open(PTInstVar pvar)
{
}

static void set_verbose_port(char *buf, int bufsize, int port, char *name)
{
	char *p = name;

	while (*p && isdigit(*p)) {
		p++;
	}
	if (*p == 0) {
		strncpy_s(buf, bufsize, name, _TRUNCATE);
	}
	else {
		_snprintf_s(buf, bufsize, _TRUNCATE, "%d (%s)", port, name);
	}
}

static void get_spec_string(FWDRequestSpec *spec, char *buf,
                            int bufsize, PTInstVar pvar)
{
	char verbose_from_port[64];
	char verbose_to_port[64];

	switch (spec->type) {
	case FWD_REMOTE_TO_LOCAL:
		set_verbose_port(verbose_from_port, sizeof(verbose_from_port),
		                 spec->from_port, spec->from_port_name);
		set_verbose_port(verbose_to_port, sizeof(verbose_to_port),
		                 spec->to_port, spec->to_port_name);
		UTIL_get_lang_msg("MSG_FWD_REMOTE", pvar,
		                  "Remote \"%s\" port %s to local \"%s\" port %s");
		_snprintf_s(buf, bufsize, _TRUNCATE, pvar->ts->UIMsg,
		            spec->bind_address, verbose_from_port,
		            spec->to_host, verbose_to_port);
		break;
	case FWD_LOCAL_TO_REMOTE:
		set_verbose_port(verbose_from_port, sizeof(verbose_from_port),
		                 spec->from_port, spec->from_port_name);
		set_verbose_port(verbose_to_port, sizeof(verbose_to_port),
		                 spec->to_port, spec->to_port_name);
		UTIL_get_lang_msg("MSG_FWD_LOCAL", pvar,
		                  "Local \"%s\" port %s to remote \"%s\" port %s");
		_snprintf_s(buf, bufsize, _TRUNCATE, pvar->ts->UIMsg,
		            spec->bind_address, verbose_from_port,
		            spec->to_host, verbose_to_port);
		break;
	case FWD_REMOTE_X11_TO_LOCAL:
		UTIL_get_lang_msg("MSG_FWD_X", pvar,
		                  "Remote X applications to local X server");
		strncpy_s(buf, bufsize, pvar->ts->UIMsg, _TRUNCATE);
		break;
	case FWD_LOCAL_DYNAMIC:
		set_verbose_port(verbose_from_port, sizeof(verbose_from_port),
		                 spec->from_port, spec->from_port_name);
		UTIL_get_lang_msg("MSG_FWD_DYNAMIC", pvar, "Local \"%s\" port %s to remote dynamic");
		_snprintf_s(buf, bufsize, _TRUNCATE, pvar->ts->UIMsg,
		            spec->bind_address, verbose_from_port);
		break;
	}
}

static void update_listbox_selection(HWND dlg)
{
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int cursel = SendMessage(listbox, LB_GETCURSEL, 0, 0);

	EnableWindow(GetDlgItem(dlg, IDC_EDIT), cursel >= 0);
	EnableWindow(GetDlgItem(dlg, IDC_REMOVE), cursel >= 0);
}

static void init_listbox_selection(HWND dlg)
{
	SendMessage(GetDlgItem(dlg, IDC_SSHFWDLIST), LB_SETCURSEL, 0, 0);
	update_listbox_selection(dlg);
}

static int add_spec_to_listbox(HWND dlg, FWDRequestSpec *spec, PTInstVar pvar)
{
	char buf[1024];
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int index;

	get_spec_string(spec, buf, sizeof(buf), pvar);

	index = SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM) buf);

	if (index >= 0) {
		FWDRequestSpec *listbox_spec = malloc(sizeof(FWDRequestSpec));

		*listbox_spec = *spec;
		if (SendMessage
			(listbox, LB_SETITEMDATA, index,
			 (LPARAM) listbox_spec) == LB_ERR) {
			free(listbox_spec);
		}
	}

	return index;
}

static void init_fwd_dlg(PTInstVar pvar, HWND dlg)
{
	int num_specs = FWD_get_num_request_specs(pvar);
	FWDRequestSpec *requests =
		(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * num_specs);
	int i;
	const static DlgTextInfo text_info[] = {
		{ 0, "DLG_FWD_TITLE" },
		{ IDC_PORTFORWARD, "DLG_FWDSETUP_LIST" },
		{ IDC_ADD, "DLG_FWDSETUP_ADD" },
		{ IDC_EDIT, "DLG_FWDSETUP_EDIT" },
		{ IDC_REMOVE, "DLG_FWDSETUP_REMOVE" },
		{ IDC_XFORWARD, "DLG_FWDSETUP_X" },
		{ IDC_SSHFWDX11, "DLG_FWDSETUP_XAPP" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_SSHFWDSETUP_HELP, "BTN_HELP" },
	};
	SetI18DlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

	FWD_get_request_specs(pvar, requests, num_specs);

	for (i = 0; i < num_specs; i++) {
		if (requests[i].type == FWD_REMOTE_X11_TO_LOCAL) {
			CheckDlgButton(dlg, IDC_SSHFWDX11, TRUE);
		} else {
			add_spec_to_listbox(dlg, requests + i, pvar);
		}
	}

	free(requests);

	init_listbox_selection(dlg);
}

static void free_listbox_spec(HWND listbox, int selection)
{
	FWDRequestSpec *spec = (FWDRequestSpec *)
		SendMessage(listbox, LB_GETITEMDATA, selection, 0);

	if (spec != NULL) {
		free(spec);
	}
}

static void free_all_listbox_specs(HWND dlg)
{
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int i;

	for (i = SendMessage(listbox, LB_GETCOUNT, 0, 0) - 1; i >= 0; i--) {
		free_listbox_spec(listbox, i);
	}
}

static BOOL end_fwd_dlg(PTInstVar pvar, HWND dlg)
{
	char buf[1024];
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int num_items = SendMessage(listbox, LB_GETCOUNT, 0, 0);
	BOOL X_enabled = IsDlgButtonChecked(dlg, IDC_SSHFWDX11);
	int num_specs = X_enabled ? 1 : 0;
	FWDRequestSpec *specs =
		(FWDRequestSpec *) malloc(sizeof(FWDRequestSpec) * (num_specs + num_items));
	int i;
	int num_unspecified_forwardings = 0;

	for (i = 0; i < num_items; i++) {
		FWDRequestSpec *spec = (FWDRequestSpec *)
			SendMessage(listbox, LB_GETITEMDATA, i, 0);

		if (spec != NULL) {
			specs[num_specs] = *spec;
			num_specs++;
		}
	}

	if (X_enabled) {
		make_X_forwarding_spec(specs, pvar);
	}

	qsort(specs, num_specs, sizeof(FWDRequestSpec), FWD_compare_specs);

	buf[0] = '\0';
	for (i = 0; i < num_specs; i++) {
		if (i < num_specs - 1 && FWD_compare_specs(specs + i, specs + i + 1) == 0) {
			switch (specs[i].type) {
			case FWD_REMOTE_TO_LOCAL:
				UTIL_get_lang_msg("MSG_SAME_SERVERPORT_ERROR", pvar,
				                  "You cannot have two forwarding from the same server port (%d).");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				            pvar->ts->UIMsg, specs[i].from_port);
				break;
			case FWD_LOCAL_TO_REMOTE:
			case FWD_LOCAL_DYNAMIC:
				UTIL_get_lang_msg("MSG_SAME_LOCALPORT_ERROR", pvar,
				                  "You cannot have two forwarding from the same local port (%d).");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				            pvar->ts->UIMsg, specs[i].from_port);
				break;
			}
			notify_nonfatal_error(pvar, buf);

			free(specs);
			return FALSE;
		}

		if (!FWD_can_server_listen_for(pvar, specs + i)) {
			num_unspecified_forwardings++;
		}
	}

	if (num_unspecified_forwardings > 0) {
		UTIL_get_lang_msg("MSG_UNSPECIFIED_FWD_ERROR1", pvar,
		                  "The following forwarding was not specified when this SSH session began:\n\n");
		strncat_s(buf, sizeof(buf), pvar->ts->UIMsg, _TRUNCATE);

		for (i = 0; i < num_specs; i++) {
			if (!FWD_can_server_listen_for(pvar, specs + i)) {
				char buf2[1024];

				get_spec_string(specs + i, buf2, sizeof(buf2), pvar);

				strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
				strncat_s(buf, sizeof(buf), "\n", _TRUNCATE);
			}
		}

		UTIL_get_lang_msg("MSG_UNSPECIFIED_FWD_ERROR2", pvar,
		                  "\nDue to a limitation of the SSH protocol, this forwarding will not work in the current SSH session.\n"
		                  "If you save these settings and start a new SSH session, the forwarding should work.");
		strncat_s(buf, sizeof(buf), pvar->ts->UIMsg, _TRUNCATE);

		notify_nonfatal_error(pvar, buf);
	}

	FWD_set_request_specs(pvar, specs, num_specs);
	FWDUI_save_settings(pvar);
	free_all_listbox_specs(dlg);
	EndDialog(dlg, 1);

	free(specs);

	return TRUE;
}

static void fill_service_names(HWND dlg, WORD item)
{
	HWND cbox = GetDlgItem(dlg, item);
	int i;
	char *svc;

	for (i=0; (svc = service_name(i)) != NULL; i++) {
		SendMessage(cbox, CB_ADDSTRING, 0, (LPARAM) svc);
	}
}

static void init_input(HWND dlg, FWDType type_to)
{
	HWND dlg_item;

	if (type_to == FWD_REMOTE_TO_LOCAL || type_to == FWD_LOCAL_DYNAMIC) {
		dlg_item = GetDlgItem(dlg, IDC_SSHLTRFROMPORT);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHLTRLISTENADDR);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHLTRTOHOST);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHLTRTOPORT);
		EnableWindow(dlg_item, FALSE);
	}
	if (type_to == FWD_LOCAL_TO_REMOTE || type_to == FWD_LOCAL_DYNAMIC) {
		dlg_item = GetDlgItem(dlg, IDC_SSHRTLFROMPORT);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHRTLLISTENADDR);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHRTLTOHOST);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHRTLTOPORT);
		EnableWindow(dlg_item, FALSE);
	}
	if (type_to == FWD_REMOTE_TO_LOCAL || type_to == FWD_LOCAL_TO_REMOTE) {
		dlg_item = GetDlgItem(dlg, IDC_SSHDYNFROMPORT);
		EnableWindow(dlg_item, FALSE);
		dlg_item = GetDlgItem(dlg, IDC_SSHDYNLISTENADDR);
		EnableWindow(dlg_item, FALSE);
	}
}

static void shift_over_input(HWND dlg, FWDType type_from, FWDType type_to,
                             WORD rtl_item, WORD ltr_item, WORD dyn_item)
{
	HWND shift_from = NULL;
	HWND shift_to = NULL;

	if (type_from == FWD_LOCAL_TO_REMOTE ) {
		shift_from = GetDlgItem(dlg, ltr_item);
	}
	else if (type_from == FWD_REMOTE_TO_LOCAL) {
		shift_from = GetDlgItem(dlg, rtl_item);
	}
	else if (type_from == FWD_LOCAL_DYNAMIC && dyn_item != 0) {
		shift_from = GetDlgItem(dlg, dyn_item);
	}

	if (type_to == FWD_LOCAL_TO_REMOTE ) {
		shift_to = GetDlgItem(dlg, ltr_item);
	}
	else if (type_to == FWD_REMOTE_TO_LOCAL) {
		shift_to = GetDlgItem(dlg, rtl_item);
	}
	else if (type_to == FWD_LOCAL_DYNAMIC && dyn_item != 0) {
		shift_to = GetDlgItem(dlg, dyn_item);
	}

	if (shift_from != 0) {
		EnableWindow(shift_from, FALSE);
	}
	if (shift_to != 0) {
		EnableWindow(shift_to, TRUE);
	}

	if (shift_from != 0 && shift_to != 0 && GetWindowTextLength(shift_to) == 0) {
		char buf[128];

		GetWindowText(shift_from, buf, sizeof(buf));
		buf[sizeof(buf) - 1] = 0;
		SetWindowText(shift_to, buf);
		SetWindowText(shift_from, "");
	}
}

static void set_dir_options_status(HWND dlg)
{
	FWDType type_to, type_from;
	BOOL enable_LTR, enable_RTL, enable_DYN;
	
	type_to = FWD_REMOTE_TO_LOCAL;
	if (IsDlgButtonChecked(dlg, IDC_SSHFWDLOCALTOREMOTE)) {
		type_to = FWD_LOCAL_TO_REMOTE;
	}
	if (IsDlgButtonChecked(dlg, IDC_SSHFWDLOCALDYNAMIC)) {
		type_to = FWD_LOCAL_DYNAMIC;
	}

	enable_LTR = IsWindowEnabled(GetDlgItem(dlg, IDC_SSHLTRFROMPORT));
	enable_RTL = IsWindowEnabled(GetDlgItem(dlg, IDC_SSHRTLFROMPORT));
	enable_DYN = IsWindowEnabled(GetDlgItem(dlg, IDC_SSHDYNFROMPORT));
	type_from = FWD_NONE; // ダイアログ表示時にはすべてEnable状態
	if (enable_LTR && !enable_RTL && !enable_DYN) {
		type_from = FWD_LOCAL_TO_REMOTE;
	}
	else if (!enable_LTR && enable_RTL && !enable_DYN) {
		type_from = FWD_REMOTE_TO_LOCAL;
	}
	else if (!enable_LTR && !enable_RTL && enable_DYN) {
		type_from = FWD_LOCAL_DYNAMIC;
	}

	if (type_from == FWD_NONE) {
		init_input(dlg, type_to);
	}
	else {
		shift_over_input(dlg, type_from, type_to, IDC_SSHRTLFROMPORT, IDC_SSHLTRFROMPORT, IDC_SSHDYNFROMPORT);
		shift_over_input(dlg, type_from, type_to, IDC_SSHRTLLISTENADDR, IDC_SSHLTRLISTENADDR, IDC_SSHDYNLISTENADDR);
		shift_over_input(dlg, type_from, type_to, IDC_SSHRTLTOHOST, IDC_SSHLTRTOHOST, 0);
		shift_over_input(dlg, type_from, type_to, IDC_SSHRTLTOPORT, IDC_SSHLTRTOPORT, 0);
	}
}

static void setup_edit_controls(HWND dlg, FWDRequestSpec *spec,
                                WORD radio_item,
                                WORD from_port_item, WORD listen_address_item,
                                WORD to_host_item, WORD to_port_item)
{
	CheckDlgButton(dlg, radio_item, TRUE);
	SetFocus(GetDlgItem(dlg, radio_item));
	SetDlgItemText(dlg, from_port_item, spec->from_port_name);
	if (to_port_item != 0) {
		SetDlgItemText(dlg, to_port_item, spec->to_port_name);
	}
	if (to_host_item != 0 && strcmp(spec->to_host, "localhost") != 0) {
		SetDlgItemText(dlg, to_host_item, spec->to_host);
	}
	if (strcmp(spec->bind_address, "localhost") != 0) {
		SetDlgItemText(dlg, listen_address_item, spec->bind_address);
	}

	set_dir_options_status(dlg);
}

static void init_fwd_edit_dlg(PTInstVar pvar, FWDRequestSpec *spec, HWND dlg)
{
	const static DlgTextInfo text_info[] = {
		{ 0, "DLG_FWD_TITLE" },
		{ IDD_SSHFWDBANNER, "DLG_FWD_BANNER" },
		{ IDC_SSHFWDLOCALTOREMOTE, "DLG_FWD_LOCAL_PORT" },
		{ IDC_SSHFWDLOCALTOREMOTE_LISTEN, "DLG_FWD_LOCAL_LISTEN" },
		{ IDC_SSHFWDLOCALTOREMOTE_HOST, "DLG_FWD_LOCAL_REMOTE" },
		{ IDC_SSHFWDLOCALTOREMOTE_PORT, "DLG_FWD_LOCAL_REMOTE_PORT" },
		{ IDC_SSHFWDREMOTETOLOCAL, "DLG_FWD_REMOTE_PORT" },
		{ IDC_SSHFWDREMOTETOLOCAL_LISTEN, "DLG_FWD_REMOTE_LISTEN" },
		{ IDC_SSHFWDREMOTETOLOCAL_HOST, "DLG_FWD_REMOTE_LOCAL" },
		{ IDC_SSHFWDREMOTETOLOCAL_PORT, "DLG_FWD_REMOTE_LOCAL_PORT" },
		{ IDC_SSHFWDLOCALDYNAMIC, "DLG_FWD_DYNAMIC_PORT" },
		{ IDC_SSHFWDLOCALDYNAMIC_LISTEN, "DLG_FWD_DYNAMIC_LISTEN" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	SetI18DlgStrs("TTSSH", dlg, text_info, _countof(text_info), pvar->ts->UILanguageFile);

	switch (spec->type) {
	case FWD_REMOTE_TO_LOCAL:
		setup_edit_controls(dlg, spec, IDC_SSHFWDREMOTETOLOCAL,
		                    IDC_SSHRTLFROMPORT, IDC_SSHRTLLISTENADDR,
		                    IDC_SSHRTLTOHOST, IDC_SSHRTLTOPORT);
		break;
	case FWD_LOCAL_TO_REMOTE:
		setup_edit_controls(dlg, spec, IDC_SSHFWDLOCALTOREMOTE,
		                    IDC_SSHLTRFROMPORT, IDC_SSHLTRLISTENADDR,
		                    IDC_SSHLTRTOHOST, IDC_SSHLTRTOPORT);
		break;
	case FWD_LOCAL_DYNAMIC:
		setup_edit_controls(dlg, spec, IDC_SSHFWDLOCALDYNAMIC,
		                    IDC_SSHDYNFROMPORT, IDC_SSHDYNLISTENADDR,
		                    0, 0);
		break;
	}

	fill_service_names(dlg, IDC_SSHRTLFROMPORT);
	fill_service_names(dlg, IDC_SSHLTRFROMPORT);
	fill_service_names(dlg, IDC_SSHRTLTOPORT);
	fill_service_names(dlg, IDC_SSHLTRTOPORT);
}

static void grab_control_text(HWND dlg, FWDType type,
                              WORD rtl_item, WORD ltr_item, WORD dyn_item,
                              char *buf, int bufsize)
{
	WORD dlg_item = ltr_item;
	if (type == FWD_REMOTE_TO_LOCAL) {
		dlg_item = rtl_item;
	}
	else if (type == FWD_LOCAL_DYNAMIC) {
		dlg_item = dyn_item;
	}

	GetDlgItemText(dlg, dlg_item, buf, bufsize);
	buf[bufsize - 1] = 0;
}

static BOOL end_fwd_edit_dlg(PTInstVar pvar, FWDRequestSpec *spec, HWND dlg)
{
	FWDRequestSpec new_spec;
	FWDType type;
	char buf[1024];

	type = FWD_LOCAL_TO_REMOTE;
	if (IsDlgButtonChecked(dlg, IDC_SSHFWDREMOTETOLOCAL)) {
		type = FWD_REMOTE_TO_LOCAL;
	}
	else if (IsDlgButtonChecked(dlg, IDC_SSHFWDLOCALDYNAMIC)) {
		type = FWD_LOCAL_DYNAMIC;
	}
	new_spec.type = type;

	grab_control_text(dlg, type,
	                  IDC_SSHRTLFROMPORT, IDC_SSHLTRFROMPORT, IDC_SSHDYNFROMPORT,
	                  new_spec.from_port_name, sizeof(new_spec.from_port_name));

	grab_control_text(dlg, type,
	                  IDC_SSHRTLLISTENADDR, IDC_SSHLTRLISTENADDR, IDC_SSHDYNLISTENADDR,
	                  new_spec.bind_address, sizeof(new_spec.bind_address));
	if (new_spec.bind_address[0] == 0) {
		strncpy_s(new_spec.bind_address, sizeof(new_spec.bind_address), "localhost", _TRUNCATE);
	}
	else if (strcmp(new_spec.bind_address, "*") == 0 ) {
		strncpy_s(new_spec.bind_address, sizeof(new_spec.bind_address), "0.0.0.0", _TRUNCATE);
	}
	else {
		// IPv6 アドレスの "[", "]" があれば削除
		if (new_spec.bind_address[strlen(new_spec.bind_address)-1] == ']') {
			new_spec.bind_address[strlen(new_spec.bind_address)-1] = '\0';
		}
		if (new_spec.bind_address[0] == '[') {
			memmove(new_spec.bind_address, new_spec.bind_address + 1, strlen(new_spec.bind_address)+1);
		}
	}

	if (type == FWD_LOCAL_TO_REMOTE || type == FWD_REMOTE_TO_LOCAL) {
		grab_control_text(dlg, type,
		                  IDC_SSHRTLTOHOST, IDC_SSHLTRTOHOST, 0,
		                  new_spec.to_host, sizeof(new_spec.to_host));
		if (new_spec.to_host[0] == 0) {
			strncpy_s(new_spec.to_host, sizeof(new_spec.to_host), "localhost", _TRUNCATE);
		}
		else {
			// IPv6 アドレスの "[", "]" があれば削除
			if (new_spec.to_host[strlen(new_spec.to_host)-1] == ']') {
				new_spec.to_host[strlen(new_spec.to_host)-1] = '\0';
			}
			if (new_spec.to_host[0] == '[') {
				memmove(new_spec.to_host, new_spec.to_host + 1, strlen(new_spec.to_host)+1);
			}
		}

		grab_control_text(dlg, type,
		                  IDC_SSHRTLTOPORT, IDC_SSHLTRTOPORT, 0,
		                  new_spec.to_port_name, sizeof(new_spec.to_port_name));
	}

	new_spec.from_port = parse_port_from_buf(new_spec.from_port_name);
	if (new_spec.from_port < 0) {
		UTIL_get_lang_msg("MSG_INVALID_PORT_ERROR", pvar,
		                  "Port \"%s\" is not a valid port number.\n"
		                  "Either choose a port name from the list, or enter a number between 1 and 65535.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, new_spec.from_port_name);
		notify_nonfatal_error(pvar, buf);
		return FALSE;
	}

	if (type == FWD_LOCAL_TO_REMOTE || type == FWD_REMOTE_TO_LOCAL) {
		new_spec.to_port = parse_port_from_buf(new_spec.to_port_name);
		if (new_spec.to_port < 0) {
			UTIL_get_lang_msg("MSG_INVALID_PORT_ERROR", pvar,
			                  "Port \"%s\" is not a valid port number.\n"
			                  "Either choose a port name from the list, or enter a number between 1 and 65535.");
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			            pvar->ts->UIMsg, new_spec.to_port_name);
			notify_nonfatal_error(pvar, buf);
			return FALSE;
		}
	}

	*spec = new_spec;

	EndDialog(dlg, 1);
	return TRUE;
}

static UINT_PTR CALLBACK fwd_edit_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
	FWDEditClosure *closure;
	PTInstVar pvar;
	BOOL result;

	switch (msg) {
	case WM_INITDIALOG:
		closure = (FWDEditClosure *) lParam;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		pvar = closure->pvar;
		init_fwd_edit_dlg(pvar, closure->spec, dlg);
		CenterWindow(dlg, GetParent(dlg));
		return FALSE;			/* because we set the focus */

	case WM_COMMAND:
		closure = (FWDEditClosure *) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:

			result = end_fwd_edit_dlg(closure->pvar, closure->spec, dlg);

			if (result) {
			}

			return result;

		case IDCANCEL:
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_SSHFWDLOCALTOREMOTE:
		case IDC_SSHFWDREMOTETOLOCAL:
		case IDC_SSHFWDLOCALDYNAMIC:
			set_dir_options_status(dlg);
			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

static void add_forwarding_entry(PTInstVar pvar, HWND dlg)
{
	FWDRequestSpec new_spec;
	int result;
	FWDEditClosure closure = { &new_spec, pvar };

	new_spec.type = FWD_LOCAL_TO_REMOTE;
	new_spec.from_port_name[0] = 0;
	new_spec.bind_address[0] = 0;
	new_spec.to_host[0] = 0;
	new_spec.to_port_name[0] = 0;

	result = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHFWDEDIT),
	                        dlg, fwd_edit_dlg_proc, (LPARAM) & closure);

	if (result == -1) {
		UTIL_get_lang_msg("MSG_CREATEWINDOW_FWDEDIT_ERROR", pvar,
		                  "Unable to display forwarding edit dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	} else if (result) {
		int index = add_spec_to_listbox(dlg, &new_spec, pvar);

		if (index >= 0) {
			SendMessage(GetDlgItem(dlg, IDC_SSHFWDLIST), LB_SETCURSEL,
			            index, 0);
		}
		update_listbox_selection(dlg);
	}
}

static void edit_forwarding_entry(PTInstVar pvar, HWND dlg)
{
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int cursel = SendMessage(listbox, LB_GETCURSEL, 0, 0);

	if (cursel >= 0) {
		FWDRequestSpec *spec = (FWDRequestSpec *)
			SendMessage(listbox, LB_GETITEMDATA, cursel, 0);

		if (spec != NULL) {
			FWDEditClosure closure = { spec, pvar };

			int result =
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHFWDEDIT),
				               dlg, fwd_edit_dlg_proc, (LPARAM) & closure);

			if (result == -1) {
				UTIL_get_lang_msg("MSG_CREATEWINDOW_FWDEDIT_ERROR", pvar,
				                  "Unable to display forwarding edit dialog box.");
				notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			} else if (result) {
				SendMessage(listbox, LB_DELETESTRING, cursel, 0);

				cursel = add_spec_to_listbox(dlg, spec, pvar);
				free(spec);
				if (cursel >= 0) {
					SendMessage(GetDlgItem(dlg, IDC_SSHFWDLIST),
					            LB_SETCURSEL, cursel, 0);
				}
				update_listbox_selection(dlg);
			}
		}
	}
}

static void remove_forwarding_entry(HWND dlg)
{
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int cursel = SendMessage(listbox, LB_GETCURSEL, 0, 0);

	if (cursel >= 0) {
		free_listbox_spec(listbox, cursel);
		SendMessage(listbox, LB_DELETESTRING, cursel, 0);
		init_listbox_selection(dlg);
	}
}

static UINT_PTR CALLBACK fwd_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
	PTInstVar pvar;
	BOOL ret;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		SetWindowLongPtr(dlg, DWLP_USER, lParam);

		init_fwd_dlg(pvar, dlg);
		CenterWindow(dlg, GetParent(dlg));
		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLongPtr(dlg, DWLP_USER);

		switch (LOWORD(wParam)) {
		case IDOK:

			ret = end_fwd_dlg(pvar, dlg);
			return ret;

		case IDCANCEL:
			free_all_listbox_specs(dlg);
			EndDialog(dlg, 0);
			return TRUE;

		case IDC_SSHFWDSETUP_HELP:
			PostMessage(GetParent(dlg), WM_USER_DLGHELP2, HlpMenuSetupSshforward, 0);
			return TRUE;

		case IDC_ADD:
			add_forwarding_entry(pvar, dlg);
			return TRUE;

		case IDC_EDIT:
			edit_forwarding_entry(pvar, dlg);
			return TRUE;

		case IDC_REMOVE:
			remove_forwarding_entry(dlg);
			return TRUE;

		case IDC_SSHFWDLIST:
			update_listbox_selection(dlg);
			return TRUE;

		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

void FWDUI_do_forwarding_dialog(PTInstVar pvar)
{
	HWND cur_active = GetActiveWindow();

	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SSHFWDSETUP),
		               cur_active != NULL ? cur_active
		                                  : pvar->NotificationWindow,
		               fwd_dlg_proc, (LPARAM) pvar) == -1) {
		UTIL_get_lang_msg("MSG_CREATEWINDOW_FWDSETUP_ERROR", pvar,
		                  "Unable to display forwarding setup dialog box.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
	}
}
