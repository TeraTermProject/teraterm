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

#include <windows.h>
#include <memory.h>

#include "matcher.h"

/* This is the "slow" wildcard matcher, but at least it is worst-case quadratic
   in the length of the string (as opposed to exponential).
   Since it's also best-case quadratic, it should only be used for unusual
   cases (2 or more *'s in the pattern). */
static BOOL wildcard_match_pattern(char const FAR * pattern, int patternLen,
                                   char const FAR * str, int strLen) {
	char * canBeAt = (char *)malloc(sizeof(char)*(strLen + 1));
	int i, result;

	canBeAt[0] = 1;
	memset(canBeAt + 1, 0, strLen);

	for (; patternLen > 0; patternLen--, pattern++) {
		char patternCh = *pattern;

		if (patternCh == '*') {
			char * firstPossible = (char *)memchr(canBeAt, 1, strLen + 1);

			if (firstPossible != NULL) {
				memset(firstPossible + 1, 1, canBeAt + strLen - firstPossible);
			}
		} else if (patternCh == '?') {
			memmove(canBeAt + 1, canBeAt, strLen);
			canBeAt[0] = 0;
		} else {
			for (i = strLen; i > 0; i--) {
				canBeAt[i] = canBeAt[i - 1] && str[i - 1] == patternCh;
		}
		canBeAt[0] = 0;
		}
	}

	result = canBeAt[strLen];

	free(canBeAt);

	return result;
}

/*
Before entry, ensure that str does not contain any invalid characters
(at least including '*', '?', ',', ' ' and '\n').
*/
BOOL match_pattern(char const FAR * pattern, char const FAR * str) {
	char patternCh = *pattern;
	char strCh = *str;

	while (strCh != 0 && (strCh == patternCh || patternCh == '?')) {
		str++;
		pattern++;
		strCh = *str;
		patternCh = *pattern;
	}

	if (strCh == 0) {
		while (patternCh == '*') {
			pattern++;
			patternCh = *pattern;
		}

		return !is_pattern_char(patternCh);
	} else if (patternCh == '*') {
		char nextPatternCh = pattern[1];

		if (!is_pattern_char(nextPatternCh)) {
			return TRUE;
		} else {
			int minStrLen = 0;
			int patternLen = 1;
			int strLen;

			patternCh = pattern[patternLen];
			while (is_pattern_char(patternCh)) {
			if (patternCh != '*') {
				minStrLen++;
			}
			patternLen++;
			patternCh = pattern[patternLen];
		}

			strLen = strlen(str);
			if (strLen < minStrLen) {
				return FALSE;
		} else {
				patternCh = pattern[patternLen - 1];
				strCh = str[strLen - 1];
				while (patternLen > 1 && (strCh == patternCh || patternCh == '?')) {
					strLen--;
					patternLen--;
					strCh = str[strLen - 1];
					patternCh = pattern[patternLen - 1];
				}

				if (patternLen == 1) { /* single * matches whatever's left in str */
					return TRUE;
				} else if (patternCh != '*') {
					return FALSE;
				} else {
					return wildcard_match_pattern(pattern, patternLen, str, strLen);
				}
			}
		}
	} else {
		return FALSE;
	}
}
