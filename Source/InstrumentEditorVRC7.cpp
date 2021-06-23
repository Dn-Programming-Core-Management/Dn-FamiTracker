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
#include <sstream>
#include "stdafx.h"
#include "../resource.h"        // // //
#include "Instrument.h"
#include "InstrumentVRC7.h"		// // //
#include "InstrumentEditPanel.h"
#include "InstrumentEditorVRC7.h"
#include "Clipboard.h"

#include "FamiTracker.h"
#include "Settings.h"

static constexpr size_t REGS_PER_INSTR = 8;
static constexpr int CUSTOM_PATCH = 0;

// based off NSFPlay emu2413's hardware patch scheme
static constexpr int OPLL_TONE_NUM = 9;
static constexpr uint8_t DEFAULT_PATCHES[OPLL_TONE_NUM][(16 + 3) * REGS_PER_INSTR] =
{
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_nuke.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_rw.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_ft36.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_ft35.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_mo.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_kt2.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/vrc7tone_kt1.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/2413tone.h"
	},
	{
#include "APU/nsfplay/xgm/devices/Sound/legacy/281btone.h"
	},
};

// CInstrumentSettingsVRC7 dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorVRC7, CInstrumentEditPanel)

CInstrumentEditorVRC7::CInstrumentEditorVRC7(CWnd* pParent /*=NULL*/)
	: CInstrumentEditPanel(CInstrumentEditorVRC7::IDD, pParent)
{
}

CInstrumentEditorVRC7::~CInstrumentEditorVRC7()
{
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

	PatchBank = (PatchTone)theApp.GetSettings()->Emulation.iVRC7Patch;

	CComboBox* pPatchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PATCH));
	CString Text;
	const _TCHAR* VRC7_patchnames[16] = {
		// various VRC7 patch versions
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
	const _TCHAR* YM2413_patchnames[16] = {
		// YM2413 tone by Mitsutaka Okazaki, 2020
		_T("(custom patch)"),
		_T("Violin"),
		_T("Guitar"),
		_T("Piano"),
		_T("Flute"),
		_T("Clarinet"),
		_T("Oboe"),
		_T("Trumpet"),
		_T("Organ"),
		_T("Horn"),
		_T("Synthesizer"),
		_T("Harpsichord"),
		_T("Vibraphone"),
		_T("Synthsizer Bass"),
		_T("Acoustic Bass"),
		_T("Electric Guitar")
	};
	const _TCHAR* YMF281B_patchnames[16] = {
		// YMF281B tone by Chabin (4/10/2004) with fixes from plgDavid
		_T("(custom patch)"),
		_T("Electric Strings"),
		_T("Bow Wow"),
		_T("Electric Guitar"),
		_T("Organ"),
		_T("Clarinet"),
		_T("Saxophone"),
		_T("Trumpet"),
		_T("Street Organ"),
		_T("Synth Brass"),
		_T("Electric Piano"),
		_T("Bass"),
		_T("Vibraphone"),
		_T("Chimes"),
		_T("Tom Tom II"),
		_T("Noise")
	};

	_TCHAR const* const* patch_names;

	switch (PatchBank) {
	case VRC7_NUKE:
	case VRC7_RW:
	case VRC7_FT36:
	case VRC7_FT35:
	case VRC7_MO:
	case VRC7_KT2:
	case VRC7_KT1:
		patch_names = VRC7_patchnames;
		break;
	case TONE_2413:
		patch_names = YM2413_patchnames;
		break;
	case TONE_281B:
		patch_names = YMF281B_patchnames;
		break;
	}

	for (int i = 0; i < 16; ++i) {
		Text.Format(_T("Patch #%i - %s"), i, patch_names[i]);
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
	EnableControls(Patch == CUSTOM_PATCH);
	LoadPatch(Patch);
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

void CInstrumentEditorVRC7::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentVRC7>(pInst);
	ASSERT(m_pInstrument);

	CComboBox *pPatchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PATCH));

	int Patch = m_pInstrument->GetPatch();

	pPatchBox->SetCurSel(Patch);
	
	LoadPatch(Patch);

	EnableControls(Patch == CUSTOM_PATCH);
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

