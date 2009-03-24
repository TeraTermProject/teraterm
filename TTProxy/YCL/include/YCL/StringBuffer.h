/*
 * $Id: StringBuffer.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_STRINGBUFFER_H_
#define _YCL_STRINGBUFFER_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/String.h>

#include <malloc.h>

namespace yebisuya {

// 可変長の文字列を扱うためのクラス。
class StringBuffer {
private:
	// 文字列を格納するバッファ。
	char* buffer;
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
	void init(const char* source, size_t length, size_t capacity) {
		if ((capacity != 0 || length != 0) && capacity < length + INIT_CAPACITY)
			capacity = length + INIT_CAPACITY;
		validLength = length;
		bufferSize = capacity;
		if (bufferSize == 0) {
			buffer = NULL;
		}else{
			buffer = new char[bufferSize];
		}
		memcpy(buffer, source, validLength);
		memset(buffer + validLength, '\0', bufferSize - validLength);
	}
public:
	// デフォルトコンストラクタ。
	StringBuffer() {
		init(NULL, 0, 0);
	}
	// バッファの初期サイズを指定するコンストラクタ。
	// 引数:
	//	capacity バッファの初期サイズ。
	StringBuffer(size_t capacity) {
		init(NULL, 0, capacity);
	}
	// バッファの初期文字列を指定するコンストラクタ。
	// 引数:
	//	source	初期文字列。
	StringBuffer(const char* source) {
		init(source, strlen(source), 0);
	}
	// コピーコンストラクタ。
	// 引数:
	//	source	初期文字列。
	StringBuffer(const StringBuffer& source) {
		init(source.buffer, source.validLength, source.bufferSize);
	}
	// デストラクタ。
	~StringBuffer() {
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
			char* oldBuffer = buffer;
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
		return index >= 0 && index < validLength ? buffer[index] : '\0';
	}
	// 指定の位置の文字を取得する。
	// 引数:
	//	index	文字の位置。
	// 返値:
	//	指定の位置の文字の参照。
	char& charAt(size_t index) {
		if (index < 0) {
			index = 0;
		}else if (index >= validLength) {
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
	StringBuffer& append(char chr) {
		charAt(validLength) = chr;
		return *this;
	}
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	// 返値:
	//	追加結果。
	StringBuffer& append(const char* source) {
		return append(source, strlen(source));
	}
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	//	length	追加する文字列の長さ。
	// 返値:
	//	追加結果。
	StringBuffer& append(const char* source, size_t length) {
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
	StringBuffer& remove(size_t index) {
		return remove(index, index + 1);
	}
	// 指定の位置の文字列を削除する。
	// 引数:
	//	start	削除する先頭位置。
	//	end	削除する終わりの位置。
	// 返値:
	//	削除結果。
	StringBuffer& remove(size_t start, size_t end) {
		if (start <= 0)
			start = 0;
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
	StringBuffer& replace(size_t start, size_t end, const char* source) {
		if (start < 0)
			start = 0;
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
	String substring(size_t index)const {
		if (index < 0)
			index = 0;
		return String(buffer + index, validLength - index);
	}
	// 指定の位置の文字列を取得する。
	// 引数:
	//	start	取得する文字列の先頭位置。
	//	end	取得する文字列の終わりの位置。
	// 返値:
	//	指定の位置の文字列。
	String substring(size_t start, size_t end)const {
		if (start < 0)
			start = 0;
		if (end > validLength)
			end = validLength;
		return String(buffer + start, end - start);
	}
	// 指定の位置に文字を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字。
	// 返値:
	//	挿入結果。
	StringBuffer& insert(size_t index, char chr) {
		return insert(index, &chr, 1);
	}
	// 指定の位置に文字列を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字列。
	// 返値:
	//	挿入結果。
	StringBuffer& insert(size_t index, const char* source) {
		return insert(index, source, strlen(source));
	}
	// 指定の位置に文字列を挿入する。
	// 引数:
	//	index	挿入する位置。
	//	source	挿入する文字列。
	//	length	文字列の長さ。
	// 返値:
	//	挿入結果。
	StringBuffer& insert(size_t index, const char* source, size_t length) {
		if (index < 0)
			index = 0;
		else if (index >= validLength)
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
	// 文字列を反転する。
	// 返値:
	//	反転結果。
	StringBuffer& reverse() {
		char* temporary = (char*) alloca(sizeof (char) * validLength);
		char* dst = temporary + validLength;
		char* src = buffer;
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
	// 文字列を取得する。
	// 返値:
	//	現在設定されている文字列。
	String toString()const {
		return String(buffer, validLength);
	}

	// 一文字だけの文字列に変更する。
	// 引数:
	//	変更する一文字。
	// 返値:
	//	変更結果。
	StringBuffer& set(char chr) {
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
	StringBuffer& set(const char* source) {
		size_t length = strlen(source);
		ensureCapacity(validLength = length);
		memcpy(buffer, source, length);
		return *this;
	}

	// char*に変換するキャスト演算子。
	// バッファのアドレスを取得する。
	// 返値:
	//	バッファのアドレス。
	operator char*() {
		return buffer;
	}
	// Stringに変換するキャスト演算子。
	// 文字列を取得する。
	// 返値:
	//	現在設定されている文字列。
	operator String()const {
		return toString();
	}
	// 代入演算子。
	// 一文字だけの文字列に変更する。
	// 引数:
	//	ch	変更する一文字。
	// 返値:
	//	代入結果。
	StringBuffer& operator=(char ch) {
		return set(ch);
	}
	// 代入演算子。
	// 指定の文字列に変更する。
	// 引数:
	//	source	変更する文字列。
	// 返値:
	//	代入結果。
	StringBuffer& operator=(const char* source) {
		return set(source);
	}
	// 連結代入演算子。
	// 文字を追加する。
	// 引数:
	//	ch	追加する文字。
	// 返値:
	//	代入結果。
	StringBuffer& operator+=(char ch) {
		return append(ch);
	}
	// 連結代入演算子。
	// 文字列を追加する。
	// 引数:
	//	source	追加する文字列。
	// 返値:
	//	代入結果。
	StringBuffer& operator+=(const char* source) {
		return append(source);
	}
};

}

#endif//_YCL_STRINGBUFFER_H_
