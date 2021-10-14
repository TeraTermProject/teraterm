/*
 * Copyright (C) 2021- TeraTerm Project
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

/* StringBuffer.h を基に作成 */

#ifndef _YCL_WSTRINGBUFFER_H_
#define _YCL_WSTRINGBUFFER_H_

#pragma once

#include <YCL/common.h>

#include <YCL/WString.h>

#include <malloc.h>

namespace yebisuya {

// 可変長の文字列を扱うためのクラス。
class WStringBuffer {
private:
	// 文字列を格納するバッファ。
	wchar_t* buffer;
	// 現在有効な文字列の長さ。
	size_t validLength;
	// バッファの大きさ。
	size_t bufferSize;
	enum {
		// バッファを広げる際に使用するサイズ。
		INIT_CAPACITY = 16,
	};
	// バッファを初期化する。
	// 引数:
	//	source	初期文字列。
	//	length	初期文字列の長さ。
	//	capacity	バッファの初期サイズ。
	void init(const wchar_t* source, size_t length, size_t capacity) {
		if ((capacity != 0 || length != 0) && capacity < length + INIT_CAPACITY)
			capacity = length + INIT_CAPACITY;
		validLength = length;
		bufferSize = capacity;
		if (bufferSize == 0) {
			buffer = NULL;
		}else{
			buffer = new wchar_t[bufferSize];
		}
		if (source != NULL) {
			memcpy(buffer, source, validLength);
		}
		memset(buffer + validLength, '\0', bufferSize - validLength);
	}
public:
	// デフォルトコンストラクタ。
	WStringBuffer() {
		init(NULL, 0, 0);
	}
	// バッファの初期サイズを指定するコンストラクタ。
	// 引数:
	//	capacity バッファの初期サイズ。
	WStringBuffer(size_t capacity) {
		init(NULL, 0, capacity);
	}
	// バッファの初期文字列を指定するコンストラクタ。
	// 引数:
	//	source	初期文字列。
	WStringBuffer(const wchar_t* source) {
		init(source, wcslen(source), 0);
	}
	// コピーコンストラクタ。
	// 引数:
	//	source	初期文字列。
	WStringBuffer(const WStringBuffer& source) {
		init(source.buffer, source.validLength, source.bufferSize);
	}
	// デストラクタ。
	~WStringBuffer() {
		delete[] buffer;
	}

