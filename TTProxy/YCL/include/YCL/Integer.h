/*
 * $Id: Integer.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_INTEGER_H_
#define _YCL_INTEGER_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/String.h>

namespace yebisuya {

// intをラップするためのクラス。
class Integer {
private:
	int value;
public:
	// デフォルトコンストラクタ
	Integer():value(0){}
	// 初期値付きコンストラクタ
	Integer(int value):value(value){}
	// キャスト演算子
	operator int()const {
		return value;
	}
	static String toString(long value) {
		return toString(value, 10);
	}
	static String toString(long value, int base) {
		bool negative = false;
		if (value < 0) {
			negative = true;
			value = -value; 
		}
		return toString(value, 10, negative);
	}
	static String toString(unsigned long value) {
		return toString(value, 10);
	}
	static String toString(unsigned long value, int base) {
		return toString(value, base, false);
	}
	static String toString(unsigned long value, int base, bool negative) {
		if (base < 2 || base > 36)
			return NULL;
		char buffer[64];
		char* p = buffer + countof(buffer);
		*--p = '\0';
		if (value == 0) {
			*--p = '0';
		}else{
			while (value > 0) {
				int d = value % base;
				value /= base;
				*--p = (d < 10 ? ('0' + d) : ('A' + d - 10));
			}
			if (negative) {
				*--p = '-';
			}
		}
		return p;
	}
	static long parseInt(const char* string) {
		return parseInt(string, 0);
	}
	static long parseInt(const char* string, int base) {
		if (base != 0 && base < 2 || base > 36)
			return 0;
		long v = 0;
		bool negative = false;
		const char* p = string;
		// 空白のスキップ
		while ('\0' < *p && *p <= ' ')
			p++;
		if (*p == '-') {
			negative = true;
			p++;
		}else if (*p == '+') {
			p++;
		}
		// 空白のスキップ
		while ('\0' < *p && *p <= ' ')
			p++;
		// 基数の変更
		if (base == 0) {
			if (*p == '0') {
				p++;
				if (*p == 'x' || *p == 'X') {
					p++;
					base = 16;
				}else{
					base = 8;
				}
			}else{
				base = 10;
			}
		}
		while (*p != '\0') {
			int d;
			if ('0' <= *p && *p <= '9') {
				d = *p - '0';
			}else if ('A' <= *p && *p <= 'Z') {
				d = *p - 'A' + 10;
			}else if ('a' <= *p && *p <= 'z') {
				d = *p - 'a' + 10;
			}else{
				// 余計な文字が見つかれば終了
				break;
			}
			// 基数以上だった場合は終了
			if (d >= base)
				break;
			v = v * base + d;
			// オーバーフローした場合は終了
			if (v < 0)
				break;
			p++;
		}
		if (negative) {
			v = -v;
		}
		return v;
	}
};

}

#endif//_YCL_INTEGER_H_
