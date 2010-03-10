/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "fwdui.h"

#include "resource.h"
#include "x11util.h"
#include "util.h"

static HFONT DlgFwdEditFont;
static HFONT DlgFwdFont;

typedef struct {
	int port;
	char FAR *name;
} TCP_service_name;

typedef struct {
	FWDRequestSpec FAR *spec;
	PTInstVar pvar;
} FWDEditClosure;

static TCP_service_name service_DB[] = {
	{384, "arns"},
	{204, "at-echo"},
	{202, "at-nbp"},
	{201, "at-rtmp"},
	{206, "at-zis"},
	{705, "auditd"},
	{113, "auth"},
	{113, "authentication"},
	{152, "bftp"},
	{179, "bgp"},
	{67, "bootp"},
	{68, "bootpc"},
	{67, "bootps"},
	{881, "bshell"},
	{1133, "bshelldbg"},
	{877, "buildd"},
	{2003, "cfinger"},
	{19, "chargen"},
	{531, "chat"},
	{2030, "client"},
	{514, "cmd"},
	{164, "cmip-agent"},
	{163, "cmip-man"},
	{1431, "coda_aux1"},
	{1433, "coda_aux2"},
	{1435, "coda_aux3"},
	{1407, "coda_backup"},
	{1423, "codacon"},
	{1285, "codger"},
	{1541, "codgerdbg"},
	{531, "conference"},
	{1389, "controller"},
	{530, "courier"},
	{1100, "cpsys"},
	{1346, "csd"},
	{1602, "csddbg"},
	{105, "csnet-ns"},
	{105, "cso-ns"},
	{1999, "cvskserver"},
	{987, "daserver"},
	{13, "daytime"},
	{1429, "dbp"},
	{10300, "dictionary"},
	{9, "discard"},
	{2100, "discuss"},
	{1399, "disptool0"},
	{1401, "disptool1"},
	{1403, "disptool2"},
	{1405, "disptool3"},
	{53, "domain"},
	{7000, "dos"},
	{7, "echo"},
	{8100, "eda1_mbx"},
	{8101, "eda2_mbx"},
	{8000, "eda_mbx"},
	{520, "efs"},
	{2105, "eklogin"},
	{545, "ekshell"},
	{1377, "erim"},
	{1617, "erimdbg"},
	{888, "erlogin"},
	{512, "exec"},
	{2001, "filesrv"},
	{79, "finger"},
	{21, "ftp"},
	{20, "ftp-data"},
	{1397, "gds_db"},
	{70, "gopher"},
	{1025, "greendbg"},
	{5999, "grmd"},
	{5710, "hcserver"},
	{751, "hesupd"},
	{101, "hostname"},
	{101, "hostnames"},
	{80, "http"},
	{7489, "iasqlsvr"},
	{113, "ident"},
	{143, "imap"},
	{143, "imap2"},
	{220, "imap3"},
	{1524, "ingreslock"},
	{1234, "instsrv"},
	{1387, "ipt"},
	{213, "ipx"},
	{194, "irc"},
	{6667, "irc-alt"},
	{883, "ishell"},
	{102, "iso-tsap"},
	{885, "itkt"},
	{1439, "jeeves"},
	{1389, "joysticknav"},
	{1629, "joysticknavdbg"},
	{544, "kcmd"},
	{750, "kdc"},
	{10401, "kdebug"},
	{750, "kerberos"},
	{749, "kerberos-adm"},
	{88, "kerberos-sec"},
	{751, "kerberos_master"},
	{1445, "kjdbc"},
	{543, "klogin"},
	{561, "kopexec"},
	{562, "kopshell"},
	{761, "kpasswd"},
	{1109, "kpop"},
	{1110, "kpopr"},
	{761, "kpwd"},
	{88, "krb5"},
	{4444, "krb524"},
	{754, "krb5_prop"},
	{754, "krb_prop"},
	{760, "krbupdate"},
	{545, "krcmd"},
	{1443, "krcp"},
	{760, "kreg"},
	{544, "kshell"},
	{750, "ktc"},
	{549, "kxct"},
	{5696, "lanmgrx.osb"},
	{1441, "lcladm"},
	{87, "link"},
	{98, "linuxconf"},
	{2766, "listen"},
	{1025, "listener"},
	{135, "loc-srv"},
	{4045, "lockd"},
	{513, "login"},
	{25, "mail"},
	{9535, "man"},
	{3881, "mbatchd"},
	{1393, "motionnav"},
	{1633, "motionnavdbg"},
	{7000, "msdos"},
	{18, "msp"},
	{57, "mtp"},
	{42, "name"},
	{42, "nameserver"},
	{773, "nanny"},
	{138, "nbdgm"},
	{137, "nbns"},
	{139, "nbssn"},
	{1419, "ndim"},
	{138, "netbios-dgm"},
	{137, "netbios-ns"},
	{139, "netbios-ssn"},
	{138, "netbios_dgm"},
	{137, "netbios_ns"},
	{139, "netbios_ssn"},
	{2106, "netdist"},
	{1287, "netimage"},
	{1543, "netimagedbg"},
	{532, "netnews"},
	{1353, "netreg"},
	{1609, "netregdbg"},
	{77, "netrjs"},
	{15, "netstat"},
	{1026, "network_terminal"},
	{526, "newdate"},
	{144, "news"},
	{178, "nextstep"},
	{1536, "nft"},
	{43, "nicname"},
	{119, "nntp"},
	{1026, "nterm"},
	{123, "ntp"},
	{123, "ntpd"},
	{9, "null"},
	{891, "odexm"},
	{560, "opcmd"},
	{560, "opshell"},
	{879, "pag"},
	{893, "papyrus"},
	{1379, "parvis"},
	{1619, "parvisdbg"},
	{600, "pcserver"},
	{2026, "pcserverbulk"},
	{2025, "pcserverrpc"},
	{1385, "pharos"},
	{1373, "pierunt"},
	{1613, "pieruntdbg"},
	{1351, "piesrv"},
	{1607, "piesrvdbg"},
	{1889, "pmlockd"},
	{109, "pop"},
	{109, "pop-2"},
	{110, "pop-3"},
	{109, "pop2"},
	{110, "pop3"},
	{111, "portmap"},
	{111, "portmapper"},
	{109, "postoffice"},
	{170, "print-srv"},
	{515, "printer"},
	{191, "prospero"},
	{1525, "prospero-np"},
	{17, "qotd"},
	{17, "quote"},
	{601, "rauth"},
	{5347, "rcisimmux"},
	{1530, "rdeliver"},
	{532, "readnews"},
	{7815, "recserv"},
	{64, "rem"},
	{1025, "remote_file_sharing"},
	{1026, "remote_login"},
	{556, "remotefs"},
	{3878, "res"},
	{875, "resolve"},
	{1131, "resolvedbg"},
	{4672, "rfa"},
	{5002, "rfe"},
	{1025, "rfs"},
	{556, "rfs_server"},
	{1027, "rfsdbg"},
	{77, "rje"},
	{1260, "rlb"},
	{1425, "rndb2"},
	{530, "rpc"},
	{111, "rpcbind"},
	{1347, "rpl"},
	{1603, "rpldbg"},
	{107, "rtelnet"},
	{3882, "sbatchd"},
	{778, "serv"},
	{1375, "service_warp"},
	{1615, "service_warpdbg"},
	{115, "sftp"},
	{5232, "sgi-dgl"},
	{514, "shell"},
	{1135, "shelob"},
	{1137, "shelobsrv"},
	{9, "sink"},
	{775, "sms_db"},
	{777, "sms_update"},
	{25, "smtp"},
	{199, "smux"},
	{108, "snagas"},
	{19, "source"},
	{6111, "spc"},
	{515, "spooler"},
	{23523, "sshprop"},
	{1391, "statnav"},
	{1621, "statnavdbg"},
	{1395, "stm_switch"},
	{1283, "sunmatrox"},
	{1539, "sunmatroxdbg"},
	{111, "sunrpc"},
	{95, "supdup"},
	{1127, "supfiledbg"},
	{871, "supfilesrv"},
	{1125, "supnamedbg"},
	{869, "supnamesrv"},
	{1529, "support"},
	{11, "systat"},
	{601, "ta-rauth"},
	{113, "tap"},
	{1381, "task_control"},
	{1, "tcpmux"},
	{23, "telnet"},
	{24, "telnet2"},
	{526, "tempo"},
	{17, "text"},
	{37, "time"},
	{37, "timserver"},
	{1600, "tnet"},
	{102, "tsap"},
	{87, "ttylink"},
	{19, "ttytst"},
	{372, "ulistserv"},
	{119, "untp"},
	{119, "usenet"},
	{11, "users"},
	{1400, "usim"},
	{540, "uucp"},
	{117, "uucp-path"},
	{540, "uucpd"},
	{1387, "vapor"},
	{712, "vexec"},
	{712, "vice-exec"},
	{713, "vice-login"},
	{714, "vice-shell"},
	{1371, "visim"},
	{1611, "visimdbg"},
	{713, "vlogin"},
	{714, "vshell"},
	{210, "wais"},
	{1427, "warplite"},
	{765, "webster"},
	{43, "whois"},
	{1421, "wiztemp"},
	{2000, "wm"},
	{1383, "wnn"},
	{22273, "wnn4"},
	{22289, "wnn4_cn"},
	{22273, "wnn4_jp"},
	{1348, "worldc"},
	{1604, "worldcdbg"},
	{2401, "writesrv"},
	{80, "www"},
	{6000, "x-server"},
	{103, "x400"},
	{104, "x400-snd"},
	{177, "xdmcp"},
	{873, "xtermd"},
	{210, "z3950"}
};

