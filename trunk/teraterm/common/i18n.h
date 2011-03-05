/* Tera Term
 Copyright(C) 2006 TeraTerm Project
 All rights reserved. */

#ifndef __I18N_H
#define __I18N_H

#include <windows.h>
#include "ttlib.h"

#define MAX_UIMSG	1024

#ifdef __cplusplus
extern "C" {
#endif

void FAR PASCAL GetI18nStr(PCHAR section, PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile);
int FAR PASCAL GetI18nLogfont(PCHAR section, PCHAR key, PLOGFONT logfont, int ppi, PCHAR iniFile);

#ifdef __cplusplus
}
#endif

#endif
