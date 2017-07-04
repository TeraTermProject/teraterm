/*
 * (C) 2004-2017 TeraTerm Project
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

#include "ed25519_crypto_api.h"

#define blocks crypto_hashblocks_sha512

static const unsigned char iv[64] = {
  0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
  0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
  0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
  0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
  0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
  0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
  0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
  0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79
} ;

typedef unsigned long long uint64;

int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen)
{
  unsigned char h[64];
  unsigned char padded[256];
//  unsigned int i;
  unsigned long long i;
  unsigned long long bytes = inlen;

  for (i = 0;i < 64;++i) h[i] = iv[i];

  blocks(h,in,inlen);
  in += inlen;
  inlen &= 127;
  in -= inlen;

  for (i = 0;i < inlen;++i) padded[i] = in[i];
  padded[inlen] = 0x80;

  if (inlen < 112) {
    for (i = inlen + 1;i < 119;++i) padded[i] = 0;
    padded[119] = (unsigned char)(bytes >> 61);
    padded[120] = (unsigned char)(bytes >> 53);
    padded[121] = (unsigned char)(bytes >> 45);
    padded[122] = (unsigned char)(bytes >> 37);
    padded[123] = (unsigned char)(bytes >> 29);
    padded[124] = (unsigned char)(bytes >> 21);
    padded[125] = (unsigned char)(bytes >> 13);
    padded[126] = (unsigned char)(bytes >> 5);
    padded[127] = (unsigned char)(bytes << 3);
    blocks(h,padded,128);
  } else {
    for (i = inlen + 1;i < 247;++i) padded[i] = 0;
    padded[247] = (unsigned char)(bytes >> 61);
    padded[248] = (unsigned char)(bytes >> 53);
    padded[249] = (unsigned char)(bytes >> 45);
    padded[250] = (unsigned char)(bytes >> 37);
    padded[251] = (unsigned char)(bytes >> 29);
    padded[252] = (unsigned char)(bytes >> 21);
    padded[253] = (unsigned char)(bytes >> 13);
    padded[254] = (unsigned char)(bytes >> 5);
    padded[255] = (unsigned char)(bytes << 3);
    blocks(h,padded,256);
  }

  for (i = 0;i < 64;++i) out[i] = h[i];

  return 0;
}

