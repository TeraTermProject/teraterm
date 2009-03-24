/*
 * $Id: EditBoxCtrl.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_EDITBOXCTRL_H_
#define _YCL_EDITBOXCTRL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/Window.h>

namespace yebisuya {

class EditBoxCtrl : virtual public Window {
public:
	void limitText(int limit) {
		SendMessage(EM_LIMITTEXT, limit);
	}
	int getLineCount()const {
		return SendMessage(EM_GETLINECOUNT);
	}
	int getLine(int line, char* buffer, int length)const {
		*((int*) buffer) = length;
		return SendMessage(EM_GETLINE, line, (LPARAM) buffer);
	}
	String getLine(int line)const {
		int length = lineLength(line);
		char* buffer = (char*) alloca(length + 1);
		getLine(line, buffer, length + 1);
		return buffer;
	}
	void getRect(RECT& rect)const {
		SendMessage(EM_GETRECT, 0, (LPARAM) &rect);
	}
	void setRect(const RECT& rect) {
		SendMessage(EM_SETRECT, 0, (LPARAM) &rect);
	}
	void setRectNoPaint(const RECT& rect) {
		SendMessage(EM_SETRECTNP, 0, (LPARAM) &rect);
	}
	long getSel()const {
		return SendMessage(EM_GETSEL);
	}
	bool getSel(int& start, int& end)const {
		return SendMessage(EM_GETSEL, (WPARAM) &start, (LPARAM) &end) != -1;
	}
	void setSel(int start, int end) {
		SendMessage(EM_SETSEL, start, end);
	}
	void replaceSel(const char* string) {
		SendMessage(EM_REPLACESEL, 0, (LPARAM) string);
	}
	bool getModify()const {
		return SendMessage(EM_GETMODIFY) != FALSE;
	}
	void setModify(bool modified) {
		SendMessage(EM_SETMODIFY, modified);
	}
	bool scrollCaret() {
		return SendMessage(EM_SCROLLCARET) != FALSE;
	}
	int lineFromChar(int index) {
		return SendMessage(EM_LINEFROMCHAR, index);
	}
	int lineIndex(int line) {
		return SendMessage(EM_LINEINDEX, line);
	}
	int lineLength(int line)const {
		return SendMessage(EM_LINELENGTH, line);
	}
	void scroll(int cx, int cy) {
		SendMessage(EM_LINESCROLL, cx, cy);
	}
	bool canUndo()const {
		return SendMessage(EM_CANUNDO) != FALSE;
	}
	bool undo() {
		return SendMessage(EM_UNDO) != FALSE;
	}
	void emptyUndoBuffer() {
		SendMessage(EM_EMPTYUNDOBUFFER);
	}
	void setPasswordChar(int ch) {
		SendMessage(EM_SETPASSWORDCHAR, ch);
	}
	void setTabStops(int count, const int tabs[]) {
		SendMessage(EM_SETTABSTOPS, count, (LPARAM) tabs);
	}
	bool fmtLines(bool addEOL) {
		return SendMessage(EM_FMTLINES, addEOL) != FALSE;
	}
	HLOCAL getHandle()const {
		return (HLOCAL) SendMessage(EM_GETHANDLE);
	}
	void setHandle(HLOCAL buffer) {
		SendMessage(EM_SETHANDLE, (WPARAM) buffer);
	}
	int getFirstVisibleLine()const {
		return SendMessage(EM_GETFIRSTVISIBLELINE);
	}
	bool setReadOnly(bool readOnly) {
		return SendMessage(EM_SETREADONLY, readOnly) != FALSE;
	}
	int getPasswordChar()const {
		return (TCHAR) SendMessage(EM_GETPASSWORDCHAR);
	}
	void setWordBreakProc(EDITWORDBREAKPROC wordBreakProc) {
		SendMessage(EM_SETWORDBREAKPROC, 0, (LPARAM) wordBreakProc);
	}
	EDITWORDBREAKPROC getWordBreakProc()const {
		return (EDITWORDBREAKPROC) SendMessage(EM_GETWORDBREAKPROC);
	}
	void charFromPos(POINT point, int& charIndex, int& line)const {
		long result = SendMessage(EM_CHARFROMPOS, 0, MAKELONG(point.x, point.y));
		charIndex = LOWORD(result);
		line = HIWORD(result);
	}
	void posFromChar(int charIndex, POINT& point)const {
		long result = SendMessage(EM_POSFROMCHAR, charIndex);
		point.x = LOWORD(result);
		point.y = HIWORD(result);
	}
	bool lineScroll(int cx, int cy) {
		return SendMessage(EM_LINESCROLL, cx, cy) != FALSE;
	}
	long getThumb()const {
		return SendMessage(EM_GETTHUMB);
	}
	int getLimitText()const {
		return SendMessage(EM_GETLIMITTEXT);
	}
	void setLimitText(int limit) {
		SendMessage(EM_SETLIMITTEXT, limit);
	}
	void getMargins(int& leftMargin, int& rightMargin)const {
		long result = SendMessage(EM_GETMARGINS);
		leftMargin = LOWORD(result);
		rightMargin = HIWORD(result);
	}
	void setMargins(int flags, int leftMargin, int rightMargin) {
		SendMessage(EM_SETMARGINS, flags, MAKELONG(leftMargin, rightMargin));
	}
#ifndef EM_GETIMESTATUS
#define EM_GETIMESTATUS 0x00D9
#endif
	long getImeStatus(int type)const {
		return SendMessage(EM_GETIMESTATUS, type);
	}
#ifndef EM_SETIMESTATUS
#define EM_SETIMESTATUS 0x00D8
#endif
	long setImeStatus(int type, long status) {
		return SendMessage(EM_SETIMESTATUS, type, status);
	}
};

}

#endif//_YCL_EDITBOXCTRL_H_
