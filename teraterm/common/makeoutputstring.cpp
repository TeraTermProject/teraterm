/*
 * (C) 2024- TeraTerm Project
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

// ���������R�[�h(wchar_t) -> �o�͕����R�[�h�֕ϊ�����

#include <assert.h>

#include "tttypes.h"
#include "tttypes_charset.h"
#include "codeconv.h"
#include "../teraterm/unicode.h"
#include "codeconv_mb.h"

#include "makeoutputstring.h"

typedef struct OutputCharStateTag {
	WORD Language;		// �o�͕����R�[�h
	WORD KanjiCode;		// �o�͕����R�[�h(sjis,jis�Ȃ�)

	// JIS ����IN/OUT/�J�i
	WORD KanjiIn;		// IdKanjiInA / IdKanjiInB
	WORD KanjiOut;		// IdKanjiOutB / IdKanjiOutJ / IdKanjiOutH
	BOOL JIS7Katakana;	// (Kanji JIS)kana

	// state
	int SendCode;		// [in,out](Kanji JIS)���O�̑��M�R�[�h Ascii/Kana/Kanji
} OutputCharState;

/**
 *	@retval	true	���{��̔��p�J�^�J�i
 *	@retval	false	���̑�
 */
static BOOL IsHalfWidthKatakana(unsigned int u32)
{
	// Halfwidth CJK punctuation (U+FF61�`FF64)
	// Halfwidth Katakana variants (U+FF65�`FF9F)
	return (0xff61 <= u32 && u32 <= 0xff9f);
}

OutputCharState *MakeOutputStringCreate(void)
{
	OutputCharState *p = (OutputCharState *)calloc(1, sizeof(*p));
	return p;
}

void MakeOutputStringDestroy(OutputCharState *state)
{
	assert(state != NULL);

	memset(state, 0, sizeof(*state));
	free(state);
}

/**
 * �o�͐ݒ�
 *
 *	@param	states
 *	@param	Language	IdLanguage
 *	@param	kanji_code	IdSJIS / IdEUC �Ȃ�
 *	@param	KanjiIn		IdKanjiInA / IdKanjiInB
 *	@param	KanjiOut	IdKanjiOutB / IdKanjiOutJ / IdKanjiOutH
 *	@param	jis7katakana
 */
void MakeOutputStringInit(
	OutputCharState *state,
	WORD Language,
	WORD kanji_code,
	WORD KanjiIn,
	WORD KanjiOut,
	BOOL jis7katakana)
{
	assert(state != NULL);

	state->Language = Language;
	state->KanjiCode = kanji_code;
	state->KanjiIn = KanjiIn;
	state->KanjiOut = KanjiOut;
	state->JIS7Katakana = jis7katakana;
	//
	state->SendCode = IdASCII;
//	state->ControlOut = OutControl;
}

/**
 * �o�͗p��������쐬����
 *
 *	Unicode(UTF-16)�����񂩂�Unicode(UTF-32)��1�������o����
 *	�o�͕���(TempStr)���쐬����
 *
 *	@param	states
 *	@param	B			���͕�����(wchar_t)
 *	@param	C			���͕�����
 *	@param	TempStr		�o�͕���ptr
 *	@param	TempLen_	�o�͕�����
 *	@param	ControlOut	��������/�o�͊֐�
 *	@param	cv
 *	@retval	���͕����񂩂�g�p����������
 */
