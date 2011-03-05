/*
 * $Id: Array.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_ARRAY_H_
#define _YCL_ARRAY_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/Object.h>

namespace yebisuya {

// 配列を管理するクラス
template<class TYPE>
class Array : public Object {
private:
	// 配列の実体
	TYPE*const array;
	// 代入演算子は実装しない
	void operator=(Array&);
public:
	// 配列の長さ
	const int length;
	// コンストラクタ
	Array(int length):array(new TYPE[length]), length(length) {
	}
	Array(const TYPE source[], int length):array(new TYPE[length]), length(length) {
		copyFrom(source);
	}
	Array(Array* source):array(new TYPE[source->length]), length(source->length) {
		copyFrom(source);
	}
protected:
	virtual ~Array() {
		delete[] array;
	}
public:
	// 配列要素演算子(代入用)
	TYPE& operator[](int index) {
		return array[index];
	}
	// 配列要素演算子(参照用)
	TYPE operator[](int index)const {
		return array[index];
	}
	void copyFrom(const TYPE source[]) {
		for (int i = 0; i < length; i++) {
			array[i] = source[i];
		}
	}
	void copyFrom(Array* source) {
		copy(source->array[i]);
	}
	void copyTo(TYPE distination[])const {
		for (int i = 0; i < length; i++) {
			distination[i] = array[i];
		}
	}
	void copyTo(Array* distination)const {
		copyTo(distination->array[i]);
	}
};

}

#endif//_YCL_ARRAY_H_