static int compare_services(void const FAR * elem1, void const FAR * elem2)
{
	TCP_service_name FAR *s1 = (TCP_service_name FAR *) elem1;
	TCP_service_name FAR *s2 = (TCP_service_name FAR *) elem2;

	return strcmp(s1->name, s2->name);
}

static void make_X_forwarding_spec(FWDRequestSpec FAR * spec, PTInstVar pvar)
{
	spec->type = FWD_REMOTE_X11_TO_LOCAL;
	spec->from_port = -1;
	X11_get_DISPLAY_info(spec->to_host, sizeof(spec->to_host),
	                     &spec->to_port);
	UTIL_get_lang_msg("MSG_FWD_REMOTE_XSERVER", pvar, "remote X server");
	strncpy_s(spec->from_port_name, sizeof(spec->from_port_name),
	          pvar->ts->UIMsg, _TRUNCATE);
	UTIL_get_lang_msg("MSG_FWD_REMOTE_XSCREEN", pvar, "X server (screen %d)");
	_snprintf_s(spec->to_port_name, sizeof(spec->to_port_name), _TRUNCATE,
	            pvar->ts->UIMsg, spec->to_port - 6000);
}

static BOOL is_service_name_char(char ch)
{
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')
	    || ch == '_' || ch == '-' || (ch >= '0' && ch <= '9');
}

