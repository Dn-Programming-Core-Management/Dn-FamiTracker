/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <iterator> 
#include <string>
#include <sstream>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorVRC7.h"
#include "Clipboard.h"

static unsigned char default_inst[(16+3)*16] = 
{
#include "apu/vrc7tone.h" 
};

// CInstrumentSettingsVRC7 dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorVRC7, CInstrumentEditPanel)

CInstrumentEditorVRC7::CInstrumentEditorVRC7(CWnd* pParent /*=NULL*/)
	: CInstrumentEditPanel(CInstrumentEditorVRC7::IDD, pParent)
{
	m_pInstrument = NULL;
}

CInstrumentEditorVRC7::~CInstrumentEditorVRC7()
{
	if (m_pInstrument)
		m_pInstrument->Release();
}

void CInstrumentEditorVRC7::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditorVRC7, CInstrumentEditPanel)
	ON_CBN_SELCHANGE(IDC_PATCH, OnCbnSelchangePatch)
	ON_BN_CLICKED(IDC_M_AM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_VIB, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_EG, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_KSR2, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_DM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_AM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_VIB, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_EG, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_KSR, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_DM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDC_COPY, &CInstrumentEditorVRC7::OnCopy)
	ON_COMMAND(IDC_PASTE, &CInstrumentEditorVRC7::OnPaste)
END_MESSAGE_MAP()


// CInstrumentSettingsVRC7 message handlers

BOOL CInstrumentEditorVRC7::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pPatchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PATCH));
	CString Text;

	const _TCHAR* const PATCH_NAME[16] = {
		_T("(custom patch)"),
		_T("Bell"),
		_T("Guitar"),
		_T("Piano"),
		_T("Flute"),
		_T("Clarinet"),
		_T("Rattling Bell"),
		_T("Trumpet"),
		_T("Reed Organ"),
		_T("Soft Bell"),
		_T("Xylophone"),
		_T("Vibraphone"),
		_T("Brass"),
		_T("Bass Guitar"),
		_T("Synthesizer"),
		_T("Chorus")
	};

	for (int i = 0; i < 16; ++i) {
		Text.Format(_T("Patch #%i - %s"), i, PATCH_NAME[i]);
		pPatchBox->AddString(Text);
	}

	pPatchBox->SetCurSel(0);

	SetupSlider(IDC_M_MUL, 15);
	SetupSlider(IDC_C_MUL, 15);
	SetupSlider(IDC_M_KSL, 3);
	SetupSlider(IDC_C_KSL, 3);
	SetupSlider(IDC_TL, 63);
	SetupSlider(IDC_FB, 7);
	SetupSlider(IDC_M_AR, 15);
	SetupSlider(IDC_M_DR, 15);
	SetupSlider(IDC_M_SL, 15);
	SetupSlider(IDC_M_RR, 15);
	SetupSlider(IDC_C_AR, 15);
	SetupSlider(IDC_C_DR, 15);
	SetupSlider(IDC_C_SL, 15);
	SetupSlider(IDC_C_RR, 15);

	EnableControls(true);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorVRC7::OnCbnSelchangePatch()
{
	CComboBox *pPatchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PATCH));
	SelectPatch(pPatchBox->GetCurSel());
}

void CInstrumentEditorVRC7::SelectPatch(int Patch)
{
	m_pInstrument->SetPatch(Patch);
	EnableControls(Patch == 0);

	if (Patch == 0)
		LoadCustomPatch();
	else
		LoadInternalPatch(Patch);
}

void CInstrumentEditorVRC7::EnableControls(bool bEnable)
{
	const int SLIDER_IDS[] = {
		IDC_M_AM, IDC_M_AR, 
		IDC_M_DM, IDC_M_DR, 
		IDC_M_EG, IDC_M_KSL, 
		IDC_M_KSR2, IDC_M_MUL, 
		IDC_M_RR, IDC_M_SL, 
		IDC_M_SL, IDC_M_VIB,
		IDC_C_AM, IDC_C_AR, 
		IDC_C_DM, IDC_C_DR, 
		IDC_C_EG, IDC_C_KSL, 
		IDC_C_KSR, IDC_C_MUL, 
		IDC_C_RR, IDC_C_SL, 
		IDC_C_SL, IDC_C_VIB,
		IDC_TL, IDC_FB
	};

	const int SLIDERS = sizeof(SLIDER_IDS) / sizeof(SLIDER_IDS[0]);

	for (int i = 0; i < SLIDERS; ++i)
		GetDlgItem(SLIDER_IDS[i])->EnableWindow(bEnable ? TRUE : FALSE);
}

