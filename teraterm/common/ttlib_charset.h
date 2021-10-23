
#pragma once

#include "tttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int language;
	const char *str;
} TLanguageList;

typedef struct {
	int lang;
	int coding;
	const char *CodeName;
	const char *KanjiCode;
} TKanjiList;

const TLanguageList *GetLanguageList(int index);
const char *GetLanguageStr(int language);
int GetLanguageFromStr(const char *language_str);

const TKanjiList *GetKanjiList(int index);
const char *GetKanjiCodeStr(int language, int kanji_code);
int GetKanjiCodeFromStr(int language, const char *kanji_code_str);

#ifdef __cplusplus
}
#endif
