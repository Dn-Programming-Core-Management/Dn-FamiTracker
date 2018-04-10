#include "stdafx.h"
#include "NoNotifyEdit.h"
// Credit: http://www.flounder.com/avoid_en_change.htm
// and http://read.pudn.com/downloads190/sourcecode/windows/other/893682/mvp_tips/fp/NoNotifyEdit.cpp__.htm
// and http://read.pudn.com/downloads190/sourcecode/windows/other/893682/mvp_tips/fp/NoNotifyEdit.h__.htm
// https://www.codeproject.com/Articles/480/Create-your-own-controls-the-art-of-subclassing
// http://forums.codeguru.com/showthread.php?234616-Subclassing-vs-Inheriting-in-MFC&p=692308#post692308

// https://stackoverflow.com/questions/8428288/using-derived-class-from-cedit-in-my-dialog

IMPLEMENT_DYNAMIC(NoNotifyEdit, CEdit)

NoNotifyEdit::NoNotifyEdit() {
	notify = true;
}

NoNotifyEdit::~NoNotifyEdit() {
}

BEGIN_MESSAGE_MAP(NoNotifyEdit, CEdit)
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()

void NoNotifyEdit::SetWindowTextNoNotify(LPCTSTR s) {
	CString old;
	CEdit::GetWindowText(old);
	if (old == s)
		return; // do nothing, already set
	bool previous = notify;
	notify = false;
	CEdit::SetWindowText(s);
	notify = previous;
}

afx_msg BOOL NoNotifyEdit::OnEnChange() {
	return !notify;
}