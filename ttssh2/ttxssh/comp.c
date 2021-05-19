/*
 * (C) 2021- TeraTerm Project
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

#include "ttxssh.h"
#include "comp.h"
#include "kex.h"


struct ssh2_comp_t {
	compression_type type;
	char *name;
};

static const struct ssh2_comp_t ssh2_comps[] = {
	{COMP_NOCOMP,  "none"},             // RFC4253
	{COMP_ZLIB,    "zlib"},             // RFC4253
	{COMP_DELAYED, "zlib@openssh.com"},
	{COMP_NONE,    NULL},
};


char* get_ssh2_comp_name(compression_type type)
{
	const struct ssh2_comp_t *ptr = ssh2_comps;

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "unknown";
}

void normalize_comp_order(char *buf)
{
	static char default_strings[] = {
		COMP_DELAYED,
		COMP_ZLIB,
		COMP_NOCOMP,
		COMP_NONE,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

compression_type choose_SSH2_compression_algorithm(char *server_proposal, char *my_proposal)
{
	compression_type type = COMP_UNKNOWN;
	char str_comp[20];
	const struct ssh2_comp_t *ptr = ssh2_comps;

	choose_SSH2_proposal(server_proposal, my_proposal, str_comp, sizeof(str_comp));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_comp) == 0) {
			type = ptr->type;
			break;
		}
		ptr++;
	}

	return (type);
}

void SSH2_update_compression_myproposal(PTInstVar pvar)
{
	static char buf[128]; // TODO: malloc()‚É‚·‚×‚«
	int index;
	int len, i;

	// ’ÊM’†‚É‚ÍŒÄ‚Î‚ê‚È‚¢‚Í‚¸‚¾‚ªA”O‚Ì‚½‚ßB(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	// ˆ³kƒŒƒxƒ‹‚É‰ž‚¶‚ÄAmyproposal[]‚ð‘‚«Š·‚¦‚éB(2005.7.9 yutaka)
	buf[0] = '\0';
	for (i = 0 ; pvar->settings.CompOrder[i] != 0 ; i++) {
		index = pvar->settings.CompOrder[i] - '0';
		if (index == COMP_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh2_comp_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma

	// ˆ³kŽw’è‚ª‚È‚¢ê‡‚ÍAˆ³kƒŒƒxƒ‹‚ð–³ðŒ‚Éƒ[ƒ‚É‚·‚éB
	if (buf[0] == '\0') {
		pvar->settings.CompressionLevel = 0;
	}

	if (pvar->settings.CompressionLevel == 0) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, get_ssh2_comp_name(COMP_NOCOMP));
	}
	if (buf[0] != '\0') {
		myproposal[PROPOSAL_COMP_ALGS_CTOS] = buf;  // Client To Server
		myproposal[PROPOSAL_COMP_ALGS_STOC] = buf;  // Server To Client
	}
}
