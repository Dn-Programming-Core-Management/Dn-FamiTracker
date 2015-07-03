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

#include <string>
#include <iterator> 
#include <string>
#include <sstream>
#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorN163Wave.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "Clipboard.h"

using namespace std;

#define WAVE_SIZE_AVAILABLE (256 - 16 * GetDocument()->GetNamcoChannels())		// // //

// CInstrumentEditorN163Wave dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorN163Wave, CInstrumentEditPanel)

CInstrumentEditorN163Wave::CInstrumentEditorN163Wave(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorN163Wave::IDD, pParent),
	m_pWaveEditor(NULL), 
	m_pInstrument(NULL),
	m_iWaveIndex(0)
{
}

CInstrumentEditorN163Wave::~CInstrumentEditorN163Wave()
{
	SAFE_RELEASE(m_pWaveEditor);

	if (m_pInstrument)
		m_pInstrument->Release();
}

void CInstrumentEditorN163Wave::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorN163Wave::SelectInstrument(int Instrument)
{
	if (m_pInstrument)
		m_pInstrument->Release();

	m_pInstrument = static_cast<CInstrumentN163*>(GetDocument()->GetInstrument(Instrument));
	ASSERT(m_pInstrument->GetType() == INST_N163);

	CComboBox *pSizeBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE));
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));

	pSizeBox->SelectString(0, MakeIntString(m_pInstrument->GetWaveSize()));
	FillPosBox(m_pInstrument->GetWaveSize());
	pPosBox->SetWindowText(MakeIntString(m_pInstrument->GetWavePos()));

	/*
	if (m_pInstrument->GetAutoWavePos()) {
		CheckDlgButton(IDC_POSITION, 1);
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(FALSE);
	}
	else {
		CheckDlgButton(IDC_POSITION, 0);
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(TRUE);
	}
	*/

	if (m_pWaveEditor) {
		m_pWaveEditor->SetInstrument(m_pInstrument);
		m_pWaveEditor->SetLength(m_pInstrument->GetWaveSize());
	}

	CSpinButtonCtrl *pIndexSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_INDEX_SPIN));
	CSpinButtonCtrl *pWavesSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_WAVES_SPIN));

	int WaveCount = m_pInstrument->GetWaveCount();

	pIndexSpin->SetRange(0, WaveCount - 1);
	pIndexSpin->SetPos(0);
	pWavesSpin->SetPos(WaveCount - 1);

	m_iWaveIndex = 0;
}

BEGIN_MESSAGE_MAP(CInstrumentEditorN163Wave, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_MESSAGE(WM_USER_WAVE_CHANGED, OnWaveChanged)
	ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
	ON_BN_CLICKED(IDC_PASTE, OnBnClickedPaste)
	ON_CBN_SELCHANGE(IDC_WAVE_SIZE, OnWaveSizeChange)
	ON_CBN_EDITCHANGE(IDC_WAVE_POS, OnWavePosChange)
	ON_CBN_SELCHANGE(IDC_WAVE_POS, OnWavePosSelChange)
//	ON_BN_CLICKED(IDC_POSITION, OnPositionClicked)
	ON_EN_CHANGE(IDC_WAVES, OnWavesChange)
	ON_EN_CHANGE(IDC_INDEX, OnIndexChange)
END_MESSAGE_MAP()

// CInstrumentEditorN163Wave message handlers

BOOL CInstrumentEditorN163Wave::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	CRect rect(SX(20), SY(30), 0, 0);
	m_pWaveEditor = new CWaveEditorN163(10, 8, 32, 16);
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, this);
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	CSpinButtonCtrl *pIndexSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_INDEX_SPIN));
	CSpinButtonCtrl *pWavesSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_WAVES_SPIN));

	pIndexSpin->SetRange(0, CInstrumentN163::MAX_WAVE_COUNT - 1);
	pWavesSpin->SetRange(0, CInstrumentN163::MAX_WAVE_COUNT - 1);
	
	CComboBox *pWaveSize = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE));

	for (int i = 0; i < WAVE_SIZE_AVAILABLE; i += 4) {
		pWaveSize->AddString(MakeIntString(i + 4));
	} 

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorN163Wave::OnPresetSine()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		float angle = (float(i) * 3.141592f * 2.0f) / float(size) + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 7.5f + 0.5f);
		m_pInstrument->SetSample(m_iWaveIndex, i, sample);
	}

	m_pWaveEditor->WaveChanged();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorN163Wave::OnPresetTriangle()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = ((i < (size / 2) ? i : ((size - 1) - i))) * 16 / (size / 2);		// // //
		m_pInstrument->SetSample(m_iWaveIndex, i, sample);
	}

	m_pWaveEditor->WaveChanged();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorN163Wave::OnPresetPulse50()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i < (size / 2) ? 0 : 15);
		m_pInstrument->SetSample(m_iWaveIndex, i, sample);
	}

	m_pWaveEditor->WaveChanged();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorN163Wave::OnPresetPulse25()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i < (size / 4) ? 0 : 15);
		m_pInstrument->SetSample(m_iWaveIndex, i, sample);
	}

	m_pWaveEditor->WaveChanged();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorN163Wave::OnPresetSawtooth()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i * 16) / size;
		m_pInstrument->SetSample(m_iWaveIndex, i, sample);
	}

	m_pWaveEditor->WaveChanged();
	theApp.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorN163Wave::OnBnClickedCopy()
{
	CString Str;
	int len = m_pInstrument->GetWaveSize();

	// Assemble a MML string
	for (int i = 0; i < len; ++i)
		Str.AppendFormat(_T("%i "), m_pInstrument->GetSample(m_iWaveIndex, i));

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer(Str.GetBuffer(), Str.GetLength() + 1);
}

