/*
 * $Id: HASHCODE.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */
#ifndef _YCL_HASHCODE_H_
#define _YCL_HASHCODE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace yebisuya {

class HASHCODE {
private:
	static int generateHashCode(const char* value) {
		int h = 0;
		while (*value != '\0')
			h = h * 31 + *value++;
		return h;
	}

	int value;
public:
	HASHCODE(const char* value):value(generateHashCode(value)) {
	}
	HASHCODE(const HASHCODE& code):value(code.value) {
	}
	HASHCODE(void* value):value((int) value) {
	}
	HASHCODE(int value):value(value) {
	}
	operator int()const {
		return value;
	}
};

}

#endif//_YCL_HASHCODE_H_
