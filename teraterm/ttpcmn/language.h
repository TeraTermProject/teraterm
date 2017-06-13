/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTCMN.DLL, character code conversion */

#ifdef __cplusplus
extern "C" {
#endif

/* proto types */
unsigned int PASCAL SJIS2UTF8(WORD KCode, int *byte, char *locale);
WORD PASCAL SJIS2JIS(WORD KCode);
WORD PASCAL SJIS2EUC(WORD KCode);
WORD PASCAL JIS2SJIS(WORD KCode);
BYTE PASCAL RussConv(int cin, int cout, BYTE b);
void PASCAL RussConvStr
  (int cin, int cout, PCHAR Str, int count);

#ifdef __cplusplus
}
#endif
