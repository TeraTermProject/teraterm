/*
 * (C) 2023- TeraTerm Project
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

#include "ttcstd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CharSetDataTag CharSetData;

typedef struct {
	int infos[6];
} CharSetState;

// 文字の出力
typedef struct CharSetOpTag {
	// 出力される Unicode 文字
	void (*PutU32)(char32_t code, void *client_data);
	// 出力される Control 文字
	void (*ParseControl)(BYTE b, void *client_data);
} CharSetOp;

/* Character sets */
typedef enum {
	IdASCII,
	IdKatakana,
	IdKanji,
	IdSpecial,
} CharSetCS;

// input
void ParseFirst(CharSetData *w, BYTE b);

// control
typedef enum {
	CHARSET_LS0,	// Locking Shift 0, SI, 0F (G0->GL)
	CHARSET_LS1,	// Locking Shift 1, SO, 0E (G1->GL)
	CHARSET_LS2,	// Locking Shift 2, ESC n, 1B 6E (G2->GL)
	CHARSET_LS3,	// Locking Shift 3, ESC o, 1B 6F(G3->GL)
	CHARSET_LS1R,	// Locking Shift 1R, ESC ~, 1B 7E (G1->GR)
	CHARSET_LS2R,	// Locking Shift 2R, ESC }, 1B 7D (G2->GR)
	CHARSET_LS3R,	// Locking Shift 3R, ESC |, 1B 7C (G3->GR)
	CHARSET_SS2,	// Single Shift 2, SS2, 8E, ESC N, 1B 4E
	CHARSET_SS3,	// Single Shift 3, SS3, 8F, ESC O, 1B 4F
} CharSet2022Shift;

CharSetData *CharSetInit(const CharSetOp *op, void *client_data);
void CharSetFinish(CharSetData *w);
void CharSet2022Designate(CharSetData *w, int gn, CharSetCS cs);
void CharSet2022Invoke(CharSetData *w, CharSet2022Shift shift);
BOOL CharSetIsSpecial(CharSetData *w, BYTE b);
void CharSetSaveState(CharSetData *w, CharSetState *state);
void CharSetLoadState(CharSetData *w, const CharSetState *state);
void CharSetFallbackFinish(CharSetData *w);

// debug mode
#define DEBUG_FLAG_NONE  0
#define DEBUG_FLAG_NORM  1
#define DEBUG_FLAG_HEXD  2
#define DEBUG_FLAG_NOUT  3
#define DEBUG_FLAG_MAXD  4
void CharSetSetNextDebugMode(CharSetData *w);
//BYTE CharSetGetDebugMode(CharSetData *w);
void CharSetSetDebugMode(CharSetData *w, BYTE mode);

#ifdef __cplusplus
}
#endif