static int parse_port_from_buf(char FAR * buf)
{
	int i;

	if (buf[0] >= '0' && buf[0] <= '9') {
		int result = atoi(buf);

		if (result < 1 || result > 65535) {
			return -1;
		} else {
			return result;
		}
	} else {
		char lower_buf[32];
		TCP_service_name FAR *result;
		TCP_service_name key;

		for (i = 0; buf[i] != 0 && i < sizeof(lower_buf) - 1; i++) {
			lower_buf[i] = tolower(buf[i]);
		}
		lower_buf[i] = 0;

		key.name = lower_buf;
		result = (TCP_service_name FAR *)
			bsearch(&key, service_DB, NUM_ELEM(service_DB),
			        sizeof(service_DB[0]), compare_services);

		if (result == NULL) {
			return -1;
		} else {
			return result->port;
		}
	}
}

static int parse_port(char FAR * FAR * str, char FAR * buf, int bufsize)
{
	int i = 0;

	while (is_service_name_char(**str) && i < bufsize - 1) {
		buf[i] = **str;
		(*str)++;
		i++;
	}
	buf[i] = 0;

	return parse_port_from_buf(buf);
}

static BOOL parse_request(FWDRequestSpec FAR * request, char FAR * str, PTInstVar pvar)
{
	char FAR *host_start;

	if (str[0] == 'L' || str[0] == 'l') {
		request->type = FWD_LOCAL_TO_REMOTE;
	} else if (str[0] == 'R' || str[0] == 'r') {
		request->type = FWD_REMOTE_TO_LOCAL;
	} else if (str[0] == 'X' || str[0] == 'x') {
		make_X_forwarding_spec(request, pvar);
		return TRUE;
	} else {
		return FALSE;
	}
	str++;

	request->from_port =
		parse_port(&str, request->from_port_name,
		           sizeof(request->from_port_name));
	if (request->from_port < 0) {
		return FALSE;
	}

	if (*str != ':') {
		return FALSE;
	}
	str++;

	host_start = str;
	while (*str != ':' && *str != 0 && *str != ';') {
		str++;
	}
	if (*str != ':') {
		return FALSE;
	}
	*str = 0;
	strncpy_s(request->to_host, sizeof(request->to_host), host_start, _TRUNCATE);
	request->to_host[sizeof(request->to_host) - 1] = 0;
	*str = ':';
	str++;

	request->to_port =
		parse_port(&str, request->to_port_name,
		           sizeof(request->to_port_name));
	if (request->to_port < 0) {
		return FALSE;
	}

	if (*str == ':') {
		str++;
		request->check_identity = TRUE;
		if (*str == '1') {
			request->check_identity = FALSE;
			str++;
		}
	}

	if (*str != ';' && *str != 0) {
		return FALSE;
	}

	return TRUE;
}

