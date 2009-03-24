/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTTEK.DLL, TEK escape sequences */

void FAR PASCAL TEKChangeCaret(PTEKVar tk, PTTSet ts);
void ParseFirst(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b);
void TEKEscape(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b);
void SelectCode(PTEKVar tk, PTTSet ts, BYTE b);
void TwoOpCode(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b);
void ControlSequence(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b);
void GraphText(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b);
