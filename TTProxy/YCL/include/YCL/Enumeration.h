/*
 * $Id: Enumeration.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_ENUMERATION_H_
#define _YCL_ENUMERATION_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/Object.h>

namespace yebisuya {

// オブジェクトを列挙するためのクラス。
template<class TYPE>
class Enumeration : public Object {
public:
	// まだ列挙するオブジェクトが存在しているかどうかを判定する。
	// 返値:
	//	次のオブジェクトが存在していれば真。
	virtual bool hasMoreElements()const = 0;
	// 次に現れるオブジェクトを返す。
	// 返値:
	//	次のオブジェクト。
	virtual TYPE nextElement()const = 0;
};

}

#endif//_YCL_ENUMERATION_H_
