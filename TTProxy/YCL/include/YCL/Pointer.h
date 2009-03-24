/*
 * $Id: Pointer.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _YCL_POINTER_H_
#define _YCL_POINTER_H_

#include <YCL/Array.h>

namespace yebisuya {
    
// ポインタをラップするためのクラス。
template<class TYPE, class REFCTRL = Object >
class Pointer {
private:
    // 格納するポインタ
    TYPE* value;
protected:
    // ポインタを置き換える
    void set(TYPE* ptr) {
	value = (TYPE*) REFCTRL::set(value, ptr);
    }
public:
    // デフォルトコンストラクタ
    Pointer():value(NULL) {
    }
    // 初期値付きコンストラクタ
    Pointer(TYPE* ptr):value(NULL) {
	set(ptr);
    }
    // コピーコンストラクタ
    Pointer(const Pointer& ptr):value(NULL) {
	set(ptr.value);
    }
    // デストラクタ
    ~Pointer() {
	set(NULL);
    }
    // 代入演算子
    Pointer& operator=(TYPE* ptr) {
	set(ptr);
	return *this;
    }
    // コピー代入演算子
    Pointer& operator=(const Pointer& ptr) {
	set(ptr.value);
	return *this;
    }
    // 間接演算子
    TYPE& operator*()const {
	return *value;
    }
    // メンバ選択演算子
    TYPE* operator->()const {
	return value;
    }
    // キャスト演算子
    operator TYPE*()const {
	return value;
    }
};
    
// PointerのArray<TYPE>型への特殊化
// 配列要素演算子[]を提供する
template<class TYPE>
class PointerArray : public Pointer< Array<TYPE> > {
public:
    // デフォルトコンストラクタ
    PointerArray() {
    }
    // 初期値付きコンストラクタ
    PointerArray(Array<TYPE>* ptr):Pointer< Array<TYPE> >(ptr) {
    }
    // コピーコンストラクタ
    PointerArray(const PointerArray& ptr):Pointer< Array<TYPE> >(ptr) {
    }
    // 代入演算子
    PointerArray& operator=(Array<TYPE>* ptr) {
	set(ptr);
	return *this;
    }
    // 配列要素演算子
    TYPE& operator[](int index) {
	Array<TYPE>* ptr = *this;
	return (**this)[index];
    }
    TYPE operator[](int index)const {
	Array<TYPE>* ptr = *this;
	return (**this)[index];
    }
};
    
}

#endif//_YCL_POINTER_H_
