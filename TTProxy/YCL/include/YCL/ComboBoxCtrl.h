/*
 * $Id: ComboBoxCtrl.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_COMBOBOXCTRL_H_
#define _YCL_COMBOBOXCTRL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/Window.h>

namespace yebisuya {

class ComboBoxCtrl : virtual public Window {
public:
	int limitText(int limit) {
		return SendMessage(CB_LIMITTEXT, limit);
	}
	int getEditSel()const {
		return SendMessage(CB_GETEDITSEL);
	}
	int setEditSel(int start, int end) {
		return SendMessage(CB_SETEDITSEL, 0, MAKELPARAM(start, end));
	}
	int getCount()const {
		return SendMessage(CB_GETCOUNT);
	}
	int resetContent() {
		return SendMessage(CB_RESETCONTENT);
	}
	int addString(const char* string) {
		return SendMessage(CB_ADDSTRING, 0, (LPARAM) string);
	}
	int addItemData(long data) {
		return SendMessage(CB_ADDSTRING, 0, data);
	}
	int insertString(int index, const char* string) {
		return SendMessage(CB_INSERTSTRING, index, (LPARAM) string);
	}
	int insertItemData(int index, long data) {
		return SendMessage(CB_INSERTSTRING, index, data);
	}
	int deleteString(int index) {
		return SendMessage(CB_DELETESTRING, index);
	}
	int getTextLen(int index)const {
		return SendMessage(CB_GETLBTEXTLEN, index);
	}
	int getText(int index, char* buffer)const {
		return SendMessage(CB_GETLBTEXT, index, (LPARAM) buffer);
	}
	String getText(int index) {
		int length = getTextLen(index);
		char* buffer = (char*) alloca(length + 1);
		getText(index, buffer);
		return buffer;
	}
	long getItemData(int index)const {
		return SendMessage(CB_GETITEMDATA, index);
	}
	int setItemData(int index, long data) {
		return SendMessage(CB_SETITEMDATA, index, data);
	}
	int findString(const char* string, int startIndex = -1)const {
		return SendMessage(CB_FINDSTRING, startIndex, (LPARAM) string);
	}
	int findItemData(long data, int startIndex = -1)const {
		return SendMessage(CB_FINDSTRING, startIndex, data);
	}
	int getCurSel()const {
		return SendMessage(CB_GETCURSEL);
	}
	int setCurSel(int index) {
		return SendMessage(CB_SETCURSEL, index);
	}
	int selectString(const char* string, int startIndex = -1) {
		return SendMessage(CB_SELECTSTRING, startIndex, (LPARAM) string);
	}
	int selectItemData(long data, int startIndex = -1) {
		return SendMessage(CB_SELECTSTRING, startIndex, data);
	}
	int dir(int attributes, const char* path) {
		return SendMessage(CB_DIR, attributes, (LPARAM) path);
	}
	bool showDropdown(bool show) {
		return SendMessage(CB_SHOWDROPDOWN, show) != FALSE;
	}
	int findStringExact(const char* string, int startIndex = -1)const {
		return SendMessage(CB_FINDSTRINGEXACT, startIndex, (LPARAM) string);
	}
	int findItemDataExact(long data, int startIndex = -1)const {
		return SendMessage(CB_FINDSTRINGEXACT, startIndex, data);
	}
	bool getDroppedState()const {
		return SendMessage(CB_GETDROPPEDSTATE) != FALSE;
	}
	void getDroppedControlRect(RECT& rect)const {
		SendMessage(CB_GETDROPPEDCONTROLRECT, 0, (LPARAM) &rect);
	}
	int getItemHeight()const {
		return SendMessage(CB_GETITEMHEIGHT);
	}
	int setItemHeight(int index, int cy) {
		return SendMessage(CB_SETITEMHEIGHT, index, cy);
	}
	int getExtendedUI()const {
		return SendMessage(CB_GETEXTENDEDUI);
	}
	int setExtendedUI(int flags) {
		return SendMessage(CB_SETEXTENDEDUI, flags);
	}
	int getTopIndex()const {
		return SendMessage(CB_GETTOPINDEX);
	}
	int setTopIndex(int index) {
		return SendMessage(CB_SETTOPINDEX, index);
	}
	int getHorizontalExtent()const {
		return SendMessage(CB_GETHORIZONTALEXTENT);
	}
	void setHorizontalExtent(int cx) {
		SendMessage(CB_SETHORIZONTALEXTENT, cx);
	}
	int getDroppedWidth()const {
		return SendMessage(CB_GETDROPPEDWIDTH);
	}
	int setDroppedWidth(int cx) {
		return SendMessage(CB_SETDROPPEDWIDTH, cx);
	}
	int initStorage(int count, long size) {
		return SendMessage(CB_INITSTORAGE, count, size);
	}
};

}

#endif//_YCL_COMBOBOXCTRL_H_
