/*
 * (C) 2025- TeraTerm Project
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tttypes_termid.h"

const static TermIDList TermIDLists[] = {
	{IdVT100, "VT100"},
	{IdVT100J, "VT100J"},
	{IdVT101, "VT101"},
	{IdVT102, "VT102"},
	{IdVT102J, "VT102J"},
	{IdVT220, "VT220"},
	{IdVT220J, "VT220J"},
	{IdVT282, "VT282"},
	{IdVT320, "VT320"},
	{IdVT382, "VT382"},
	{IdVT420, "VT420"},
	{IdVT520, "VT520"},
	{IdVT525, "VT525"},
};

const TermIDList *TermIDGetList(int index)
{
	if (index < 0 || index >= (int)_countof(TermIDLists)) {
		return NULL;
	}
	return &TermIDLists[index];
}

int TermIDGetID(const char *term_id_str)
{
	if (term_id_str == NULL) {
		return IdVT100;
	}
	for (size_t i = 0; i < _countof(TermIDLists); i++) {
		if (strcmp(term_id_str, TermIDLists[i].TermIDStr) == 0) {
			return TermIDLists[i].TermID;
		}
	}
	return IdVT100;
}

const char *TermIDGetStr(int term_id)
{
	for (size_t i = 0; i < _countof(TermIDLists); i++) {
		if (term_id == TermIDLists[i].TermID) {
			return TermIDLists[i].TermIDStr;
		}
	}
	assert(0);
	return NULL;
}

int TermIDGetVTLevel(int term_id)
{
	int VTlevel;

	switch (term_id) {
	case IdVT100:
	case IdVT100J:
	case IdVT101:
		VTlevel = 1;
		break;
	case IdVT102:
	case IdVT102J:
	case IdVT220:
	case IdVT220J:
		VTlevel = 2;
		break;
	case IdVT282:
	case IdVT320:
	case IdVT382:
		VTlevel = 3;
		break;
	case IdVT420:
		VTlevel = 4;
		break;
	case IdVT520:
	case IdVT525:
		VTlevel = 5;
		break;
	default:
		assert(0);
		VTlevel = 1;
		break;
	}
	return VTlevel;
}
