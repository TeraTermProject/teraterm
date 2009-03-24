/*
 * $Id: String.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_STRING_H_
#define _YCL_STRING_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <string.h>

namespace yebisuya {

// 文字列の管理・操作を行うクラス。
class String {
private:
	// 文字列を格納するバッファ。
	// 文字列の前には参照カウンタを持ち、
	// 代入や破棄の際にはそこを変更する。
	const char* string;

	// utilities
	// 文字列を格納するバッファを作成する。
	// 文字列と参照カウンタの分の領域を確保する。
	// 参照カウンタは0になっている。
	// 引数:
	//	length 文字列の長さ。
	// 返値:
	//	作成したバッファの文字列部のアドレス。
	static char* createBuffer(size_t length) {
		size_t* count = (size_t*) new unsigned char[sizeof (size_t) + sizeof (char) * (length + 1)];
		*count = 0;
		return (char*) (count + 1);
	}
	// 文字列を格納したバッファを作成する。
	// 引数:
	//	source 格納する文字列。
	// 返値:
	//	作成したバッファの文字列部のアドレス。
	static const char* create(const char* source) {
		return source != NULL ? create(source, strlen(source)) : NULL;
	}
	// 文字列を格納したバッファを作成する。
	// 引数:
	//	source 格納する文字列。
	//	length 文字列の長さ。
	// 返値:
	//	作成したバッファの文字列部のアドレス。
	static const char* create(const char* source, size_t length) {
		if (source != NULL) {
			char* buffer = createBuffer(length);
			memcpy(buffer, source, length);
			buffer[length] = '\0';
			return buffer;
		}
		return NULL;
	}
	// 二つの文字列を連結し格納したバッファを作成する。
	// 引数:
	//	str1 連結する文字列(前)。
	//	str2 連結する文字列(後)。
	// 返値:
	//	作成したバッファの文字列部のアドレス。
	static const char* concat(const char* str1, const char* str2) {
		size_t len1 = strlen(str1);
		size_t len2 = strlen(str2);
		char* buffer = createBuffer(len1 + len2);
		memcpy(buffer, str1, len1);
		memcpy(buffer + len1, str2, len2);
		buffer[len1 + len2] = '\0';
		return buffer;
	}
	// private methods
	// 参照カウンタを減らし、0になったらバッファを破棄する。
	void release() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			if (--*count == 0)
				delete[] (unsigned char*) count;
		}
	}
	// 参照カウンタを増やす。
	void add() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			++*count;
		}
	}
	// 別のバッファと置き換える。
	// 元のバッファの参照カウンタを減らし、
	// 新しいバッファの参照カウンタを増やす。
	// 引数:
	//	source 置き換える新しいバッファ。
	void set(const char* source) {
		if (string != source) {
			release();
			string = source;
			add();
		}
	}
public:
	// constructor
	// デフォルトコンストラクタ。
	// NULLが入っているので、このままで文字列操作するとアクセス違反になるので注意。
	String():string(NULL) {
	}
	// 元の文字列を指定するコンストラクタ。
	// 引数:
	//	source 元の文字列。
	String(const char* source):string(NULL) {
		set(create(source));
	}
	// 元の文字列を長さ付きで指定するコンストラクタ。
	// 引数:
	//	source 元の文字列。
	//	length 文字列の長さ。
	String(const char* source, size_t length):string(NULL) {
		set(create(source, length));
	}
	// コピーコンストラクタ。
	// 引数:
	//	source 元の文字列。
	String(const String& source):string(NULL) {
		set(source.string);
	}
	// 二つの文字列を連結するコンストラクタ。
	// 引数:
	//	str1 前になる文字列。
	//	str2 後になる文字列。
	String(const char* str1, const char* str2):string(NULL) {
		set(concat(str1, str2));
	}
	// 二つの文字列を連結するコンストラクタ。
	// 引数:
	//	str1 前になる文字列。
	//	str2 後になる文字列。
	String(const String& str1, const char* str2):string(NULL) {
		set(*str2 != '\0' ? concat(str1.string, str2) : str1.string);
	}
	// destructor
	// デストラクタ。
	// 派生することは考えていないので仮想関数にはしない。
	~String() {
		release();
	}
	// public methods
	// この文字列の後に指定の文字列を連結する。
	// 引数:
	//	source 連結する文字列。
	// 返値:
	//	連結された文字列。
	String concat(const char* source)const {
		return String(*this, source);
	}
	// 文字列との比較を行う。
	// NULLとも比較できる。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	等しければ0、strの方が大きければ負、小さければ正。
	int compareTo(const char* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return strcmp(string, str);
	}
	// 文字列との比較を大文字小文字の区別なしで行う。
	// NULLとも比較できる。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	等しければ0、strの方が大きければ負、小さければ正。
	int compareToIgnoreCase(const char* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return _stricmp(string, str);
	}
	// 文字列との比較を行う。
	// NULLとも比較できる。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	等しければ真。
	bool equals(const char* str)const {
		return compareTo(str) == 0;
	}
	// 文字列との比較を大文字小文字の区別なしで行う。
	// NULLとも比較できる。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	等しければ真。
	bool equalsIgnoreCase(const char* str)const {
		return compareToIgnoreCase(str) == 0;
	}
	// 指定された文字列で始まっているかどうかを判定する。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	指定された文字列で始まっていれば真。
	bool startsWith(const char* str)const {
		return startsWith(str, 0);
	}
	// 指定の位置から指定された文字列で始まっているかどうかを判定する。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	指定された文字列で始まっていれば真。
	bool startsWith(const char* str, int offset)const {
		return strncmp(string, str, strlen(str)) == 0;
	}
	// 指定された文字列で終わっているかどうかを判定する。
	// 引数:
	//	str 比較する文字列。
	// 返値:
	//	指定された文字列で終わっていれば真。
	// 
	bool endsWith(const char* str)const {
		size_t str_length = strlen(str);
		size_t string_length = length();
		if (string_length < str_length)
			return false;
		return strcmp(string + string_length - str_length, str) == 0;
	}
	// 指定の文字がどの位置にあるかを探す。
	// 引数:
	//	chr 探す文字。
	// 返値:
	//	文字の見つかったインデックス。見つからなければ-1。
	int indexOf(char chr)const {
		return indexOf(chr, 0);
	}
	// 指定の文字がどの位置にあるかを指定の位置から探す。
	// 引数:
	//	chr 探す文字。
	//	from 探し始める位置。
	// 返値:
	//	文字の見つかったインデックス。見つからなければ-1。
	int indexOf(char chr, size_t from)const {
		if (from < 0)
			from = 0;
		else if (from >= length())
			return -1;
		const char* found = strchr(string + from, chr);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// 指定の文字列がどの位置にあるかを探す。
	// 引数:
	//	str 探す文字列。
	// 返値:
	//	文字列の見つかったインデックス。見つからなければ-1。
	int indexOf(const char* str)const {
		return indexOf(str, 0);
	}
	// 指定の文字列がどの位置にあるかを指定の位置から探す。
	// 引数:
	//	str 探す文字列。
	//	from 探し始める位置。
	// 返値:
	//	文字列の見つかったインデックス。見つからなければ-1。
	// 
	int indexOf(const char* str, size_t from)const {
		if (from < 0)
			from = 0;
		else if (from >= length())
			return -1;
		const char* found = strstr(string + from, str);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// 文字列の長さを返す。
	size_t length()const {
		return strlen(string);
	}
	// 指定の文字が最後に見つかる位置を取得する。
	// 引数:
	//	chr 探す文字。
	// 返値:
	//	文字の見つかったインデックス。見つからなければ-1。
	int lastIndexOf(char chr)const {
		return lastIndexOf(chr, (size_t) -1);
	}
	// 指定の文字が指定の位置よりも前で最後に見つかる位置を取得する。
	// 引数:
	//	chr 探す文字。
	//	from 探し始める位置。
	// 返値:
	//	文字の見つかったインデックス。見つからなければ-1。
	int lastIndexOf(char chr, size_t from)const {
		size_t len = length();
		if (from > len - 1)
			from = len - 1;
		const char* s = string;
		const char* end = string + from;
		const char* found = NULL;
		while (*s != '0' && s <= end) {
			if (*s == chr)
				found = s;
			if (isLeadByte(*s))
				s++;
			s++;
		}
		return found != NULL ? found - string : -1;
	}
	// 指定の文字列が最後に見つかる位置を取得する。
	// 引数:
	//	str 探す文字列。
	// 返値:
	//	文字列の見つかったインデックス。見つからなければ-1。
	int lastIndexOf(const char* str)const {
		return lastIndexOf(str, (size_t) -1);
	}
	// 指定の文字列が指定の位置よりも前で最後に見つかる位置を取得する。
	// 引数:
	//	str 探す文字列。
	//	from 探し始める位置。
	// 返値:
	//	文字列の見つかったインデックス。見つからなければ-1。
	int lastIndexOf(const char* str, size_t from)const {
		size_t len = length();
		size_t str_len = strlen(str);
		if (from > len - str_len)
			from = len - str_len;
		const char* s = string + from;
		while (s >= string) {
			if (strncmp(s, str, str_len) == 0)
				return s - string;
			s--;
		}
		return -1;
	}
	// 文字列の一部を取り出す。
	// 引数:
	//	start 取り出す文字列の先頭の位置。
	// 返値:
	//	文字列の一部。
	String substring(int start)const {
		return String(string + start);
	}
	// 文字列の一部を取り出す。
	// 引数:
	//	start 取り出す文字列の先頭の位置。
	//	end 取り出す文字列の後の位置。
	// 返値:
	//	文字列の一部。
	String substring(int start, int end)const {
		return String(string + start, end - start);
	}
	// 指定の位置にある文字を取り出す。
	// 引数:
	//	index 取り出す文字の位置。
	// 返値:
	//	指定の位置にある文字。
	char charAt(size_t index)const {
		return index >= 0 && index < length() ? string[index] : '\0';
	}
	// 指定の文字を指定の文字に置き換えます。
	// 引数:
	//	oldChr 元の文字。
	//	newChr 置き換える文字。
	// 返値:
	//	置換後の文字列。
	String replace(char oldChr, char newChr)const {
		String result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (String::isLeadByte(*s))
				s++;
			else if (*s == oldChr)
				*s = newChr;
			s++;
		}
		return result;
	}
	// 文字列中の大文字を小文字に変換する。
	// 返値:
	//	変換後の文字列。
	String toLowerCase()const {
		String result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (String::isLeadByte(*s))
				s++;
			else if ('A' <= *s && *s <= 'Z')
				*s += 'a' - 'A';
			s++;
		}
		return result;
	}
	// 文字列中の小文字を大文字に変換する。
	// 返値:
	//	変換後の文字列。
	String toUpperCase()const {
		String result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (String::isLeadByte(*s))
				s++;
			else if ('a' <= *s && *s <= 'z')
				*s += 'A' - 'a';
			s++;
		}
		return result;
	}
	// 文字列の前後の空白文字を削除する。
	// 返値:
	//	削除後の文字列。
	String trim()const {
		const char* s = string;
		while (*s != '\0' && (unsigned char) *s <= ' ')
			s++;
		const char* start = s;
		s = string + length();
		while (s > start && (*s != '\0' && (unsigned char) *s <= ' '))
			s--;
		return String(start, s - start);
	}

	// operators

	// const char*へのキャスト演算子
	// 返値:
	//	文字列へのアドレス。
	operator const char*()const {
		return string;
	}
	// char配列のように扱うための[]演算子。
	// 引数:
	//	index 取得する文字のインデックス。
	// 返値:
	//	指定のインデックスにある文字。
	char operator[](size_t index)const {
		return charAt(index);
	}
	// 文字列を連結するための+演算子。
	// 引数:
	//	source 連結する文字列。
	// 返値:
	//	連結した文字列。
	String operator+(const char* source)const {
		return String(string, source);
	}
	// 文字列を連結するための+演算子。
	// 引数:
	//	source 連結する文字列。
	// 返値:
	//	連結した文字列。
	String operator+(const String& source)const {
		return *string != '\0' ? String(string, source.string) : source;
	}
	// 文字列を連結するための+演算子。
	// 引数:
	//	str1 連結する文字列(前)。
	//	str2 連結する文字列(後)。
	// 返値:
	//	連結した文字列。
	friend String operator+(const char* str1, const String& str2) {
		return *str1 != '\0' ? String(str1, str2.string) : str2;
	}
	// 代入演算子。
	// 引数:
	//	source 代入する文字列。
	// 返値:
	//	代入結果。
	String& operator=(const char* source) {
		set(create(source));
		return *this;
	}
	// 代入演算子。
	// 引数:
	//	source 代入する文字列。
	// 返値:
	//	代入結果。
	String& operator=(const String& source) {
		set(source.string);
		return *this;
	}
	// 連結した結果を代入する演算子。
	// 引数:
	//	source 連結する文字列。
	// 返値:
	//	連結結果。
	String& operator+=(const char* source) {
		if (*source != '\0')
			set(concat(string, source));
		return *this;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が等しければ真。
	bool operator==(const String& str)const {
		return compareTo(str.string) == 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が等しければ真。
	bool operator==(const char* str)const {
		return compareTo(str) == 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が等しければ真。
	friend bool operator==(const char* str1, const String& str2) {
		return str2.compareTo(str1) == 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が等しくなければ真。
	bool operator!=(const String& str)const {
		return compareTo(str) != 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が等しくなければ真。
	bool operator!=(const char* str)const {
		return compareTo(str) != 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が等しくなければ真。
	friend bool operator!=(const char* str1, const String& str2) {
		return str2.compareTo(str1) != 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が大きければ真。
	bool operator<(const String& str)const {
		return compareTo(str) < 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が大きければ真。
	bool operator<(const char* str)const {
		return compareTo(str) < 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が大きければ真。
	friend bool operator<(const char* str1, const String& str2) {
		return str2.compareTo(str1) > 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が大きいか等しければ真。
	bool operator<=(const String& str)const {
		return compareTo(str) <= 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が大きいか等しければ真。
	bool operator<=(const char* str)const {
		return compareTo(str) <= 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が大きいか等しければ真。
	friend bool operator<=(const char* str1, const String& str2) {
		return str2.compareTo(str1) >= 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が小さければ真。
	bool operator>(const String& str)const {
		return compareTo(str) > 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が小さければ真。
	bool operator>(const char* str)const {
		return compareTo(str) > 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が小さければ真。
	friend bool operator>(const char* str1, const String& str2) {
		return str2.compareTo(str1) < 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が小さいか等しければ真。
	bool operator>=(const String& str)const {
		return compareTo(str) >= 0;
	}
	// 比較演算子。
	// 引数:
	//	str 比較対象の文字列。
	// 返値:
	//	strの方が小さいか等しければ真。
	bool operator>=(const char* str)const {
		return compareTo(str) >= 0;
	}
	// 比較演算子。
	// 引数:
	//	str1 比較する文字列。
	//	str2 比較する文字列。
	// 返値:
	//	str1よりstr2の方が小さいか等しければ真。
	friend bool operator>=(const char* str1, const String& str2) {
		return str2.compareTo(str1) <= 0;
	}

	// public utilities

	// 2バイト文字の最初の1バイトかどうかを判定する。
	// 引数:
	//	判定するバイト。
	// 返値:
	//	2バイト文字の最初の1バイトであれば真。
	static bool isLeadByte(char ch) {
	#ifdef _INC_WINDOWS
		return ::IsDBCSLeadByte(ch) != 0;
	#else
		return (ch & 0x80) != 0;
	#endif
	}
};

}

#endif//_YCL_STRING_H_
