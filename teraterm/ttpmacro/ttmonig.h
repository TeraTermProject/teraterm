#pragma once

// Oniguruma: Regular expression library
#define ONIG_EXTERN extern
#include "oniguruma.h"
#undef ONIG_EXTERN

#ifdef __cplusplus
extern "C" {
#endif

extern OnigOptionType RegexOpt;
extern OnigEncoding RegexEnc;
extern OnigSyntaxType *RegexSyntax;

#ifdef __cplusplus
}
#endif
