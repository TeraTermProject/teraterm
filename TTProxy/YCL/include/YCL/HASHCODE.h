/*
 * $Id: HASHCODE.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */
#ifndef _YCL_HASHCODE_H_
#define _YCL_HASHCODE_H_

#pragma once

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
	HASHCODE(const void* value):value((int)(uintptr_t) value) {
	}
	HASHCODE(const FARPROC value) :value((int)(uintptr_t)value) {
	}
	HASHCODE(SOCKET value) :value((int)(uintptr_t)value) {
	}
	HASHCODE(int value):value(value) {
	}
	operator int()const {
		return value;
	}
};

}

#endif//_YCL_HASHCODE_H_