static void FWDUI_save_settings(PTInstVar pvar)
{
	int num_specs = FWD_get_num_request_specs(pvar);
	FWDRequestSpec FAR *requests =
		(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) * num_specs);
	int i;
	char FAR *str = pvar->settings.DefaultForwarding;
	int str_remaining = sizeof(pvar->settings.DefaultForwarding) - 1;

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
			FWDRequestSpec FAR *spec = requests + i;
			int chars;

			switch (spec->type) {
			case FWD_LOCAL_TO_REMOTE:
				if (spec->check_identity == 0) {
					_snprintf_s(str, str_remaining, _TRUNCATE, "L%s:%s:%s:1",
					            spec->from_port_name, spec->to_host,
					            spec->to_port_name);
				}
				else {
					_snprintf_s(str, str_remaining, _TRUNCATE, "L%s:%s:%s",
					            spec->from_port_name, spec->to_host,
					            spec->to_port_name);
				}
				break;
			case FWD_REMOTE_TO_LOCAL:
				_snprintf_s(str, str_remaining, _TRUNCATE, "R%s:%s:%s",
				            spec->from_port_name, spec->to_host,
				            spec->to_port_name);
				break;
			case FWD_REMOTE_X11_TO_LOCAL:
				_snprintf_s(str, str_remaining, _TRUNCATE, "X");
				break;
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
	char FAR *str = pvar->settings.DefaultForwarding;

	if (str[0] != 0) {
		int i, ch, j;
		FWDRequestSpec FAR *requests;

		j = 1;
		for (i = 0; (ch = str[i]) != 0; i++) {
			if (ch == ';') {
				j++;
			}
		}

		requests =
			(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) * j);

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
}

void FWDUI_end(PTInstVar pvar)
{
}

void FWDUI_open(PTInstVar pvar)
{
}

static void set_verbose_port(char FAR * buf, int bufsize, int port,
                             char FAR * name)
{
	if (*name >= '0' && *name <= '9') {
		strncpy_s(buf, bufsize, name, _TRUNCATE);
	} else {
		_snprintf_s(buf, bufsize, _TRUNCATE, "%d (%s)", port, name);
	}

	buf[bufsize - 1] = 0;
}

