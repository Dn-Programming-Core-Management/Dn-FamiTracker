#pragma once
#include "stdafx.h"

class NoNotifyEdit : public CEdit {
	DECLARE_DYNAMIC(NoNotifyEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	//NoNotifyEdit();
	void SetWindowTextNoNotify(LPCTSTR s);
protected:
	bool notify;
	afx_msg BOOL OnEnChange();
};