void CInstrumentEditorN163Wave::OnBnClickedPaste()
{
	// Copy from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPTSTR text = (LPTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseString(text);
	}
}

void CInstrumentEditorN163Wave::ParseString(LPCTSTR pString)
{
	string str(pString);

	// Convert to register values
	istringstream values(str);
	istream_iterator<int> begin(values);
	istream_iterator<int> end;

	int i;
	for (i = 0; (i < WAVE_SIZE_AVAILABLE) && (begin != end); ++i) {		// // //
		int value = *begin++;
		if (value >= 0 && value <= 15)
			m_pInstrument->SetSample(m_iWaveIndex, i, value);
	}

	int size = i & 0xFC;
	if (size < 4)
		size = 4;
	m_pInstrument->SetWaveSize(size);

	static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE))->SelectString(0, MakeIntString(size));

	FillPosBox(size);

	m_pWaveEditor->SetLength(size);
	m_pWaveEditor->WaveChanged();
}

LRESULT CInstrumentEditorN163Wave::OnWaveChanged(WPARAM wParam, LPARAM lParam)
{
	CString str;
	int Size = m_pInstrument->GetWaveSize();
	for (int i = 0; i < Size; ++i) {
		str.AppendFormat(_T("%i "), m_pInstrument->GetSample(m_iWaveIndex, i));
	}
	SetDlgItemText(IDC_MML, str);
	return 0;
}

void CInstrumentEditorN163Wave::OnWaveSizeChange()
{
	BOOL trans;
	int size = GetDlgItemInt(IDC_WAVE_SIZE, &trans, FALSE);
	size = size & 0xFC;
	
	if (size > WAVE_SIZE_AVAILABLE)
		size = WAVE_SIZE_AVAILABLE;
	if (size < 4)
		size = 4;

	m_pInstrument->SetWaveSize(size);

	FillPosBox(size);

	m_pWaveEditor->SetLength(size);
	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN163Wave::OnWavePosChange()
{
	BOOL trans;
	int pos = GetDlgItemInt(IDC_WAVE_POS, &trans, FALSE);
	
	if (pos > 255)
		pos = 255;
	if (pos < 0)
		pos = 0;

	m_pInstrument->SetWavePos(pos);
}

void CInstrumentEditorN163Wave::OnWavePosSelChange()
{
	CString str;
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));
	pPosBox->GetLBText(pPosBox->GetCurSel(), str);

	int pos = _ttoi(str);

	if (pos > 255)
		pos = 255;
	if (pos < 0)
		pos = 0;

	m_pInstrument->SetWavePos(pos);
}

void CInstrumentEditorN163Wave::FillPosBox(int size)
{
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));
	pPosBox->ResetContent();

	for (int i = 0; i <= WAVE_SIZE_AVAILABLE - size; i += size) {		// // // prevent reading non-wave n163 registers
		pPosBox->AddString(MakeIntString(i));
	}
}
/*
void CInstrumentEditorN163Wave::OnPositionClicked()
{
	if (IsDlgButtonChecked(IDC_POSITION)) {
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(FALSE);
		m_pInstrument->SetAutoWavePos(true);
	}
	else {
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(TRUE);
		m_pInstrument->SetAutoWavePos(false);
	}
}
*/
void CInstrumentEditorN163Wave::OnWavesChange()
{
	CSpinButtonCtrl *pIndexSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_INDEX_SPIN));

	int count = GetDlgItemInt(IDC_WAVES) + 1;
	
	if (m_pInstrument != NULL)
		m_pInstrument->SetWaveCount(count);

	pIndexSpin->SetRange(0, count - 1);
	pIndexSpin->RedrawWindow();

	if (pIndexSpin->GetPos() > (count - 1))
		pIndexSpin->SetPos(count - 1);

	if (m_pWaveEditor != NULL) {
		m_pWaveEditor->SetWave(m_iWaveIndex);
		m_pWaveEditor->WaveChanged();
	}
}

void CInstrumentEditorN163Wave::OnIndexChange()
{
	m_iWaveIndex = GetDlgItemInt(IDC_INDEX);

	if (m_pWaveEditor != NULL) {
		m_pWaveEditor->SetWave(m_iWaveIndex);
		m_pWaveEditor->WaveChanged();
	}
}

void CInstrumentEditorN163Wave::OnKeyReturn()
{
	// Parse MML string
	CString text;
	GetDlgItemText(IDC_MML, text);
	ParseString(text);
}
