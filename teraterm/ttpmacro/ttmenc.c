/* Tera Term
   Copyright(C) 1994-1998 T. Teranishi
   All rights reserved. */

/* TTMACRO.EXE, password encryption */

#include "teraterm.h"
#include <stdlib.h>
#include <string.h>

BOOL EncSeparate(PCHAR Str, int far *i, LPBYTE b)
{
	int cptr, bptr;
	unsigned int d;

	cptr = *i / 8;
	if (Str[cptr]==0) {
		return FALSE;
	}
	bptr = *i % 8;
	d = ((BYTE)Str[cptr] << 8) |
	     (BYTE)Str[cptr+1];
	*b = (BYTE)((d >> (10-bptr)) & 0x3f);

	*i = *i + 6;
	return TRUE;
}

BYTE EncCharacterize(BYTE c, LPBYTE b)
{
	BYTE d;

	d = c + *b;
	if (d > (BYTE)0x7e) {
		d = d - (BYTE)0x5e;
	}
	if (*b<0x30) {
		*b = 0x30;
	}
	else if (*b<0x40) {
		*b = 0x40;
	}
	else if (*b<0x50) {
		*b = 0x50;
	}
	else if (*b<0x60) {
		*b = 0x60;
	}
	else if (*b<0x70) {
		*b = 0x70;
	}
	else {
		*b = 0x21;
	}

	return d;
}

void Encrypt(PCHAR InStr, PCHAR OutStr)
{
	int i, j;
	BYTE b, r, r2;

	OutStr[0] = 0;
	if (InStr[0]==0) {
		return;
	}
	srand(LOWORD(GetTickCount()));
	r = (BYTE)(rand() & 0x3f);
	r2 = (~r) & 0x3f;
	OutStr[0] = r;
	i = 0;
	j = 1;
	while (EncSeparate(InStr,&i,&b)) {
		r = (BYTE)(rand() & 0x3f);
		OutStr[j++] = (b + r) & 0x3f;
		OutStr[j++] = r;
	}
	OutStr[j++] = r2;
	OutStr[j] = 0;
	i = 0;
	b = 0x21;
	while (i < j) {
		OutStr[i] = EncCharacterize(OutStr[i],&b);
		i++;
	}
}

void DecCombine(PCHAR Str, int *i, BYTE b)
{
	int cptr, bptr;
	unsigned int d;

	cptr = *i / 8;
	bptr = *i % 8;
	if (bptr==0) {
		Str[cptr] = 0;
	}

	d = ((BYTE)Str[cptr] << 8) |
	    (b << (10 - bptr));

	Str[cptr] = (BYTE)(d >> 8);
	Str[cptr+1] = (BYTE)(d & 0xff);
	*i = *i + 6;
	return;
}

BYTE DecCharacter(BYTE c, LPBYTE b)
{
	BYTE d;

	if (c < *b) {
		d = (BYTE)0x5e + c - *b;
	}
	else {
		d = c - *b;
	}
	d = d & 0x3f;

	if (*b<0x30) {
		*b = 0x30;
	}
	else if (*b<0x40) {
		*b = 0x40;
	}
	else if (*b<0x50) {
		*b = 0x50;
	}
	else if (*b<0x60) {
		*b = 0x60;
	}
	else if (*b<0x70) {
		*b = 0x70;
	}
	else {
		*b = 0x21;
	}

	return d;
}

void Decrypt(PCHAR InStr, PCHAR OutStr)
{
	int i, j, k;
	BYTE b;
	char Temp[512];

	OutStr[0] = 0;
	j = strlen(InStr);
	if (j==0) {
		return;
	}
	b = 0x21;
	for (i=0 ; i < j ; i++) {
		Temp[i] = DecCharacter(InStr[i],&b);
	}
	if ((Temp[0] ^ Temp[j-1]) != (BYTE)0x3f) {
		return;
	}
	i = 1;
	k = 0;
	while (i < j-2) {
		Temp[i] = ((BYTE)Temp[i] - (BYTE)Temp[i+1]) & (BYTE)0x3f;
		DecCombine(OutStr,&k,Temp[i]);
		i = i + 2;
	}
}
