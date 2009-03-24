/* Tera Term
 Copyright(C) 2006 TeraTerm Project
 All rights reserved. */

#include "i18n.h"

void FAR PASCAL GetI18nStr(PCHAR section, PCHAR key, PCHAR buf, int buf_len, PCHAR def, PCHAR iniFile)
{
	GetPrivateProfileString(section, key, def, buf, buf_len, iniFile);
	RestoreNewLine(buf);
}

int FAR PASCAL GetI18nLogfont(PCHAR section, PCHAR key, PLOGFONT logfont, int ppi, PCHAR iniFile)
{
	static char tmp[MAX_UIMSG];
	static char font[LF_FACESIZE];
	int hight, charset;
	GetPrivateProfileString(section, key, "-", tmp, MAX_UIMSG, iniFile);
	if (strcmp(tmp, "-") == 0) {
		return FALSE;
	}

	GetNthString(tmp, 1, LF_FACESIZE-1, font);
	GetNthNum(tmp, 2, &hight);
	GetNthNum(tmp, 3, &charset);

	strncpy_s(logfont->lfFaceName, sizeof(logfont->lfFaceName), font, _TRUNCATE);
	logfont->lfCharSet = charset;
	logfont->lfHeight = MulDiv(hight, -ppi, 72);
	logfont->lfWidth = 0;

	return TRUE;
}