	// 現在有効な文字列の長さを取得する。
	// 返値:
	//	有効な文字列の長さ。
	size_t length()const {
		return validLength;
	}
	// バッファのサイズを取得する。
	// 返値:
	//	バッファのサイズ。
	size_t capacity()const {
		return bufferSize;
	}
	// バッファのサイズを指定の長さが収まるように調節する。
	// 引数:
	//	newLength	調節する長さ。
	void ensureCapacity(size_t newLength) {
		if (bufferSize < newLength) {
			wchar_t* oldBuffer = buffer;
			init(oldBuffer, validLength, newLength + INIT_CAPACITY);
			delete[] oldBuffer;
		}
	}
	// 有効な文字列長を変更する。
	// 引数:
	//	newLength	新しい文字列長。
	void setLength(size_t newLength) {
		if (validLength < newLength)
			ensureCapacity(newLength);
		validLength = newLength;
	}
	// 指定の位置の文字を取得する。
	// 引数:
	//	index	文字の位置。
	// 返値:
	//	指定の位置の文字。
	char charAt(size_t index)const {
		return index < validLength ? buffer[index] : '\0';
	}
	// 指定の位置の文字を取得する。
	// 引数:
	//	index	文字の位置。
	// 返値:
	//	指定の位置の文字の参照。
	wchar_t& charAt(size_t index) {
		if (index >= validLength) {
			ensureCapacity(validLength + 1);
			index = validLength++;
		}
		return buffer[index];
	}
	// 指定の位置の文字を変更する。
	// 引数:
	//	index	変更する文字の位置。
	//	chr	変更する文字。
	void setCharAt(int index, char chr) {
		charAt(index) = chr;
	}
	// 文字を追加する。
	// 引数:
	//	chr	追加する文字。
	// 返値:
	//	追加結果。
	WStringBuffer& append(wchar_t chr) {
		charAt(validLength) = chr;
		return *this;
	}
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	// 返値:
	//	追加結果。
	WStringBuffer& append(const wchar_t* source) {
		return append(source, wcslen(source));
	}
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	//	length	追加する文字列の長さ。
	// 返値:
	//	追加結果。
	WStringBuffer& append(const wchar_t* source, size_t length) {
		size_t oldLength = validLength;
		ensureCapacity(validLength + length);
		memcpy(buffer + oldLength, source, length);
		validLength += length;
		return *this;
	}
	// 指定の位置の文字を削除する。
	// 引数:
	//	start	削除する位置。
	// 返値:
	//	削除結果。
	WStringBuffer& remove(size_t index) {
		return remove(index, index + 1);
	}
	// 指定の位置の文字列を削除する。
	// 引数:
	//	start	削除する先頭位置。
	//	end	削除する終わりの位置。
	// 返値:
	//	削除結果。
	WStringBuffer& remove(size_t start, size_t end) {
		if (start < end) {
			if (end < validLength){
				memcpy(buffer + start, buffer + end, validLength - end);
				validLength -= end - start;
			}else{
				validLength = start;
			}
		}
		return *this;
	}
	// 指定の位置の文字列を置換する。
	// 引数:
	//	start	置換する先頭位置。
	//	end	置換する終わりの位置。
	//	source	置換する文字列。
	// 返値:
	//	置換結果。
	WStringBuffer& replace(size_t start, size_t end, const char* source) {
		if (end > validLength)
			end = validLength;
		if (start < end) {
			size_t length = strlen(source);
			size_t oldLength = validLength;
			ensureCapacity(validLength += length - (end - start));
			memcpy(buffer + start + length, buffer + end, oldLength - end);
			memcpy(buffer + start, source, length);
		}
		return *this;
	}
	// 指定の位置の文字列を取得する。
	// 引数:
	//	start	取得する文字列の先頭位置。
	// 返値:
	//	指定の位置の文字列。
	WString substring(size_t index)const {
		return WString(buffer + index, validLength - index);
	}
	// 指定の位置の文字列を取得する。
	// 引数:
	//	start	取得する文字列の先頭位置。
	//	end	取得する文字列の終わりの位置。
	// 返値:
	//	指定の位置の文字列。
	WString substring(size_t start, size_t end)const {
		if (end > validLength)
			end = validLength;
		return WString(buffer + start, end - start);
	}
	// 指定の位置に文字を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字。
	// 返値:
	//	挿入結果。
	WStringBuffer& insert(size_t index, char chr) {
		return insert(index, &chr, 1);
	}
	// 指定の位置に文字列を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字列。
	// 返値:
	//	挿入結果。
	WStringBuffer& insert(size_t index, const char* source) {
		return insert(index, source, strlen(source));
	}
	// 指定の位置に文字列を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字列。
	//	length	文字列の長さ。
	// 返値:
	//	挿入結果。
	WStringBuffer& insert(size_t index, const char* source, size_t length) {
		if (index >= validLength)
			index = validLength;
		size_t oldLength = validLength;
		ensureCapacity(validLength + length);
		char* temp = (char*) alloca(oldLength - index);
		memcpy(temp, buffer + index, oldLength - index);
		memcpy(buffer + index, source, length);
		memcpy(buffer + index + length, temp, oldLength - index);
		validLength += length;
		return *this;
	}
#if 0
	// 文字列を反転する。
	// 返値:
	//	反転結果。
	WStringBuffer& reverse() {
		char* temporary = (char*) alloca(sizeof (char) * validLength);
		char* dst = temporary + validLength;
		wchar_t* src = buffer;
		while (temporary < dst) {
			if (String::isLeadByte(*src)) {
				char pre = *src++;
				*--dst = *src++;
				*--dst = pre;
			}else{
				*--dst = *src++;
			}
		}
		memcpy(buffer, temporary, validLength);
		return *this;
	}
#endif
	// 文字列を取得する。
	// 返値:
	//	現在設定されている文字列。
	WString toString()const {
		return WString(buffer, validLength);
	}

	// 一文字だけの文字列に変更する。
	// 引数:
	//	変更する一文字。
	// 返値:
	//	変更結果。
	WStringBuffer& set(char chr) {
		ensureCapacity(1);
		buffer[0] = chr;
		validLength = 1;
		return *this;
	}
	// 指定の文字列に変更する。
	// 引数:
	//	source	変更する文字列。
	// 返値:
	//	変更結果。
	WStringBuffer& set(const char* source) {
		size_t length = strlen(source);
		ensureCapacity(validLength = length);
		memcpy(buffer, source, length);
		return *this;
	}

	// char*に変換するキャスト演算子。
	// バッファのアドレスを取得する。
	// 返値:
	//	バッファのアドレス。
	operator wchar_t*() {
		return buffer;
	}
	// Stringに変換するキャスト演算子。
	// 文字列を取得する。
	// 返値:
	//	現在設定されている文字列。
	operator WString()const {
		return toString();
	}
	// 代入演算子。
	// 一文字だけの文字列に変更する。
	// 引数:
	//	ch	変更する一文字。
	// 返値:
	//	代入結果。
	WStringBuffer& operator=(char ch) {
		return set(ch);
	}
	// 代入演算子。
	// 指定の文字列に変更する。
	// 引数:
	//	source	変更する文字列。
	// 返値:
	//	代入結果。
	WStringBuffer& operator=(const char* source) {
		return set(source);
	}
	// 連結代入演算子。
	// 文字を追加する。
	// 引数:
	//	ch	追加する文字。
	// 返値:
	//	代入結果。
	WStringBuffer& operator+=(char ch) {
		return append(ch);
	}
	// 連結代入演算子。
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	// 返値:
	//	代入結果。
	WStringBuffer& operator+=(const wchar_t* source) {
		return append(source);
	}
};

}

#endif//_YCL_WSTRINGBUFFER_H_
