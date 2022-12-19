/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <iterator> 
#include <sstream>
#include "stdafx.h"
#include "FamiTracker.h"
#include "Instrument.h"		// // //
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "DPI.h"		// // //
#include "InstrumentEditPanel.h"
#include "InstrumentEditorFDS.h"
#include "APU/Types.h"		// // //
#include "SoundGen.h"
#include "Clipboard.h"
#include "WaveEditor.h"		// // //
#include "ModSequenceEditor.h"

// CInstrumentEditorFDS dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDS, CInstrumentEditPanel)

CInstrumentEditorFDS::CInstrumentEditorFDS(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorFDS::IDD, pParent),
	m_pWaveEditor(NULL), 
	m_pModSequenceEditor(NULL)
{
}

CInstrumentEditorFDS::~CInstrumentEditorFDS()
{
	SAFE_RELEASE(m_pModSequenceEditor);
	SAFE_RELEASE(m_pWaveEditor);
}

void CInstrumentEditorFDS::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorFDS::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(pInst);
	ASSERT(m_pInstrument);
	
	if (m_pWaveEditor)
		m_pWaveEditor->SetInstrument(m_pInstrument);

	if (m_pModSequenceEditor)
		m_pModSequenceEditor->SetInstrument(m_pInstrument);

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetPos(m_pInstrument->GetModulationSpeed());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetPos(m_pInstrument->GetModulationDepth());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetPos(m_pInstrument->GetModulationDelay());

//	CheckDlgButton(IDC_ENABLE_FM, m_pInstrument->GetModulationEnable() ? 1 : 0);

	EnableModControls(m_pInstrument->GetModulationEnable());
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDS, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_COMMAND(IDC_MOD_PRESET_FLAT, OnModPresetFlat)
	ON_COMMAND(IDC_MOD_PRESET_SINE, OnModPresetSine)
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_MOD_RATE, OnModRateChange)
	ON_EN_CHANGE(IDC_MOD_DEPTH, OnModDepthChange)
	ON_EN_CHANGE(IDC_MOD_DELAY, OnModDelayChange)
	ON_BN_CLICKED(IDC_COPY_WAVE, &CInstrumentEditorFDS::OnBnClickedCopyWave)
	ON_BN_CLICKED(IDC_PASTE_WAVE, &CInstrumentEditorFDS::OnBnClickedPasteWave)
	ON_BN_CLICKED(IDC_COPY_TABLE, &CInstrumentEditorFDS::OnBnClickedCopyTable)
	ON_BN_CLICKED(IDC_PASTE_TABLE, &CInstrumentEditorFDS::OnBnClickedPasteTable)
//	ON_BN_CLICKED(IDC_ENABLE_FM, &CInstrumentEditorFDS::OnBnClickedEnableFm)
	ON_MESSAGE(WM_USER + 1, OnModChanged)
END_MESSAGE_MAP()

// CInstrumentEditorFDS message handlers

