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
 *	operator new / delete の置き換え
 *
 *	libstdc++ の operator new はメモリ不足のとき std::bad_alloc を throw する
 *	そのため C++ の例外処理一式(eh_alloc.o など)がリンクされ、
 *	GCC 13 以降では libgcc の gthr-win32.o が Windows 95 にない API
 *	(TryEnterCriticalSection(), GetThreadId() など)を使用してしまう
 *
 *	malloc()/free() を使う operator new / delete に置き換えて
 *	例外処理がリンクされないようにする
 *	メモリ不足のときは例外の代わりに abort() する
 */

#include <stdlib.h>
#include <stdint.h>
#include <new>

static void *alloc(size_t size)
{
	if (size == 0) {
		size = 1;
	}
	void *p = malloc(size);
	if (p == NULL) {
		abort();
	}
	return p;
}

void *operator new(size_t size)
{
	return alloc(size);
}

void *operator new[](size_t size)
{
	return alloc(size);
}

void *operator new(size_t size, const std::nothrow_t &) noexcept
{
	return malloc(size == 0 ? 1 : size);
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept
{
	return malloc(size == 0 ? 1 : size);
}

void operator delete(void *p) noexcept
{
	free(p);
}

void operator delete[](void *p) noexcept
{
	free(p);
}

void operator delete(void *p, size_t) noexcept
{
	free(p);
}

void operator delete[](void *p, size_t) noexcept
{
	free(p);
}

void operator delete(void *p, const std::nothrow_t &) noexcept
{
	free(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept
{
	free(p);
}

#if defined(__cpp_aligned_new)
/*
 *	aligned new / delete (C++17, alignas() で既定より大きなアラインメントを持つ型)
 *
 *	古い msvcrt.dll には _aligned_malloc() がないため malloc() で実装する
 *	多めに確保してアラインし、直前に malloc() が返したポインタを保存しておく
 */

static void *aligned_alloc_nothrow(size_t size, std::align_val_t al) noexcept
{
	size_t align = static_cast<size_t>(al);
	if (align < sizeof(void *)) {
		align = sizeof(void *);
	}
	if (size == 0) {
		size = 1;
	}
	if (size > (size_t)-1 - align - sizeof(void *)) {
		return NULL;
	}
	void *raw = malloc(size + align - 1 + sizeof(void *));
	if (raw == NULL) {
		return NULL;
	}
	uintptr_t addr = (reinterpret_cast<uintptr_t>(raw) + sizeof(void *) + align - 1) & ~(uintptr_t)(align - 1);
	void *p = reinterpret_cast<void *>(addr);
	static_cast<void **>(p)[-1] = raw;
	return p;
}

static void *aligned_alloc_abort(size_t size, std::align_val_t al)
{
	void *p = aligned_alloc_nothrow(size, al);
	if (p == NULL) {
		abort();
	}
	return p;
}

static void aligned_free(void *p) noexcept
{
	if (p != NULL) {
		free(static_cast<void **>(p)[-1]);
	}
}

void *operator new(size_t size, std::align_val_t al)
{
	return aligned_alloc_abort(size, al);
}

void *operator new[](size_t size, std::align_val_t al)
{
	return aligned_alloc_abort(size, al);
}

void *operator new(size_t size, std::align_val_t al, const std::nothrow_t &) noexcept
{
	return aligned_alloc_nothrow(size, al);
}

void *operator new[](size_t size, std::align_val_t al, const std::nothrow_t &) noexcept
{
	return aligned_alloc_nothrow(size, al);
}

void operator delete(void *p, std::align_val_t) noexcept
{
	aligned_free(p);
}

void operator delete[](void *p, std::align_val_t) noexcept
{
	aligned_free(p);
}

void operator delete(void *p, size_t, std::align_val_t) noexcept
{
	aligned_free(p);
}

void operator delete[](void *p, size_t, std::align_val_t) noexcept
{
	aligned_free(p);
}

void operator delete(void *p, std::align_val_t, const std::nothrow_t &) noexcept
{
	aligned_free(p);
}

void operator delete[](void *p, std::align_val_t, const std::nothrow_t &) noexcept
{
	aligned_free(p);
}
#endif
