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

#ifndef __ED25519_SC25519_H
#define __ED25519_SC25519_H

#include "ed25519_crypto_api.h"

#define sc25519                  crypto_sign_ed25519_ref_sc25519
#define shortsc25519             crypto_sign_ed25519_ref_shortsc25519
#define sc25519_from32bytes      crypto_sign_ed25519_ref_sc25519_from32bytes
#define shortsc25519_from16bytes crypto_sign_ed25519_ref_shortsc25519_from16bytes
#define sc25519_from64bytes      crypto_sign_ed25519_ref_sc25519_from64bytes
#define sc25519_from_shortsc     crypto_sign_ed25519_ref_sc25519_from_shortsc
#define sc25519_to32bytes        crypto_sign_ed25519_ref_sc25519_to32bytes
#define sc25519_iszero_vartime   crypto_sign_ed25519_ref_sc25519_iszero_vartime
#define sc25519_isshort_vartime  crypto_sign_ed25519_ref_sc25519_isshort_vartime
#define sc25519_lt_vartime       crypto_sign_ed25519_ref_sc25519_lt_vartime
#define sc25519_add              crypto_sign_ed25519_ref_sc25519_add
#define sc25519_sub_nored        crypto_sign_ed25519_ref_sc25519_sub_nored
#define sc25519_mul              crypto_sign_ed25519_ref_sc25519_mul
#define sc25519_mul_shortsc      crypto_sign_ed25519_ref_sc25519_mul_shortsc
#define sc25519_window3          crypto_sign_ed25519_ref_sc25519_window3
#define sc25519_window5          crypto_sign_ed25519_ref_sc25519_window5
#define sc25519_2interleave2     crypto_sign_ed25519_ref_sc25519_2interleave2

typedef struct 
{
  crypto_uint32 v[32]; 
}
sc25519;

typedef struct 
{
  crypto_uint32 v[16]; 
}
shortsc25519;

void sc25519_from32bytes(sc25519 *r, const unsigned char x[32]);

void shortsc25519_from16bytes(shortsc25519 *r, const unsigned char x[16]);

void sc25519_from64bytes(sc25519 *r, const unsigned char x[64]);

void sc25519_from_shortsc(sc25519 *r, const shortsc25519 *x);

void sc25519_to32bytes(unsigned char r[32], const sc25519 *x);

int sc25519_iszero_vartime(const sc25519 *x);

int sc25519_isshort_vartime(const sc25519 *x);

int sc25519_lt_vartime(const sc25519 *x, const sc25519 *y);

void sc25519_add(sc25519 *r, const sc25519 *x, const sc25519 *y);

void sc25519_sub_nored(sc25519 *r, const sc25519 *x, const sc25519 *y);

void sc25519_mul(sc25519 *r, const sc25519 *x, const sc25519 *y);

void sc25519_mul_shortsc(sc25519 *r, const sc25519 *x, const shortsc25519 *y);

/* Convert s into a representation of the form \sum_{i=0}^{84}r[i]2^3
 * with r[i] in {-4,...,3}
 */
void sc25519_window3(signed char r[85], const sc25519 *s);

/* Convert s into a representation of the form \sum_{i=0}^{50}r[i]2^5
 * with r[i] in {-16,...,15}
 */
void sc25519_window5(signed char r[51], const sc25519 *s);

void sc25519_2interleave2(unsigned char r[127], const sc25519 *s1, const sc25519 *s2);

#endif