BOOL CInstrumentEditorFDS::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	m_pWaveEditor = new CWaveEditorFDS(5, 2, 64, 64);
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(20, 30, 0, 0), this);		// // //
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	// Create modulation sequence editor
	m_pModSequenceEditor = new CModSequenceEditor();
	m_pModSequenceEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(10, 200, 0, 0), this);		// // //
	m_pModSequenceEditor->ShowWindow(SW_SHOW);
	m_pModSequenceEditor->UpdateWindow();

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetRange(0, 4095);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetRange(0, 63);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetRange(0, 255);
/*
	CSliderCtrl *pModSlider;
	pModSlider = (CSliderCtrl*)GetDlgItem(IDC_MOD_FREQ);
	pModSlider->SetRange(0, 0xFFF);
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CInstrumentEditorFDS::PreTranslateMessage(MSG* pMsg)		// // //
{
	if (pMsg->message == WM_KEYDOWN) {
		if ((::GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
			switch (pMsg->wParam) {
			case VK_LEFT:
				m_pWaveEditor->PhaseShift(1);
				return TRUE;
			case VK_RIGHT:
				m_pWaveEditor->PhaseShift(-1);
				return TRUE;
			case VK_DOWN:
				m_pWaveEditor->Invert(63);
				return TRUE;
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorFDS::OnPresetSine()
{
	for (int i = 0; i < 64; ++i) {
		float angle = (float(i) * 3.141592f * 2.0f) / 64.0f + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 31.5f + 0.5f);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetTriangle()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? i << 1 : (63 - i) << 1);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetPulse50()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetPulse25()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 16 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetSawtooth()
{
	for (int i = 0; i < 64; ++i) {
		int sample = i;
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetFlat()
{
	for (int i = 0; i < CInstrumentFDS::MOD_SIZE; ++i) {
		m_pInstrument->SetModulation(i, 0);
	}

	m_pModSequenceEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetSine()
{
	const int BEGIN = 0;
	const int CENTER = CInstrumentFDS::MOD_SIZE / 2;
	const int END = CInstrumentFDS::MOD_SIZE;
	const int DELTA = 6;

	for (int i = 0; i < END; i++) m_pInstrument->SetModulation(i, 0);

	m_pInstrument->SetModulation(BEGIN, 4);
	m_pInstrument->SetModulation(CENTER, 4);

	for (int i = 1; i <= DELTA; ++i) {
		m_pInstrument->SetModulation(BEGIN + i, -1);
		m_pInstrument->SetModulation(CENTER - i, 1);
		m_pInstrument->SetModulation(CENTER + i, 1);
		m_pInstrument->SetModulation(END - i, -1);
	}

	m_pModSequenceEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ModSpeed = GetDlgItemInt(IDC_MOD_RATE);
//	int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
//	int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);

	m_pInstrument->SetModulationSpeed(ModSpeed);
}

void CInstrumentEditorFDS::OnModRateChange()
{
	if (m_pInstrument) {
		int ModSpeed = GetDlgItemInt(IDC_MOD_RATE);
		ModSpeed = std::max(ModSpeed, 0);
		ModSpeed = std::min(ModSpeed, 4095);
		m_pInstrument->SetModulationSpeed(ModSpeed);
	}
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModDepthChange()
{
	if (m_pInstrument) {
		int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
		ModDepth = std::max(ModDepth, 0);
		ModDepth = std::min(ModDepth, 63);
		m_pInstrument->SetModulationDepth(ModDepth);
	}
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModDelayChange()
{
	if (m_pInstrument) {
		int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);
		ModDelay = std::max(ModDelay, 0);
		ModDelay = std::min(ModDelay, 255);
		m_pInstrument->SetModulationDelay(ModDelay);
	}
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedCopyWave()
{
	CString Str;

	// Assemble a MML string
	for (int i = 0; i < 64; ++i)
		Str.AppendFormat(_T("%i "), m_pInstrument->GetSample(i));

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer(Str.GetBuffer(), Str.GetLength() + 1);
}

void CInstrumentEditorFDS::OnBnClickedPasteWave()
{
	// Paste from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPCTSTR text = (LPCTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseWaveString(text);
	}
}

void CInstrumentEditorFDS::ParseWaveString(LPCTSTR pString)
{
	std::string str(pString);

	// Convert to register values
	std::istringstream values(str);
	std::istream_iterator<std::string> begin(values);
	std::istream_iterator<std::string> end;

	for (int i = 0; (i < 64) && (begin != end); ++i) {
		int value = CSequenceInstrumentEditPanel::ReadStringValue(*begin++, false);
		value = std::min<int>(std::max<int>(value, 0), 63);
		m_pInstrument->SetSample(i, value);
	}

	m_pWaveEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedCopyTable()
{
	CString Str;

	// Assemble a MML string
	for (int i = 0; i < 32; ++i)
		Str.AppendFormat(_T("%i "), m_pInstrument->GetModulation(i));

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer(Str.GetBuffer(), Str.GetLength() + 1);
}

void CInstrumentEditorFDS::OnBnClickedPasteTable()
{
	// Paste from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPCTSTR text = (LPCTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseTableString(text);
	}
}

void CInstrumentEditorFDS::ParseTableString(LPCTSTR pString)
{
	std::string str(pString);

	// Convert to register values
	std::istringstream values(str);
	std::istream_iterator<std::string> begin(values);
	std::istream_iterator<std::string> end;

	for (int i = 0; (i < 32) && (begin != end); ++i) {
		int value = CSequenceInstrumentEditPanel::ReadStringValue(*begin++, false);
		value = std::min<int>(std::max<int>(value, 0), 7);
		m_pInstrument->SetModulation(i, value);
	}

	m_pModSequenceEditor->RedrawWindow();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedEnableFm()
{
	/*
	UINT button = IsDlgButtonChecked(IDC_ENABLE_FM);

	EnableModControls(button == 1);
	*/
}

void CInstrumentEditorFDS::EnableModControls(bool enable)
{
	if (!enable) {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(FALSE);
//		m_pInstrument->SetModulationEnable(false);
	}
	else {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(TRUE);
//		m_pInstrument->SetModulationEnable(true);
	}
}

LRESULT CInstrumentEditorFDS::OnModChanged(WPARAM wParam, LPARAM lParam)
{
	theApp.GetSoundGenerator()->WaveChanged();
	return 0;
}
