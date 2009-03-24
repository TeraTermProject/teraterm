/*
 * $Id: ValueCtrl.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_VALUECTRL_H_
#define _YCL_VALUECTRL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace yebisuya {

template<class TYPE>
class ValueCtrl {
public:
	static void initialize(TYPE& value) {
		value = 0;
	}
	static bool isEmpty(const TYPE& value) {
		return value == NULL;
	}
	static int compare(const TYPE& a, const TYPE& b) {
		if (a > b)
			return 1;
		if (a < b)
			return -1;
		return 0;
	}
};

}

#endif//_YCL_VALUECTRL_H_
