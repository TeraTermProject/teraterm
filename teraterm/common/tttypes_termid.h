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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

  /* Terminal ID */
typedef enum {
	IdVT100 = 1,
	IdVT100J,
	IdVT101,
	IdVT102,
	IdVT102J,
	IdVT220J,
	IdVT282,
	IdVT320,
	IdVT382,
	IdVT420,
	IdVT520,
	IdVT525
} IdTerminalID;

typedef struct {
	int TermID;				// Terminal ID
	const char *TermIDStr;	// Terminal ID ‚Ì•¶Žš—ñ
} TermIDList;

/**
 * @brief TermIDList ‚ðŽæ“¾
 * @param index 0...
 * @return TermIDList ‚Ìƒ|ƒCƒ“ƒ^(NULL‚ÌŽž–³Œø)
 */
const TermIDList *TermIDGetList(int index);

/**
 * @brief •¶Žš‚©‚çID‚Ö•ÏŠ·
 * @param term_id_str 
 * @return IdTerminalID (•¶Žš—ñ‚©‚çŒ©‚Â‚¯‚ç‚ê‚È‚©‚Á‚½‚Æ‚«‚Í idVT100)
 */
int TermIDGetID(const char *term_id_str);

/**
 * @brief ID‚©‚ç•¶Žš—ñ‚Ö•ÏŠ·
 * @param term_id 
 * @return •¶Žš—ñ
 */
const char *TermIDGetStr(int term_id);

#ifdef __cplusplus
}
#endif
