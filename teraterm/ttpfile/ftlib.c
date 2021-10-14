/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* TTFILE.DLL, routines for file transfer protocol */

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include <stdio.h>
#include <string.h>

#include "ftlib.h"
#include "tt_res.h"

WORD UpdateCRC(BYTE b, WORD CRC)
{
  int i;

  CRC = CRC ^ (WORD)((WORD)b << 8);
  for (i = 1 ; i <= 8 ; i++)
    if ((CRC & 0x8000)!=0)
      CRC = (CRC << 1) ^ 0x1021;
    else
      CRC = CRC << 1;
  return CRC;
}

LONG UpdateCRC32(BYTE b, LONG CRC)
{
  int i;

  CRC = CRC ^ (LONG)b;
  for (i = 1 ; i <= 8 ; i++)
    if ((CRC & 0x00000001)!=0)
      CRC = (CRC >> 1) ^ 0xedb88320;
    else
      CRC = CRC >> 1;
  return CRC;
}