void CInstrumentEditorVRC7::SelectInstrument(int Instrument)
{
	CComboBox *pPatchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PATCH));

	if (m_pInstrument)
		m_pInstrument->Release();

	m_pInstrument = static_cast<CInstrumentVRC7*>(GetDocument()->GetInstrument(Instrument));
	ASSERT(m_pInstrument->GetType() == INST_VRC7);

	int Patch = m_pInstrument->GetPatch();

	pPatchBox->SetCurSel(Patch);
	
	if (Patch == 0)
		LoadCustomPatch();
	else
		LoadInternalPatch(Patch);
	
	EnableControls(Patch == 0);
}

BOOL CInstrumentEditorVRC7::OnEraseBkgnd(CDC* pDC)
{
	return CDialog::OnEraseBkgnd(pDC);
}

HBRUSH CInstrumentEditorVRC7::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CInstrumentEditorVRC7::SetupSlider(int Slider, int Max)
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(Slider));
	pSlider->SetRangeMax(Max);
}

int CInstrumentEditorVRC7::GetSliderVal(int Slider)
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(Slider));
	return pSlider->GetPos();
}

void CInstrumentEditorVRC7::SetSliderVal(int Slider, int Value)
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(Slider));
	pSlider->SetPos(Value);
}

void CInstrumentEditorVRC7::LoadInternalPatch(int Num)
{
	unsigned int Reg;

	GetDlgItem(IDC_PASTE)->EnableWindow(FALSE);

	// Register 0
	Reg = default_inst[(Num * 16) + 0];
	CheckDlgButton(IDC_M_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_M_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_M_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_M_KSR2, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_M_MUL, Reg & 0x0F);

	// Register 1
	Reg = default_inst[(Num * 16) + 1];
	CheckDlgButton(IDC_C_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_C_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_C_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_C_KSR, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_C_MUL, Reg & 0x0F);

	// Register 2
	Reg = default_inst[(Num * 16) + 2];
	SetSliderVal(IDC_M_KSL, Reg >> 6);
	SetSliderVal(IDC_TL, Reg & 0x3F);

	// Register 3
	Reg = default_inst[(Num * 16) + 3];
	SetSliderVal(IDC_C_KSL, Reg >> 6);
	SetSliderVal(IDC_FB, 7 - (Reg & 7));
	CheckDlgButton(IDC_C_DM, Reg & 0x10 ? 1 : 0);
	CheckDlgButton(IDC_M_DM, Reg & 0x08 ? 1 : 0);

	// Register 4
	Reg = default_inst[(Num * 16) + 4];
	SetSliderVal(IDC_M_AR, Reg >> 4);
	SetSliderVal(IDC_M_DR, Reg & 0x0F);

	// Register 5
	Reg = default_inst[(Num * 16) + 5];
	SetSliderVal(IDC_C_AR, Reg >> 4);
	SetSliderVal(IDC_C_DR, Reg & 0x0F);

	// Register 6
	Reg = default_inst[(Num * 16) + 6];
	SetSliderVal(IDC_M_SL, Reg >> 4);
	SetSliderVal(IDC_M_RR, Reg & 0x0F);

	// Register 7
	Reg = default_inst[(Num * 16) + 7];
	SetSliderVal(IDC_C_SL, Reg >> 4);
	SetSliderVal(IDC_C_RR, Reg & 0x0F);
}