size_t MakeOutputString(
	OutputCharState *states,
	const wchar_t *B, size_t C,
	char *TempStr, size_t *TempLen_,
	BOOL (*ControlOut)(unsigned int u32, BOOL check_only, char *TempStr, size_t *StrLen, void *data),
	void *data)
{
	size_t TempLen = 0;
	size_t TempLen2;
	size_t output_char_count;	// �����������

	assert(states != NULL);

	// UTF-32 ��1�������o��
	unsigned int u32;
	size_t u16_len = UTF16ToUTF32(B, C, &u32);
	if (u16_len == 0) {
		// �f�R�[�h�ł��Ȃ�? ���蓾�Ȃ��̂ł�?
		assert(FALSE);
		u32 = '?';
		u16_len = 1;
	}
	output_char_count = u16_len;

	// �e��V�t�g��Ԃ�ʏ�ɖ߂�
	if (u32 < 0x100 || (ControlOut != NULL && ControlOut(u32, TRUE, NULL, NULL, data))) {
		if (states->Language == IdJapanese && states->KanjiCode == IdJIS) {
			// ���̂Ƃ���A���{��,JIS�����Ȃ�
			if (states->SendCode == IdKanji) {
				// �����ł͂Ȃ��̂ŁA����OUT
				TempStr[TempLen++] = 0x1B;
				TempStr[TempLen++] = '(';
				switch (states->KanjiOut) {
				case IdKanjiOutJ:
					TempStr[TempLen++] = 'J';
					break;
				case IdKanjiOutH:
					TempStr[TempLen++] = 'H';
					break;
				default:
					TempStr[TempLen++] = 'B';
				}
			}

			if (states->JIS7Katakana == 1) {
				if (states->SendCode == IdKatakana) {
					TempStr[TempLen++] = SO;
				}
			}

			states->SendCode = IdASCII;
		}
	}

	// 1������������
	if (ControlOut != NULL && ControlOut(u32, FALSE, &TempStr[TempLen], &TempLen2, data)) {
		// ���ʂȕ�������������
		TempLen += TempLen2;
		output_char_count = 1;
	}
	else if ((states->Language == IdUtf8) ||
			 (states->Language == IdJapanese && states->KanjiCode == IdUTF8) ||
			 (states->Language == IdKorean && states->KanjiCode == IdUTF8) ||
			 (states->Language == IdChinese && states->KanjiCode == IdUTF8))
	{
		// UTF-8 �ŏo��
		size_t utf8_len = sizeof(TempStr);
		utf8_len = UTF32ToUTF8(u32, TempStr, utf8_len);
		TempLen += utf8_len;
	} else if (states->Language == IdJapanese) {
		// ���{��
		// �܂� CP932(SJIS) �ɕϊ����Ă���o��
		char mb_char[2];
		size_t mb_len = sizeof(mb_char);
		mb_len = UTF32ToMBCP(u32, 932, mb_char, mb_len);
		if (mb_len == 0) {
			// SJIS�ɕϊ��ł��Ȃ�
			TempStr[TempLen++] = '?';
		} else {
			switch (states->KanjiCode) {
			case IdEUC:
				// TODO ���p�J�i
				if (mb_len == 1) {
					TempStr[TempLen++] = mb_char[0];
				} else {
					WORD K;
					K = (((WORD)(unsigned char)mb_char[0]) << 8) +
						(WORD)(unsigned char)mb_char[1];
					K = CodeConvSJIS2EUC(K);
					TempStr[TempLen++] = HIBYTE(K);
					TempStr[TempLen++] = LOBYTE(K);
				}
				break;
			case IdJIS:
				if (u32 < 0x100) {
					// ASCII
					TempStr[TempLen++] = mb_char[0];
					states->SendCode = IdASCII;
				} else if (IsHalfWidthKatakana(u32)) {
					// ���p�J�^�J�i
					if (states->JIS7Katakana==1) {
						if (states->SendCode != IdKatakana) {
							TempStr[TempLen++] = SI;
						}
						TempStr[TempLen++] = mb_char[0] & 0x7f;
					} else {
						TempStr[TempLen++] = mb_char[0];
					}
					states->SendCode = IdKatakana;
				} else {
					// ����
					WORD K;
					K = (((WORD)(unsigned char)mb_char[0]) << 8) +
						(WORD)(unsigned char)mb_char[1];
					K = CodeConvSJIS2JIS(K);
					if (states->SendCode != IdKanji) {
						// ����IN
						TempStr[TempLen++] = 0x1B;
						TempStr[TempLen++] = '$';
						if (states->KanjiIn == IdKanjiInB) {
							TempStr[TempLen++] = 'B';
						}
						else {
							TempStr[TempLen++] = '@';
						}
						states->SendCode = IdKanji;
					}
					TempStr[TempLen++] = HIBYTE(K);
					TempStr[TempLen++] = LOBYTE(K);
				}
				break;
			case IdSJIS:
				if (mb_len == 1) {
					TempStr[TempLen++] = mb_char[0];
				} else {
					TempStr[TempLen++] = mb_char[0];
					TempStr[TempLen++] = mb_char[1];
				}
				break;
			default:
				assert(FALSE);
				break;
			}
		}
	} else if (states->Language == IdRussian) {
		/* �܂�CP1251�ɕϊ����ďo�� */
		char mb_char[2];
		size_t mb_len = sizeof(mb_char);
		BYTE b;
		mb_len = UTF32ToMBCP(u32, 1251, mb_char, mb_len);
		if (mb_len != 1) {
			b = '?';
		} else {
			b = CodeConvRussConv(IdWindows, states->KanjiCode, mb_char[0]);
		}
		TempStr[TempLen++] = b;
	}
	else if (states->Language == IdKorean || states->Language == IdChinese) {
		int code_page;
		char mb_char[2];
		size_t mb_len;
		if (states->Language == IdKorean) {
			code_page = 949;
		}
		else if (states->Language == IdChinese) {
			switch (states->KanjiCode) {
			case IdCnGB2312:
				code_page = 936;
				break;
			case IdCnBig5:
				code_page = 950;
				break;
			default:
				assert(FALSE);
				code_page = 936;
				break;
			}
		} else {
			assert(FALSE);
			code_page = 0;
		}
		/* code_page �ɕϊ����ďo�� */
		mb_len = sizeof(mb_char);
		mb_len = UTF32ToMBCP(u32, code_page, mb_char, mb_len);
		if (mb_len == 0) {
			TempStr[TempLen++] = '?';
		}
		else if (mb_len == 1) {
			TempStr[TempLen++] = mb_char[0];
		} else  {
			TempStr[TempLen++] = mb_char[0];
			TempStr[TempLen++] = mb_char[1];
		}
	}
	else if (states->Language == IdEnglish) {
		unsigned char byte;
		int part = KanjiCodeToISO8859Part(states->KanjiCode);
		int r = UnicodeToISO8859(part, u32, &byte);
		if (r == 0) {
			// �ϊ��ł��Ȃ������R�[�h������
			byte = '?';
		}
		TempStr[TempLen++] = byte;
	} else {
		assert(FALSE);
	}

	*TempLen_ = TempLen;
	return output_char_count;
}
