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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "servicenames.h"

#define NUM_ELEM(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
	int port;
	char *name;
} TCP_service_name;

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
	{443, "https"},
	{7489, "iasqlsvr"},
	{113, "ident"},
	{143, "imap"},
	{143, "imap2"},
	{220, "imap3"},
	{143, "imap4"},
	{993, "imaps"},
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
	{995, "pop3s"},
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
	{3389, "rdp"},
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
	{5900, "rfb"},
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
	{4190, "sieve"},
	{9, "sink"},
	{775, "sms_db"},
	{777, "sms_update"},
	{25, "smtp"},
	{465, "smtps"},
	{199, "smux"},
	{108, "snagas"},
	{19, "source"},
	{6111, "spc"},
	{515, "spooler"},
	{22, "ssh"},
	{23523, "sshprop"},
	{1391, "statnav"},
	{1621, "statnavdbg"},
	{1395, "stm_switch"},
	{587, "submission"},
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
	{992, "telnets"},
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
	{5900, "vnc"},
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

static int compare_services(void const *elem1, void const *elem2)
{
	TCP_service_name *s1 = (TCP_service_name *) elem1;
	TCP_service_name *s2 = (TCP_service_name *) elem2;

	return strcmp(s1->name, s2->name);
}

static BOOL is_service_name_char(char ch)
{
	return (isalnum(ch) || ch == '_' || ch == '-');
}

int PASCAL parse_port_from_buf(char * buf)
{
	int i;
	char lower_buf[32];
	TCP_service_name *found;
	TCP_service_name key;

	for (i = 0; buf[i] != 0 && i < sizeof(lower_buf) - 1; i++) {
		lower_buf[i] = tolower(buf[i]);
	}
	lower_buf[i] = 0;

	key.name = lower_buf;
	found = (TCP_service_name *)
		bsearch(&key, service_DB, NUM_ELEM(service_DB),
				sizeof(service_DB[0]), compare_services);

	if (found) {
		return found->port;
	}
	else if (isdigit(buf[0])) {
		int result = atoi(buf);
		if (result > 0 && result < 65536) {
			return result;
		}
	}

	return -1;
}

int PASCAL parse_port(char *str, char *buf, int bufsize)
{
	int i = 0;

	while (is_service_name_char(*str) && i < bufsize - 1) {
		buf[i] = *str;
		str++;
		i++;
	}
	buf[i] = 0;

	return parse_port_from_buf(buf);
}

char * PASCAL service_name(int num) {
	if (num < 0 || num >= NUM_ELEM(service_DB))
		return NULL;
	return (service_DB[num].name);
}
