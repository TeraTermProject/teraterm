/*
 * $Id: Dialog.h,v 1.6 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_DIALOG_H_
#define _YCL_DIALOG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/Window.h>
#include <YCL/Hashtable.h>

namespace yebisuya {

class Dialog : virtual public Window {
protected:
	typedef Hashtable<HWND, Dialog*> Map;
	static Map& getMap() {
		static Map map;
		return map;
	}
	static Dialog* prepareOpen(Dialog* next) {
		static Dialog* initializeing = NULL;
		Dialog* prev = initializeing;
		initializeing = next;
		return prev;
	}
	static BOOL CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
		Map& map = getMap();
		Dialog* target = map.get(dialog);
		if (target == NULL) {
			target = prepareOpen(NULL);
			if (target != NULL) {
				*target <<= dialog;
				map.put(dialog, target);
			}
		}
		BOOL result = target != NULL ? target->dispatch(message, wParam, lParam) : FALSE;
		if (message == WM_NCDESTROY) {
			map.remove(dialog);
			if (target != NULL)
				*target <<= NULL;
		}
		if (result && (message == WM_CTLCOLORBTN
				|| message == WM_CTLCOLORDLG
				|| message == WM_CTLCOLOREDIT
				|| message == WM_CTLCOLORLISTBOX
				|| message == WM_CTLCOLORSCROLLBAR
				|| message == WM_CTLCOLORSTATIC)) {
			result = (BOOL) ::GetWindowLong(dialog, DWL_MSGRESULT);
		}
		return result;
	}

public:
	bool SetDlgItemInt(int id, int value, bool valueSigned) {
		return ::SetDlgItemInt(*this, id, value, valueSigned) != FALSE;
	}
	int GetDlgItemInt(int id, int defaultValue, bool valueSigned) {
		BOOL succeeded;
		int value = ::GetDlgItemInt(*this, id, &succeeded, valueSigned);
		return succeeded ? value : defaultValue;
	}
	bool SetDlgItemText(int id, const char* text) {
		return ::SetDlgItemText(*this, id, text) != FALSE;
	}
	int GetDlgItemText(int id, char* buffer, size_t length)const {
		return ::GetDlgItemText(*this, id, buffer, length);
	}
	String GetDlgItemText(int id)const {
		return Window(GetDlgItem(id)).GetWindowText();
	}
	LRESULT SendDlgItemMessage(int id, int message, int wparam = 0, long lparam = 0)const {
		return ::SendDlgItemMessage(*this, id, message, wparam, lparam);
	}
	bool EndDialog(int result) {
		return ::EndDialog(*this, result) != FALSE;
	}
	bool MapDialogRect(RECT& rect)const {
		return ::MapDialogRect(*this, &rect) != FALSE;
	}

	void setResult(LRESULT result) {
		SetWindowLong(DWL_MSGRESULT, result);
	}
	int getDefID()const {
		return LOWORD(SendMessage(DM_GETDEFID));
	}
	void setDefID(int controlId) {
		SendMessage(DM_SETDEFID, controlId);
	}
	void reposition() {
		SendMessage(DM_REPOSITION);
	}
	void nextDlgCtrl(bool previous = false)const {
		SendMessage(WM_NEXTDLGCTL, previous, FALSE);
	}
	void gotoDlgCtrl(HWND control) {
		SendMessage(WM_NEXTDLGCTL, (WPARAM) control, TRUE); 
	}


	int open(int resourceId, HWND owner = NULL) {
		return open(GetInstanceHandle(), resourceId, owner);
	}
	int open(HINSTANCE instance, int resourceId, HWND owner = NULL) {
		YCLVERIFY(prepareOpen(this) == NULL, "Another dialog has been opening yet.");
		return ::DialogBoxParam(instance, MAKEINTRESOURCE(resourceId), owner, DialogProc, NULL);
	}
protected:
	virtual bool dispatch(int message, int wparam, long lparam) {
		switch (message) {
		case WM_INITDIALOG:
			return onInitDialog();
		case WM_COMMAND:
			switch (wparam) {
			case MAKEWPARAM(IDOK, BN_CLICKED):
				onOK();
				return true;
			case MAKEWPARAM(IDCANCEL, BN_CLICKED):
				onCancel();
				return true;
			}
			break;
		}
		return false;
	}
	virtual bool onInitDialog() {
		return true;
	}
	virtual void onOK() {
		EndDialog(IDOK);
	}
	virtual void onCancel() {
		EndDialog(IDCANCEL);
	}
};

}

#endif//_YCL_DIALOG_H_
