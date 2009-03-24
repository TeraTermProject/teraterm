/*
 * $Id: Object.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_OBJECT_H_
#define _YCL_OBJECT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace yebisuya {

// 参照カウンタ付きのオブジェクトの基本クラス
class Object {
private:
	// 参照カウンタ
	unsigned int refCount;
protected:
	// デフォルトコンストラクタ
	Object():refCount(0) {
	}
	// デストラクタ
	// protectedなので派生クラスでデストラクタをpublicにしない限り
	// 自動変数としては生成できない
	virtual ~Object() {
	}
	// 内部的な参照の数と実際の参照カウンタが等しいかどうかを判定する
	bool existsOuterReference(unsigned int innerRefCount)const {
		return refCount != innerRefCount;
	}
public:
	// 参照カウンタを増やす
	virtual int refer() {
		return ++refCount;
	}
	// 参照カウンタを減らし、0になったらdeleteする
	virtual int release() {
		if (--refCount != 0) {
			return refCount;
		}else{
			delete this;
			return 0;
		}
	}
	static Object* set(Object* value, Object* ptr) {
		if (value != ptr) {
			if (value != NULL)
				value->release();
			value = ptr;
			if (value != NULL)
				value->refer();
		}
		return value;
	}
};

}

#endif//_YCL_OBJECT_H_