static void get_spec_string(FWDRequestSpec FAR * spec, char FAR * buf,
                            int bufsize, PTInstVar pvar)
{
	char verbose_from_port[64];
	char verbose_to_port[64];

	set_verbose_port(verbose_from_port, sizeof(verbose_from_port),
	                 spec->from_port, spec->from_port_name);
	set_verbose_port(verbose_to_port, sizeof(verbose_to_port),
	                 spec->to_port, spec->to_port_name);

	switch (spec->type) {
	case FWD_REMOTE_TO_LOCAL:
		UTIL_get_lang_msg("MSG_FWD_REMOTE", pvar,
		                  "Remote %s to local \"%s\" port %s");
		_snprintf_s(buf, bufsize, _TRUNCATE, pvar->ts->UIMsg,
		            verbose_from_port, spec->to_host, verbose_to_port);
		break;
	case FWD_LOCAL_TO_REMOTE:
		UTIL_get_lang_msg("MSG_FWD_LOCAL", pvar,
		                  "Local %s to remote \"%s\" port %s");
		_snprintf_s(buf, bufsize, _TRUNCATE, pvar->ts->UIMsg,
		            verbose_from_port, spec->to_host, verbose_to_port);
		break;
	case FWD_REMOTE_X11_TO_LOCAL:
		UTIL_get_lang_msg("MSG_FWD_X", pvar,
		                  "Remote X applications to local X server");
		strncpy_s(buf, bufsize, pvar->ts->UIMsg, _TRUNCATE);
		return;
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

static int add_spec_to_listbox(HWND dlg, FWDRequestSpec FAR * spec, PTInstVar pvar)
{
	char buf[1024];
	HWND listbox = GetDlgItem(dlg, IDC_SSHFWDLIST);
	int index;

	get_spec_string(spec, buf, sizeof(buf), pvar);

	index = SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM) buf);

	if (index >= 0) {
		FWDRequestSpec FAR *listbox_spec = malloc(sizeof(FWDRequestSpec));

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
	FWDRequestSpec FAR *requests =
		(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) * num_specs);
	int i;
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_PORTFORWARD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_LIST", pvar, uimsg);
	SetDlgItemText(dlg, IDC_PORTFORWARD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_ADD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_ADD", pvar, uimsg);
	SetDlgItemText(dlg, IDC_ADD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_EDIT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_EDIT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_EDIT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_REMOVE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_REMOVE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_REMOVE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_CHECKIDENTITY, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_CHECKIDENTITY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_CHECKIDENTITY, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_XFORWARD, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLD_FWDSETUP_X", pvar, uimsg);
	SetDlgItemText(dlg, IDC_XFORWARD, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDX11, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWDSETUP_XAPP", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDX11, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_CANCEL", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

	FWD_get_request_specs(pvar, requests, num_specs);

	for (i = 0; i < num_specs; i++) {
		if (requests[i].type == FWD_REMOTE_X11_TO_LOCAL) {
			CheckDlgButton(dlg, IDC_SSHFWDX11, TRUE);
		} else {
			add_spec_to_listbox(dlg, requests + i, pvar);
		}
	}

	if (!pvar->settings.LocalForwardingIdentityCheck) {
		CheckDlgButton(dlg, IDC_CHECKIDENTITY, TRUE);
	}

	free(requests);

	init_listbox_selection(dlg);
}

static void free_listbox_spec(HWND listbox, int selection)
{
	FWDRequestSpec FAR *spec = (FWDRequestSpec FAR *)
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
	FWDRequestSpec FAR *specs =
		(FWDRequestSpec FAR *) malloc(sizeof(FWDRequestSpec) *
		                              (num_specs + num_items));
	int i;
	int num_unspecified_forwardings = 0;

	for (i = 0; i < num_items; i++) {
		FWDRequestSpec FAR *spec = (FWDRequestSpec FAR *)
			SendMessage(listbox, LB_GETITEMDATA, i, 0);

		if (spec != NULL) {
			specs[num_specs] = *spec;
			num_specs++;
		}
	}

	if (X_enabled) {
		make_X_forwarding_spec(specs, pvar);
	}

	if (IsDlgButtonChecked(dlg, IDC_CHECKIDENTITY)) {
		pvar->settings.LocalForwardingIdentityCheck = FALSE;
	}
	else {
		pvar->settings.LocalForwardingIdentityCheck = TRUE;
	}

	qsort(specs, num_specs, sizeof(FWDRequestSpec), FWD_compare_specs);

	buf[0] = '\0';
	for (i = 0; i < num_specs; i++) {
		if (i < num_specs - 1
			&& FWD_compare_specs(specs + i, specs + i + 1) == 0) {
			switch (specs[i].type) {
			case FWD_REMOTE_TO_LOCAL:
				UTIL_get_lang_msg("MSG_SAME_SERVERPORT_ERROR", pvar,
				                  "You cannot have two forwardings from the same server port (%d).");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				            pvar->ts->UIMsg, specs[i].from_port);
				break;
			case FWD_LOCAL_TO_REMOTE:
				UTIL_get_lang_msg("MSG_SAME_LOCALPORT_ERROR", pvar,
				                  "You cannot have two forwardings from the same local port (%d).");
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				            pvar->ts->UIMsg, specs[i].from_port);
				break;
			}
			notify_nonfatal_error(pvar, buf);

			free(specs);
			return FALSE;
		}

		if (specs[i].type != FWD_LOCAL_TO_REMOTE
			&& !FWD_can_server_listen_for(pvar, specs + i)) {
			num_unspecified_forwardings++;
		}
	}

	if (num_unspecified_forwardings > 0) {
		UTIL_get_lang_msg("MSG_UNSPECIFYIED_FWD_ERROR1", pvar,
		                  "The following forwarding(s) was not specified when this SSH session began:\n\n");
		strncat_s(buf, sizeof(buf), pvar->ts->UIMsg, _TRUNCATE);

		for (i = 0; i < num_specs; i++) {
			if (specs[i].type != FWD_LOCAL_TO_REMOTE &&
			    !FWD_can_server_listen_for(pvar, specs + i)) {
				char buf2[1024];

				get_spec_string(specs + i, buf2, sizeof(buf2), pvar);

				strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
				strncat_s(buf, sizeof(buf), "\n", _TRUNCATE);
			}
		}

		UTIL_get_lang_msg("MSG_UNSPECIFYIED_FWD_ERROR2", pvar,
		                  "\nDue to a limitation of the SSH protocol, this forwarding(s) will not work in the current SSH session.\n"
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

	for (i = 0; i < NUM_ELEM(service_DB); i++) {
		SendMessage(cbox, CB_ADDSTRING, 0, (LPARAM) service_DB[i].name);
	}
}

static void shift_over_input(HWND dlg, int type, WORD rtl_item,
							 WORD ltr_item)
{
	HWND shift_from;
	HWND shift_to;

	if (type == FWD_REMOTE_TO_LOCAL) {
		shift_from = GetDlgItem(dlg, ltr_item);
		shift_to = GetDlgItem(dlg, rtl_item);
	} else {
		shift_from = GetDlgItem(dlg, rtl_item);
		shift_to = GetDlgItem(dlg, ltr_item);
	}

	EnableWindow(shift_from, FALSE);
	EnableWindow(shift_to, TRUE);

	if (GetWindowTextLength(shift_to) == 0) {
		char buf[128];

		GetWindowText(shift_from, buf, sizeof(buf));
		buf[sizeof(buf) - 1] = 0;
		SetWindowText(shift_to, buf);
		SetWindowText(shift_from, "");
	}
}

static void set_dir_options_status(HWND dlg)
{
	int type = IsDlgButtonChecked(dlg, IDC_SSHFWDREMOTETOLOCAL)
		? FWD_REMOTE_TO_LOCAL : FWD_LOCAL_TO_REMOTE;

	shift_over_input(dlg, type, IDC_SSHRTLFROMPORT, IDC_SSHLTRFROMPORT);
	shift_over_input(dlg, type, IDC_SSHRTLTOHOST, IDC_SSHLTRTOHOST);
	shift_over_input(dlg, type, IDC_SSHRTLTOPORT, IDC_SSHLTRTOPORT);

	if (IsDlgButtonChecked(GetParent(dlg),IDC_CHECKIDENTITY)) {
		if (type == FWD_LOCAL_TO_REMOTE) {
			EnableWindow(GetDlgItem(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY), TRUE);
		}
		else {
			EnableWindow(GetDlgItem(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY), FALSE);
		}
	}
}

static void setup_edit_controls(HWND dlg, FWDRequestSpec FAR * spec,
                                WORD radio_item, WORD from_port_item,
                                WORD to_host_item, WORD to_port_item)
{
	CheckDlgButton(dlg, radio_item, TRUE);
	SetFocus(GetDlgItem(dlg, radio_item));
	SetDlgItemText(dlg, from_port_item, spec->from_port_name);
	SetDlgItemText(dlg, to_port_item, spec->to_port_name);
	if (strcmp(spec->to_host, "localhost") != 0) {
		SetDlgItemText(dlg, to_host_item, spec->to_host);
	}

	set_dir_options_status(dlg);
}

static void init_fwd_edit_dlg(PTInstVar pvar, FWDRequestSpec FAR * spec, HWND dlg)
{
	char uimsg[MAX_UIMSG];

	GetWindowText(dlg, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_TITLE", pvar, uimsg);
	SetWindowText(dlg, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDD_SSHFWDBANNER, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_BANNER", pvar, uimsg);
	SetDlgItemText(dlg, IDD_SSHFWDBANNER, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_LOCAL_PORT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_HOST, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_LOCAL_REMOTE", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_HOST, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_PORT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_LOCAL_REMOTE_PORT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_PORT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_LOCAL_CHECKIDENTITY", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_REMOTE_PORT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL_HOST, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_REMOTE_LOCAL", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL_HOST, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL_PORT, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("DLG_FWD_REMOTE_LOCAL_PORT", pvar, uimsg);
	SetDlgItemText(dlg, IDC_SSHFWDREMOTETOLOCAL_PORT, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDOK, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_OK", pvar, uimsg);
	SetDlgItemText(dlg, IDOK, pvar->ts->UIMsg);
	GetDlgItemText(dlg, IDCANCEL, uimsg, sizeof(uimsg));
	UTIL_get_lang_msg("BTN_CANCEL", pvar, uimsg);
	SetDlgItemText(dlg, IDCANCEL, pvar->ts->UIMsg);

	switch (spec->type) {
	case FWD_REMOTE_TO_LOCAL:
		setup_edit_controls(dlg, spec, IDC_SSHFWDREMOTETOLOCAL,
		                    IDC_SSHRTLFROMPORT, IDC_SSHRTLTOHOST,
		                    IDC_SSHRTLTOPORT);
		break;
	case FWD_LOCAL_TO_REMOTE:
		setup_edit_controls(dlg, spec, IDC_SSHFWDLOCALTOREMOTE,
		                    IDC_SSHLTRFROMPORT, IDC_SSHLTRTOHOST,
		                    IDC_SSHLTRTOPORT);
		if (!spec->check_identity) {
			CheckDlgButton(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY, TRUE);
		}
		break;
	}

	if (!IsDlgButtonChecked(GetParent(dlg),IDC_CHECKIDENTITY)) {
		CheckDlgButton(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY, FALSE);
		EnableWindow(GetDlgItem(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY), FALSE);
	}

	fill_service_names(dlg, IDC_SSHRTLFROMPORT);
	fill_service_names(dlg, IDC_SSHLTRFROMPORT);
	fill_service_names(dlg, IDC_SSHRTLTOPORT);
	fill_service_names(dlg, IDC_SSHLTRTOPORT);
}

static void grab_control_text(HWND dlg, int type, WORD rtl_item,
                              WORD ltr_item, char FAR * buf, int bufsize)
{
	GetDlgItemText(dlg, type == FWD_REMOTE_TO_LOCAL ? rtl_item : ltr_item,
	               buf, bufsize);
	buf[bufsize - 1] = 0;
}

static BOOL end_fwd_edit_dlg(PTInstVar pvar, FWDRequestSpec FAR * spec,
                             HWND dlg)
{
	FWDRequestSpec new_spec;
	int type = IsDlgButtonChecked(dlg, IDC_SSHFWDREMOTETOLOCAL)
		? FWD_REMOTE_TO_LOCAL : FWD_LOCAL_TO_REMOTE;
	char buf[1024];

	new_spec.type = type;
	grab_control_text(dlg, type, IDC_SSHRTLFROMPORT, IDC_SSHLTRFROMPORT,
	                  new_spec.from_port_name,
	                  sizeof(new_spec.from_port_name));
	grab_control_text(dlg, type, IDC_SSHRTLTOHOST, IDC_SSHLTRTOHOST,
	                  new_spec.to_host, sizeof(new_spec.to_host));
	if (new_spec.to_host[0] == 0) {
		strncpy_s(new_spec.to_host, sizeof(new_spec.to_host), "localhost", _TRUNCATE);
	}
	grab_control_text(dlg, type, IDC_SSHRTLTOPORT, IDC_SSHLTRTOPORT,
	                  new_spec.to_port_name,
	                  sizeof(new_spec.to_port_name));

	new_spec.from_port = parse_port_from_buf(new_spec.from_port_name);
	if (new_spec.from_port < 0) {
		UTIL_get_lang_msg("MSG_INVALID_PORT_ERROR", pvar,
		                  "Port \"%s\" is not a valid port number.\n"
		                  "Either choose a port name from the list, or enter a number between 1 and 65535.");
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		            pvar->ts->UIMsg, new_spec.to_port_name);
		notify_nonfatal_error(pvar, buf);
		return FALSE;
	}

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

	new_spec.check_identity = TRUE;
	if (type == FWD_LOCAL_TO_REMOTE) {
		if (IsDlgButtonChecked(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY)) {
			new_spec.check_identity = FALSE;
		}
	}

	*spec = new_spec;

	EndDialog(dlg, 1);
	return TRUE;
}

static BOOL CALLBACK fwd_edit_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
	FWDEditClosure FAR *closure;
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		closure = (FWDEditClosure FAR *) lParam;
		SetWindowLong(dlg, DWL_USER, lParam);

		pvar = closure->pvar;
		init_fwd_edit_dlg(pvar, closure->spec, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgFwdEditFont, pvar)) {
			SendDlgItemMessage(dlg, IDD_SSHFWDBANNER, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDLOCALTOREMOTE, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDLOCALTOREMOTE_HOST, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDLOCALTOREMOTE_PORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDLOCALTOREMOTE_CHECKIDENTITY, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDREMOTETOLOCAL, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDREMOTETOLOCAL_HOST, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDREMOTETOLOCAL_PORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHLTRFROMPORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHLTRTOHOST, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHLTRTOPORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHRTLFROMPORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHRTLTOHOST, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHRTLTOPORT, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgFwdEditFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgFwdEditFont = NULL;
		}
		return FALSE;			/* because we set the focus */

	case WM_COMMAND:
		closure = (FWDEditClosure FAR *) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDOK:

			if (DlgFwdEditFont != NULL) {
				DeleteObject(DlgFwdEditFont);
			}

			return end_fwd_edit_dlg(closure->pvar, closure->spec, dlg);

		case IDCANCEL:
			EndDialog(dlg, 0);

			if (DlgFwdEditFont != NULL) {
				DeleteObject(DlgFwdEditFont);
			}

			return TRUE;

		case IDC_SSHFWDLOCALTOREMOTE:
		case IDC_SSHFWDREMOTETOLOCAL:
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
	new_spec.to_host[0] = 0;
	new_spec.to_port_name[0] = 0;
	new_spec.check_identity = 1;

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
		FWDRequestSpec FAR *spec = (FWDRequestSpec FAR *)
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

static BOOL CALLBACK fwd_dlg_proc(HWND dlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
	PTInstVar pvar;
	LOGFONT logfont;
	HFONT font;

	switch (msg) {
	case WM_INITDIALOG:
		pvar = (PTInstVar) lParam;
		SetWindowLong(dlg, DWL_USER, lParam);

		init_fwd_dlg(pvar, dlg);

		font = (HFONT)SendMessage(dlg, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (UTIL_get_lang_font("DLG_TAHOMA_FONT", dlg, &logfont, &DlgFwdFont, pvar)) {
			SendDlgItemMessage(dlg, IDC_PORTFORWARD, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDLIST, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_ADD, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_EDIT, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_REMOVE, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_CHECKIDENTITY, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_XFORWARD, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDC_SSHFWDX11, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDOK, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(dlg, IDCANCEL, WM_SETFONT, (WPARAM)DlgFwdFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgFwdFont = NULL;
		}

		return TRUE;			/* because we do not set the focus */

	case WM_COMMAND:
		pvar = (PTInstVar) GetWindowLong(dlg, DWL_USER);

		switch (LOWORD(wParam)) {
		case IDOK:

			if (DlgFwdFont != NULL) {
				DeleteObject(DlgFwdFont);
			}

			return end_fwd_dlg(pvar, dlg);

		case IDCANCEL:
			free_all_listbox_specs(dlg);
			EndDialog(dlg, 0);

			if (DlgFwdFont != NULL) {
				DeleteObject(DlgFwdFont);
			}

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
