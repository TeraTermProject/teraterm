/*
 * Copyright (C) 2019- TeraTerm Project
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	wchar_t *port_name;			// �|�[�g�� "COM%d" �ȊO������
	int port_no;				// "COM%d" �� %d����, 0�̏ꍇ�� "COM%d" �ȊO, 1..128(9x)/255(xp) (2^32�����?)
	wchar_t *friendly_name;		// ���݂��Ȃ��ꍇ�� NULL
	wchar_t *property;			// �ʂ̒l���܂Ƃ߂�������
	// �ʂ̒l
	wchar_t *class_name;
	wchar_t *instance_id;
	wchar_t *manufacturer;
	wchar_t *provider_name;
	wchar_t *driverdate;
	wchar_t *driverversion;
} ComPortInfo_t;

ComPortInfo_t *ComPortInfoGet(int *count);
void ComPortInfoFree(ComPortInfo_t *info, int count);

#ifdef __cplusplus
}
#endif