void CInstrumentEditorVRC7::LoadCustomPatch()
{
	unsigned char Reg;

	GetDlgItem(IDC_PASTE)->EnableWindow(TRUE);

	// Register 0
	Reg = m_pInstrument->GetCustomReg(0);
	CheckDlgButton(IDC_M_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_M_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_M_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_M_KSR2, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_M_MUL, Reg & 0x0F);

	// Register 1
	Reg = m_pInstrument->GetCustomReg(1);
	CheckDlgButton(IDC_C_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_C_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_C_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_C_KSR, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_C_MUL, Reg & 0x0F);

	// Register 2
	Reg = m_pInstrument->GetCustomReg(2);
	SetSliderVal(IDC_M_KSL, Reg >> 6);
	SetSliderVal(IDC_TL, Reg & 0x3F);

	// Register 3
	Reg = m_pInstrument->GetCustomReg(3);
	SetSliderVal(IDC_C_KSL, Reg >> 6);
	SetSliderVal(IDC_FB, 7 - (Reg & 7));
	CheckDlgButton(IDC_C_DM, Reg & 0x10 ? 1 : 0);
	CheckDlgButton(IDC_M_DM, Reg & 0x08 ? 1 : 0);

	// Register 4
	Reg = m_pInstrument->GetCustomReg(4);
	SetSliderVal(IDC_M_AR, Reg >> 4);
	SetSliderVal(IDC_M_DR, Reg & 0x0F);

	// Register 5
	Reg = m_pInstrument->GetCustomReg(5);
	SetSliderVal(IDC_C_AR, Reg >> 4);
	SetSliderVal(IDC_C_DR, Reg & 0x0F);

	// Register 6
	Reg = m_pInstrument->GetCustomReg(6);
	SetSliderVal(IDC_M_SL, Reg >> 4);
	SetSliderVal(IDC_M_RR, Reg & 0x0F);

	// Register 7
	Reg = m_pInstrument->GetCustomReg(7);
	SetSliderVal(IDC_C_SL, Reg >> 4);
	SetSliderVal(IDC_C_RR, Reg & 0x0F);
}

void CInstrumentEditorVRC7::SaveCustomPatch()
{
	unsigned char Reg;

	// Register 0
	Reg  = (IsDlgButtonChecked(IDC_M_AM) ? 0x80 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_VIB) ? 0x40 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_EG) ? 0x20 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_KSR2) ? 0x10 : 0);
	Reg |= GetSliderVal(IDC_M_MUL);
	m_pInstrument->SetCustomReg(0, Reg);

	// Register 1
	Reg  = (IsDlgButtonChecked(IDC_C_AM) ? 0x80 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_VIB) ? 0x40 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_EG) ? 0x20 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_KSR) ? 0x10 : 0);
	Reg |= GetSliderVal(IDC_C_MUL);
	m_pInstrument->SetCustomReg(1, Reg);

	// Register 2
	Reg  = GetSliderVal(IDC_M_KSL) << 6;
	Reg |= GetSliderVal(IDC_TL);
	m_pInstrument->SetCustomReg(2, Reg);

	// Register 3
	Reg  = GetSliderVal(IDC_C_KSL) << 6;
	Reg |= IsDlgButtonChecked(IDC_C_DM) ? 0x10 : 0;
	Reg |= IsDlgButtonChecked(IDC_M_DM) ? 0x08 : 0;
	Reg |= 7 - GetSliderVal(IDC_FB);
	m_pInstrument->SetCustomReg(3, Reg);

	// Register 4
	Reg = GetSliderVal(IDC_M_AR) << 4;
	Reg |= GetSliderVal(IDC_M_DR);
	m_pInstrument->SetCustomReg(4, Reg);

	// Register 5
	Reg = GetSliderVal(IDC_C_AR) << 4;
	Reg |= GetSliderVal(IDC_C_DR);
	m_pInstrument->SetCustomReg(5, Reg);

	// Register 6
	Reg = GetSliderVal(IDC_M_SL) << 4;
	Reg |= GetSliderVal(IDC_M_RR);
	m_pInstrument->SetCustomReg(6, Reg);

	// Register 7
	Reg = GetSliderVal(IDC_C_SL) << 4;
	Reg |= GetSliderVal(IDC_C_RR);
	m_pInstrument->SetCustomReg(7, Reg);	
}

