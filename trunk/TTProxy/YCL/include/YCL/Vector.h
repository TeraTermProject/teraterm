/*
 * $Id: Vector.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_VECTOR_H_
#define _YCL_VECTOR_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/ValueCtrl.h>
#include <YCL/Pointer.h>
#include <YCL/Enumeration.h>
#include <YCL/Array.h>

namespace yebisuya {

template<class TYPE, class CTRL = ValueCtrl<TYPE> >
class Vector {
private:
	enum {
		BOUNDARY = 8,
	};
	class EnumElements : public Enumeration<TYPE> {
	private:
		TYPE* array;
		int size;
		mutable int index;
	public:
		EnumElements(TYPE* array, int size):array(array), size(size), index(0) {
		}
		virtual bool hasMoreElements()const {
			return index < size;
		}
		virtual TYPE nextElement()const {
			return array[index++];
		}
	};

	TYPE* array;
	int arraySize;
	int arrayLength;

	Vector(Vector<TYPE,CTRL>&);
	void operator=(Vector<TYPE,CTRL>&);
public:
	Vector():array(NULL), arraySize(0), arrayLength(0) {
	}
	Vector(int size):array(NULL), arraySize(0), arrayLength(0) {
		setSize(size);
	}
	~Vector() {
		removeAll();
	}
	int size()const {
		return arraySize;
	}
	TYPE get(int index)const {
		TYPE result;
		if (0 <= index && index < arraySize) {
			result = array[index];
		}else{
			CTRL::initialize(result);
		}
		return result;
	}

	void setSize(int newSize) {
		if (newSize != arraySize) {
			if (newSize > arrayLength) {
				int newLength = (newSize + 7) & ~7;
				TYPE* newArray = new TYPE[newLength];
				int i;
				for (i = 0; i < arraySize; i++) {
					newArray[i] = array[i];
				}
				for (; i < newSize; i++) {
					CTRL::initialize(newArray[i]);
				}
				arrayLength = newLength;
				delete[] array;
				array = newArray;
			}
			for (int i = newSize; i < arraySize && i < arrayLength; i++) {
				CTRL::initialize(array[i]);
			}
			arraySize = newSize;
		}
	}
	void set(int index, const TYPE& value) {
		if (0 <= index && index < arraySize) {
			array[index] = value;
		}
	}
	void add(const TYPE& value) {
		add(arraySize, value);
	}
	void add(int index, const TYPE& value) {
		if (index < 0)
			index = 0;
		if (index > arraySize)
			index = arraySize;
		setSize(arraySize + 1);
		for (int i = arraySize - 1; i > index; i--) {
			array[i] = array[i - 1];
		}
		array[index] = value;
	}
	Pointer<Enumeration<TYPE> > elements()const {
		return new EnumElements(array, arraySize);
	}
	int indexOf(const TYPE& value)const {
		return indexOf(value, 0);
	}
	int indexOf(const TYPE& value, int index)const {
		for (int i = index; i < arraySize; i++) {
			if (array[i] == value)
				return i;
		}
		return -1;
	}
	int lastIndexOf(const TYPE& value)const {
		return indexOf(value, arraySize - 1);
	}
	int lastIndexOf(const TYPE& value, int index)const {
		for (int i = index; i >= 0; i--) {
			if (array[i] == value)
				return i;
		}
		return -1;
	}
	bool isEmpty()const {
		return arraySize == 0;
	}
	TYPE firstElement()const {
		return get(0);
	}
	TYPE lastElement()const {
		return get(arraySize - 1);
	}
	TYPE remove(int index) {
		TYPE removed = get(index);
		for (int i = index; i < arraySize - 1; i++) {
			array[i] = array[i + 1];
		}
		setSize(arraySize - 1);
		return removed;
	}
	bool remove(const TYPE& value) {
		int index = indexOf(value);
		if (index < 0)
			return false;
		remove(index);
		return true;
	}
	void removeAll() {
		delete[] array;
		array = NULL;
		arraySize = 0;
		arrayLength = 0;
	}
	PointerArray<TYPE> toArray()const {
		return toArray(new Array<TYPE>(arraySize));
	}
	PointerArray<TYPE> toArray(PointerArray<TYPE> copyto)const {
		int i;
		for (i = 0; i < (int) copyto->length && i < arraySize; i++) {
			copyto[i] = array[i];
		}
		for (; i < (int) copyto->length; i++) {
			CTRL::initialize(copyto[i]);
		}
		return copyto;
	}
	void trimToSize() {
		TYPE* newArray = array;
		if (arraySize == 0) {
			newArray = NULL;
		}else if (arraySize < arrayLength) {
			newArray = new TYPE[arraySize];
			for (int i = 0; i < arraySize; i++) {
				newArray[i] = array[i];
			}
			arrayLength = arraySize;
		}
		if (newArray != array) {
			delete[] array;
			array = newArray;
		}
	}
	TYPE operator[](int index)const {
		return get(index);
	}
};

}

#endif//_YCL_VECTOR_H_
