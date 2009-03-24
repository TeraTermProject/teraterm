/*
 * $Id: DynamicLinkLibrary.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_DYNAMICLINKLIBRARY_H_
#define _YCL_DYNAMICLINKLIBRARY_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace yebisuya {

template<class IMPLEMENT>
class DynamicLinkLibrary {
protected:
	typedef DynamicLinkLibrary<IMPLEMENT> super;

	DynamicLinkLibrary() {
	}
public:
	static IMPLEMENT& getInstance() {
		static IMPLEMENT instance;
		return instance;
	}
	bool processAttach() {
		return true;
	}
	bool processDetach() {
		return true;
	}
	bool threadAttach() {
		return true;
	}
	bool threadDetach() {
		return true;
	}
};

}

#endif//_YCL_DYNAMICLINKLIBRARY_H_
