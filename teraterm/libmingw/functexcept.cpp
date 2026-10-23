/*
 * Copyright (C) 2026- TeraTerm Project
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

/*
 *	libstdc++ の std::__throw_*() の置き換え
 *
 *	-fno-exceptions でビルドしても、libstdc++ のヘッダ(std::vector など)は
 *	エラー時に std::__throw_length_error() などを呼ぶ
 *	libstdc++ の実装(functexcept.o)は例外を throw するため
 *	C++ の例外処理一式がリンクされ、GCC 13 以降では libgcc の gthr-win32.o が
 *	Windows 95 にない API(TryEnterCriticalSection(), GetThreadId() など)を使用してしまう
 *
 *	functexcept.o が定義している関数をすべて abort() するものに置き換えて、
 *	functexcept.o がリンクされないようにする
 */

#include <stdlib.h>
#include <bits/functexcept.h>

namespace std {

void __throw_bad_exception()
{
	abort();
}

void __throw_bad_alloc()
{
	abort();
}

void __throw_bad_array_new_length()
{
	abort();
}

void __throw_bad_cast()
{
	abort();
}

void __throw_bad_typeid()
{
	abort();
}

void __throw_logic_error(const char *)
{
	abort();
}

void __throw_domain_error(const char *)
{
	abort();
}

void __throw_invalid_argument(const char *)
{
	abort();
}

void __throw_length_error(const char *)
{
	abort();
}

void __throw_out_of_range(const char *)
{
	abort();
}

void __throw_out_of_range_fmt(const char *, ...)
{
	abort();
}

void __throw_runtime_error(const char *)
{
	abort();
}

void __throw_range_error(const char *)
{
	abort();
}

void __throw_overflow_error(const char *)
{
	abort();
}

void __throw_underflow_error(const char *)
{
	abort();
}

}	// namespace std
