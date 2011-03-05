/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTCMN.DLL, character code conversion */

#ifdef __cplusplus
extern "C" {
#endif

/* proto types */
unsigned int FAR PASCAL SJIS2UTF8(WORD KCode, int *byte, char *locale);
WORD FAR PASCAL SJIS2JIS(WORD KCode);
WORD FAR PASCAL SJIS2EUC(WORD KCode);
WORD FAR PASCAL JIS2SJIS(WORD KCode);
BYTE FAR PASCAL RussConv(int cin, int cout, BYTE b);
void FAR PASCAL RussConvStr
  (int cin, int cout, PCHAR Str, int count);

#ifdef __cplusplus
}
#endif
