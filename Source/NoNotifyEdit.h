#pragma once

class NoNotifyEdit : public CEdit {
	DECLARE_DYNAMIC(NoNotifyEdit)

public:
	NoNotifyEdit();
	virtual ~NoNotifyEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	void SetWindowTextNoNotify(LPCTSTR s);
protected:
	bool notify;
	afx_msg BOOL OnEnChange();
};