void CInstrumentEditorVRC7::OnBnClickedCheckbox()
{
	SaveCustomPatch();
	SetFocus();
}

void CInstrumentEditorVRC7::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SaveCustomPatch();
	SetFocus();
	CInstrumentEditPanel::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CInstrumentEditorVRC7::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SaveCustomPatch();
	SetFocus();
	CInstrumentEditPanel::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CInstrumentEditorVRC7::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, 1, _T("&Copy"));
	menu.AppendMenu(MF_STRING, 2, _T("Copy as Plain &Text"));		// // //
	menu.AppendMenu(MF_STRING, 3, _T("&Paste"));

	switch (menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this)) {
		case 1: // Copy
			OnCopy();
			break;
		case 2: // // //
			CopyAsPlainText();
			break;
		case 3: // Paste
			OnPaste();
			break;
	}
}

void CInstrumentEditorVRC7::OnCopy()
{
	CString MML;

	int patch = m_pInstrument->GetPatch();
	// Assemble a MML string
	for (int i = 0; i < 8; ++i)
		MML.AppendFormat(_T("$%02X "), (patch == 0) ? (unsigned char)(m_pInstrument->GetCustomReg(i)) : default_inst[patch * 16 + i]);
	
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer(MML.GetBuffer(), MML.GetLength() + 1);
}

void CInstrumentEditorVRC7::CopyAsPlainText()		// // //
{
	int patch = m_pInstrument->GetPatch();
	unsigned char reg[8] = {};
	for (int i = 0; i < 8; ++i)
		reg[i] = patch == 0 ? m_pInstrument->GetCustomReg(i) : default_inst[patch * 16 + i];
	
	CString MML;
	GetDlgItemTextA(IDC_PATCH, MML);
	MML.Format(_T(";%s\r\n;%s\r\n"), MML, m_pInstrument->GetName());
	MML.AppendFormat(_T(";TL FB\r\n %2d,%2d,\r\n;AR DR SL RR KL MT AM VB EG KR DT\r\n"), reg[2] & 0x3F, reg[3] & 0x07);
	for (int i = 0; i <= 1; i++)
		MML.AppendFormat(_T(" %2d,%2d,%2d,%2d,%2d,%2d,%2d,%2d,%2d,%2d,%2d,\r\n"),
			(reg[4 + i] >> 4) & 0x0F, reg[4 + i] & 0x0F, (reg[6 + i] >> 4) & 0x0F, reg[6 + i] & 0x0F,
			(reg[2 + i] >> 6) & 0x03, reg[i] & 0x0F,
			(reg[i] >> 7) & 0x01, (reg[i] >> 6) & 0x01, (reg[i] >> 5) & 0x01, (reg[i] >> 4) & 0x01,
			(reg[3] >> (4 - i)) & 0x01);
	
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer(MML.GetBuffer(), MML.GetLength() + 1);
}

void CInstrumentEditorVRC7::OnPaste()
{
	// Copy from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPCTSTR text = (LPCTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			PasteSettings(text);
	}
}

void CInstrumentEditorVRC7::PasteSettings(LPCTSTR pString)
{
	std::string str(pString);

	// Convert to register values
	std::istringstream values(str);
	std::istream_iterator<std::string> begin(values);
	std::istream_iterator<std::string> end;

	for (int i = 0; (i < 8) && (begin != end); ++i) {
		int value = CSequenceInstrumentEditPanel::ReadStringValue(*begin++, false);		// // //
		value = std::min<int>(std::max<int>(value, 0x00), 0xFF);
		m_pInstrument->SetCustomReg(i, value);
	}

	LoadCustomPatch();
}

BOOL CInstrumentEditorVRC7::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && m_pInstrument != NULL && GetFocus() != GetDlgItem(IDC_PATCH)) {
		int Patch = m_pInstrument->GetPatch();
		switch (pMsg->wParam) {
			case VK_DOWN:
				if (Patch < 15) {
					SelectPatch(Patch + 1);
					static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(Patch + 1);
				}
				break;
			case VK_UP:
				if (Patch > 0) {
					SelectPatch(Patch - 1);
					static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(Patch - 1);
				}
				break;
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}
