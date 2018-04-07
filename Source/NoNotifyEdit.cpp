#include "NoNotifyEdit.h"
// Credit: http://www.flounder.com/avoid_en_change.htm

IMPLEMENT_DYNAMIC(NoNotifyEdit, CEdit)

BEGIN_MESSAGE_MAP(NoNotifyEdit, CEdit)
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()

void NoNotifyEdit::SetWindowTextNoNotify(LPCTSTR s)
{
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