void CInstrumentEditorVRC7::LoadPatch(int Num)
{
	uint8_t inst_params[8];
	for (int i = 0; i < 8; ++i)
		inst_params[i] = FetchPatchByte(PatchBank, Num, i);

	// Register 0
	CheckDlgButton(IDC_M_AM, inst_params[0] & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_M_VIB, inst_params[0] & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_M_EG, inst_params[0] & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_M_KSR2, inst_params[0] & 0x10 ? 1 : 0);
	SetSliderVal(IDC_M_MUL, inst_params[0] & 0x0F);

	// Register 1
	CheckDlgButton(IDC_C_AM, inst_params[1] & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_C_VIB, inst_params[1] & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_C_EG, inst_params[1] & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_C_KSR, inst_params[1] & 0x10 ? 1 : 0);
	SetSliderVal(IDC_C_MUL, inst_params[1] & 0x0F);

	// Register 2
	SetSliderVal(IDC_M_KSL, inst_params[2] >> 6);
	SetSliderVal(IDC_TL, inst_params[2] & 0x3F);

	// Register 3
	SetSliderVal(IDC_C_KSL, inst_params[3] >> 6);
	SetSliderVal(IDC_FB, 7 - (inst_params[3] & 7));
	CheckDlgButton(IDC_C_DM, inst_params[3] & 0x10 ? 1 : 0);
	CheckDlgButton(IDC_M_DM, inst_params[3] & 0x08 ? 1 : 0);

	// Register 4
	SetSliderVal(IDC_M_AR, inst_params[4] >> 4);
	SetSliderVal(IDC_M_DR, inst_params[4] & 0x0F);

	// Register 5
	SetSliderVal(IDC_C_AR, inst_params[5] >> 4);
	SetSliderVal(IDC_C_DR, inst_params[5] & 0x0F);

	// Register 6
	SetSliderVal(IDC_M_SL, inst_params[6] >> 4);
	SetSliderVal(IDC_M_RR, inst_params[6] & 0x0F);

	// Register 7
	SetSliderVal(IDC_C_SL, inst_params[7] >> 4);
	SetSliderVal(IDC_C_RR, inst_params[7] & 0x0F);

	WritePatchText(Num);
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

	WritePatchText(CUSTOM_PATCH);
}

void CInstrumentEditorVRC7::WritePatchText(int Patch)
{
	CString patchtxt;

	for (int i = 0; i < 8; ++i)
		patchtxt.AppendFormat(_T("$%02X "), FetchPatchByte(PatchBank, Patch, i));

	CWnd* pPatchText = GetDlgItem(IDC_PATCH_TEXT);
	pPatchText->SetWindowText(patchtxt);
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
		MML.AppendFormat(_T("$%02X "), FetchPatchByte(PatchBank, patch, i));
	
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
		reg[i] = FetchPatchByte(PatchBank, patch, i);//patch == 0 ? m_pInstrument->GetCustomReg(i) : DEFAULT_PATCHES[PatchBank][patch * 8 + i];
	
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

uint8_t CInstrumentEditorVRC7::FetchPatchByte(PatchTone patch_bank_id, int patch, unsigned char patch_byte)
{
	if (patch > CUSTOM_PATCH)
		return DEFAULT_PATCHES[patch_bank_id][patch * REGS_PER_INSTR + patch_byte];
	else
		return m_pInstrument->GetCustomReg(patch_byte);
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
		if (value < 0) value = 0;
		if (value > 0xFF) value = 0xFF;
		m_pInstrument->SetCustomReg(i, value);
	}

	int Patch = m_pInstrument->GetPatch();
	if (Patch != CUSTOM_PATCH) {
		SelectPatch(CUSTOM_PATCH);
		static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(0);
		LoadPatch(CUSTOM_PATCH);
	}
	else
		LoadPatch(CUSTOM_PATCH);
}

BOOL CInstrumentEditorVRC7::PreTranslateMessage(MSG* pMsg)
{
	int Patch = m_pInstrument->GetPatch();

	if (pMsg->message == WM_KEYDOWN && m_pInstrument != NULL) {
		if (GetFocus() == GetDlgItem(IDC_PATCH_TEXT)) {
			if (pMsg->wParam == VK_RETURN) {
				CString patchtxt;
				CWnd* pPatchText = GetDlgItem(IDC_PATCH_TEXT);
				pPatchText->GetWindowText(patchtxt);
				if (Patch != CUSTOM_PATCH) {
					SelectPatch(CUSTOM_PATCH);
					static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(0);
					PasteSettings(patchtxt);
				}
				else
					PasteSettings(patchtxt);
			}
		}
		else if (GetFocus() != GetDlgItem(IDC_PATCH)) {
			switch (pMsg->wParam) {
			case VK_DOWN:
				if (Patch < 15) {
					SelectPatch(Patch + 1);
					static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(Patch + 1);
				}
				break;
			case VK_UP:
				if (Patch > CUSTOM_PATCH) {
					SelectPatch(Patch - 1);
					static_cast<CComboBox*>(GetDlgItem(IDC_PATCH))->SetCurSel(Patch - 1);
				}
				break;
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}
