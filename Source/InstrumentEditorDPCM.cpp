/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "InstrumentEditorDPCM.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "SampleEditorView.h"
#include "SampleEditorDlg.h"
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "PCMImport.h"
#include "Settings.h"
#include "SoundGen.h"
#include "DSample.h"		// // //
#include "PatternNote.h"		// // // note names
#include <algorithm>		// // //

LPCTSTR NO_SAMPLE_STR = _T("(no sample)");

// Derive a new class from CFileDialog with implemented preview of DMC files

class CDMCFileSoundDialog : public CFileDialog
{
public:
	CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL, DWORD dwSize = 0);
	virtual ~CDMCFileSoundDialog();

	static const int DEFAULT_PREVIEW_PITCH = 15;

protected:
	virtual void OnFileNameChange();
	CString m_strLastFile;
};

//	CFileSoundDialog

CDMCFileSoundDialog::CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize) 
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize)
{
}

CDMCFileSoundDialog::~CDMCFileSoundDialog()
{
	// Stop any possible playing sound
	//PlaySound(NULL, NULL, SND_NODEFAULT | SND_SYNC);
}

void CDMCFileSoundDialog::OnFileNameChange()
{
	// Preview DMC file
	if (!GetFileExt().CompareNoCase(_T("dmc")) && theApp.GetSettings()->General.bWavePreview) {
		DWORD dwAttrib = GetFileAttributes(GetPathName());
		if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) && GetPathName() != m_strLastFile) {
			CFile file(GetPathName(), CFile::modeRead);
			ULONGLONG size = file.GetLength();
			size = std::min<ULONGLONG>(size, CDSample::MAX_SIZE);
			CDSample *pSample = new CDSample((int)size);
			file.Read(pSample->GetData(), (int)size);
			theApp.GetSoundGenerator()->PreviewSample(pSample, 0, DEFAULT_PREVIEW_PITCH);
			file.Close();
			m_strLastFile = GetPathName();
		}
	}
	
	CFileDialog::OnFileNameChange();
}

// CInstrumentDPCM dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorDPCM, CInstrumentEditPanel)
CInstrumentEditorDPCM::CInstrumentEditorDPCM(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorDPCM::IDD, pParent), m_pInstrument(NULL)
{
}

CInstrumentEditorDPCM::~CInstrumentEditorDPCM()
{
}

void CInstrumentEditorDPCM::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditorDPCM, CInstrumentEditPanel)
	ON_BN_CLICKED(IDC_LOAD, &CInstrumentEditorDPCM::OnBnClickedLoad)
	ON_BN_CLICKED(IDC_UNLOAD, &CInstrumentEditorDPCM::OnBnClickedUnload)
	ON_BN_CLICKED(IDC_IMPORT, &CInstrumentEditorDPCM::OnBnClickedImport)
	ON_BN_CLICKED(IDC_SAVE, &CInstrumentEditorDPCM::OnBnClickedSave)
	ON_BN_CLICKED(IDC_LOOP, &CInstrumentEditorDPCM::OnBnClickedLoop)
	ON_BN_CLICKED(IDC_ADD, &CInstrumentEditorDPCM::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE, &CInstrumentEditorDPCM::OnBnClickedRemove)
	ON_BN_CLICKED(IDC_EDIT, &CInstrumentEditorDPCM::OnBnClickedEdit)
	ON_BN_CLICKED(IDC_PREVIEW, &CInstrumentEditorDPCM::OnBnClickedPreview)
	ON_CBN_SELCHANGE(IDC_PITCH, &CInstrumentEditorDPCM::OnCbnSelchangePitch)
	ON_CBN_SELCHANGE(IDC_SAMPLES, &CInstrumentEditorDPCM::OnCbnSelchangeSamples)
	ON_NOTIFY(NM_CLICK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMClickSampleList)
	ON_NOTIFY(NM_CLICK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMClickTable)
	ON_NOTIFY(NM_DBLCLK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMDblclkSampleList)
	ON_NOTIFY(NM_RCLICK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMRClickSampleList)
	ON_NOTIFY(NM_RCLICK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMRClickTable)
	ON_NOTIFY(NM_DBLCLK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMDblclkTable)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DELTA_SPIN, &CInstrumentEditorDPCM::OnDeltaposDeltaSpin)
	ON_EN_CHANGE(IDC_LOOP_POINT, &CInstrumentEditorDPCM::OnEnChangeLoopPoint)
	ON_EN_CHANGE(IDC_DELTA_COUNTER, &CInstrumentEditorDPCM::OnEnChangeDeltaCounter)
END_MESSAGE_MAP()

// CInstrumentDPCM message handlers

BOOL CInstrumentEditorDPCM::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	m_iOctave = 3;
	m_iSelectedKey = 0;

	CListCtrl *pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	CRect r;		// // // 050B
	pTableListCtrl->GetClientRect(&r);
	int Width = r.Width() - ::GetSystemMetrics(SM_CXHSCROLL);
	pTableListCtrl->DeleteAllItems();
	pTableListCtrl->InsertColumn(0, _T("Key"), LVCFMT_LEFT, static_cast<int>(.2 * Width));
	pTableListCtrl->InsertColumn(1, _T("Pitch"), LVCFMT_LEFT, static_cast<int>(.23 * Width));
	pTableListCtrl->InsertColumn(2, _T("Sample"), LVCFMT_LEFT, static_cast<int>(.57 * Width));
	pTableListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	pSampleListCtrl->GetClientRect(&r);
	Width = r.Width();
	pSampleListCtrl->DeleteAllItems();
	pSampleListCtrl->InsertColumn(0, _T("#"), LVCFMT_LEFT, static_cast<int>(.2 * Width));		// // //
	pSampleListCtrl->InsertColumn(1, _T("Name"), LVCFMT_LEFT, static_cast<int>(.55 * Width));
	pSampleListCtrl->InsertColumn(2, _T("Size"), LVCFMT_LEFT, static_cast<int>(.25 * Width));
	pSampleListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
	
	CComboBox *pPitch = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	for (int i = 0; i < 16; ++i)
		pPitch->AddString(std::to_string(i).c_str());
	pPitch->SetCurSel(15);

	CheckDlgButton(IDC_LOOP, 0);
	
	CString text;		// // //
	pTableListCtrl->DeleteAllItems();
	for (int i = 0; i < NOTE_COUNT; ++i) {
		text.Format(_T("%s%d"), stChanNote::NOTE_NAME[GET_NOTE(i) - 1].c_str(), GET_OCTAVE(i));
		pTableListCtrl->InsertItem(i, text);
	}
	pTableListCtrl->GetItemRect(0, &r, 2);		// // //
	pTableListCtrl->Scroll({0, m_iOctave * NOTE_RANGE * r.Height()});

	BuildSampleList();
	m_iSelectedSample = 0;

	CSpinButtonCtrl *pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_DELTA_SPIN);
	pSpin->SetRange(-1, 127);
	pSpin->SetPos(-1);

	SetDlgItemText(IDC_DELTA_COUNTER, _T("Off"));

	static_cast<CButton*>(GetDlgItem(IDC_ADD))->SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_LEFT)));
	static_cast<CButton*>(GetDlgItem(IDC_REMOVE))->SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RIGHT)));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorDPCM::BuildKeyList()
{
	for (int i = 0; i < NOTE_COUNT; ++i)
		UpdateKey(i);
}

void CInstrumentEditorDPCM::UpdateCurrentKey()		// // //
{
	UpdateKey(MIDI_NOTE(m_iOctave, m_iSelectedKey + 1));
}

void CInstrumentEditorDPCM::UpdateKey(int Index)
{
	CListCtrl *pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	CString NameStr = NO_SAMPLE_STR;
	CString PitchStr = _T("-");
	
	int Octave = GET_OCTAVE(Index);		// // //
	int Note = GET_NOTE(Index) - 1;
	if (m_pInstrument->GetSampleIndex(Octave, Note) > 0) {
		int Item = m_pInstrument->GetSampleIndex(Octave, Note) - 1;
		int Pitch = m_pInstrument->GetSamplePitch(Octave, Note);
		const CDSample *pSample = m_pInstrument->GetDSample(Octave, Note);		// // //
		NameStr = !pSample ? _T("(n/a)") : pSample->GetName();
		PitchStr.Format(_T("%i %s"), Pitch & 0x0F, (Pitch & 0x80) ? "L" : "");
	}

	pTableListCtrl->SetItemText(Index, 1, PitchStr);
	pTableListCtrl->SetItemText(Index, 2, NameStr);
}

void CInstrumentEditorDPCM::BuildSampleList()
{
	CComboBox *pSampleBox = static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	pSampleListCtrl->DeleteAllItems();
	pSampleBox->ResetContent();

	unsigned int Size(0), Index(0);
	CString	Text;

	pSampleBox->AddString(NO_SAMPLE_STR);

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (const CDSample *pDSample = GetDocument()->GetSample(i)) {		// // //
			pSampleListCtrl->InsertItem(Index, MakeIntString(i));
			Text.Format(_T("%s"), pDSample->GetName());
			pSampleListCtrl->SetItemText(Index, 1, Text);
			Text.Format(_T("%i"), pDSample->GetSize());
			pSampleListCtrl->SetItemText(Index, 2, Text);
			Text.Format(_T("%02i - %s"), i, pDSample->GetName());
			pSampleBox->AddString(Text);
			Size += pDSample->GetSize();
			++Index;
		}
	}

	AfxFormatString3(Text, IDS_DPCM_SPACE_FORMAT, 
		MakeIntString(Size / 0x400), 
		MakeIntString((MAX_SAMPLE_SPACE - Size) / 0x400),
		MakeIntString(MAX_SAMPLE_SPACE / 0x400));
	
	SetDlgItemText(IDC_SPACE, Text);
}

// When saved in NSF, the samples has to be aligned at even 6-bits addresses
//#define ADJUST_FOR_STORAGE(x) (((x >> 6) + (x & 0x3F ? 1 : 0)) << 6)
// TODO: I think I was wrong
#define ADJUST_FOR_STORAGE(x) (x)

bool CInstrumentEditorDPCM::LoadSample(const CString &FilePath, const CString &FileName)
{
	CFile SampleFile;

	if (!SampleFile.Open(FilePath, CFile::modeRead)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return false;
	}

	int Size = (int)SampleFile.GetLength();
	int AddSize = 0;
	
	// Clip file if too large
	Size = std::min(Size, CDSample::MAX_SIZE);
	
	// Make sure size is compatible with DPCM hardware
	if ((Size & 0xF) != 1) {
		AddSize = 0x10 - ((Size + 0x0F) & 0x0F);
	}

	CDSample *pNewSample = new CDSample(Size + AddSize);

	SampleFile.Read(pNewSample->GetData(), Size);
	// Pad uneven sizes with AAh
	memset(pNewSample->GetData() + Size, 0xAA, AddSize);

	pNewSample->SetName(FileName);

	SampleFile.Close();

	if (!InsertSample(pNewSample))
		return false;
	
	BuildSampleList();

	return true;
}

bool CInstrumentEditorDPCM::InsertSample(CDSample *pNewSample)
{	
	int FreeSlot = GetDocument()->GetFreeSampleSlot();

	// Out of sample slots
	if (FreeSlot == -1) {
		delete pNewSample;
		return false;
	}

	int Size = GetDocument()->GetTotalSampleSize();
	
	if ((Size + pNewSample->GetSize()) > MAX_SAMPLE_SPACE) {
		CString message;
		AfxFormatString1(message, IDS_OUT_OF_SAMPLEMEM_FORMAT, MakeIntString(MAX_SAMPLE_SPACE / 1024));
		AfxMessageBox(message, MB_ICONERROR);
	}
	else
		GetDocument()->SetSample(FreeSlot, pNewSample);		// // //

	return true;
}

void CInstrumentEditorDPCM::OnBnClickedLoad()
{
	CString fileFilter = LoadDefaultFilter(IDS_FILTER_DMC, _T(".dmc"));
	CDMCFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, fileFilter);

	OpenFileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_DMC);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return;

	theApp.GetSettings()->SetPath(OpenFileDialog.GetPathName(), PATH_DMC);

	if (OpenFileDialog.GetFileName().GetLength() == 0) {
		theApp.GetSettings()->SetPath(OpenFileDialog.GetPathName() + _T("\\"), PATH_DMC);		// // //
		// Multiple files
		POSITION Pos = OpenFileDialog.GetStartPosition();
		while (Pos) {
			CString Path = OpenFileDialog.GetNextPathName(Pos);
			CString FileName = Path.Right(Path.GetLength() - Path.ReverseFind('\\') - 1);
			LoadSample(Path, FileName);
		}
	}
	else {
		// Single file
		LoadSample(OpenFileDialog.GetPathName(), OpenFileDialog.GetFileName());
	}
}

void CInstrumentEditorDPCM::OnBnClickedUnload()
{
	CListCtrl *pListBox = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	int SelCount;
	char ItemName[256];
	int nItem = -1;

	if (m_iSelectedSample == MAX_DSAMPLES)
		return;
	
	if (!(SelCount = pListBox->GetSelectedCount()))
		return;

	for (int i = 0; i < SelCount; i++) {
		nItem = pListBox->GetNextItem(nItem, LVNI_SELECTED);
		ASSERT(nItem != -1);
		pListBox->GetItemText(nItem, 0, ItemName, 256);
		int Index = atoi(ItemName);
		theApp.GetSoundGenerator()->CancelPreviewSample();
		GetDocument()->RemoveSample(Index);
	}

	BuildSampleList();
}

void CInstrumentEditorDPCM::OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	if (pSampleListCtrl->GetItemCount() == 0)
		return;

	int Index = pSampleListCtrl->GetSelectionMark();
	
	TCHAR ItemName[256];
	pSampleListCtrl->GetItemText(Index, 0, ItemName, 256);
	m_iSelectedSample = _ttoi(ItemName);

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnBnClickedImport()
{
	CPCMImport	ImportDialog;
	CDSample	*pImported;

	if (!(pImported = ImportDialog.ShowDialog()))
		return;

	InsertSample(pImported);
	BuildSampleList();
}

void CInstrumentEditorDPCM::OnCbnSelchangePitch()
{
	if (m_iSelectedKey == -1)
		return;

	int Pitch = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH))->GetCurSel();

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);

	UpdateCurrentKey();
}

void CInstrumentEditorDPCM::OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	//CSpinButtonCtrl *pSpinButton;
	//CComboBox *pSampleBox, *pPitchBox;
	//CEdit *pDeltaValue;
	CString Text;

	//m_pTableListCtrl	= static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	CListCtrl *pTableListCtrl	 = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	CComboBox *pSampleBox		 = static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	CComboBox *pPitchBox		 = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	CSpinButtonCtrl *pSpinButton = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_DELTA_SPIN));
	CEdit *pDeltaValue			 = static_cast<CEdit*>(GetDlgItem(IDC_DELTA_COUNTER));

	m_iSelectedKey = GET_NOTE(pTableListCtrl->GetSelectionMark()) - 1;		// // //
	m_iOctave = GET_OCTAVE(pTableListCtrl->GetSelectionMark());

	int Sample = m_pInstrument->GetSampleIndex(m_iOctave, m_iSelectedKey) - 1;
	int Pitch = m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey);
	int Delta = m_pInstrument->GetSampleDeltaValue(m_iOctave, m_iSelectedKey);

	Text.Format(_T("%02i - %s"), Sample, pTableListCtrl->GetItemText(pTableListCtrl->GetSelectionMark(), 2));

	if (Sample != -1)
		pSampleBox->SelectString(0, Text);
	else
		pSampleBox->SetCurSel(0);
	
	if (Sample >= 0) {
		pPitchBox->SetCurSel(Pitch & 0x0F);

		if (Pitch & 0x80)
			CheckDlgButton(IDC_LOOP, 1);
		else
			CheckDlgButton(IDC_LOOP, 0);

		if (Delta == -1)
			pDeltaValue->SetWindowText(_T("Off"));
		else
			SetDlgItemInt(IDC_DELTA_COUNTER, Delta, FALSE);

		pSpinButton->SetPos(Delta);
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnCbnSelchangeSamples()
{
	CComboBox *pSampleBox = static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	
	int PrevSample = m_pInstrument->GetSampleIndex(m_iOctave, m_iSelectedKey);
	int Sample = pSampleBox->GetCurSel();

	if (Sample > 0) {
		char Name[256];
		pSampleBox->GetLBText(Sample, Name);
		
		Name[2] = 0;
		if (Name[0] == _T('0')) {
			Name[0] = Name[1];
			Name[1] = 0;
		}

		Sample = _tstoi(Name);
		Sample++;

		if (PrevSample == 0)
			m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, pPitchBox->GetCurSel());
	}

	m_pInstrument->SetSampleIndex(m_iOctave, m_iSelectedKey, Sample);

	UpdateCurrentKey();
}

const CDSample *CInstrumentEditorDPCM::GetSelectedSample()
{
	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	int Index = pSampleListCtrl->GetSelectionMark();

	if (Index == -1)
		return NULL;

	TCHAR Text[256];
	pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	Index = _tstoi(Text);
	
	return GetDocument()->GetSample(Index);
}

void CInstrumentEditorDPCM::SetSelectedSample(CDSample *pSamp) const
{
	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	int Index = pSampleListCtrl->GetSelectionMark();

	if (Index == -1) return;

	TCHAR Text[256];
	pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	Index = _tstoi(Text);
	
	GetDocument()->SetSample(Index, pSamp);
}

void CInstrumentEditorDPCM::OnBnClickedSave()
{
	CString	Path;
	CFile	SampleFile;
	TCHAR	Text[256];

	CListCtrl *pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	int Index = pSampleListCtrl->GetSelectionMark();

	if (Index == -1)
		return;

	pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	Index = _tstoi(Text);
	
	const CDSample *pDSample = GetDocument()->GetSample(Index);

	if (!pDSample) return;

	CString fileFilter = LoadDefaultFilter(IDS_FILTER_DMC, _T(".dmc"));
	CFileDialog SaveFileDialog(FALSE, _T("dmc"), (LPCSTR)pDSample->GetName(), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter);
	
	SaveFileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_DMC);

	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	theApp.GetSettings()->SetPath(SaveFileDialog.GetPathName(), PATH_DMC);

	Path = SaveFileDialog.GetPathName();

	if (!SampleFile.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	SampleFile.Write(pDSample->GetData(), pDSample->GetSize());
	SampleFile.Close();
}

void CInstrumentEditorDPCM::OnBnClickedLoop()
{
	int Pitch = m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey) & 0x0F;

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);

	UpdateCurrentKey();
}

void CInstrumentEditorDPCM::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrument2A03>(pInst);
	ASSERT(m_pInstrument);

	BuildKeyList();
}

BOOL CInstrumentEditorDPCM::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindowVisible()) {
		switch (pMsg->message) {
			case WM_KEYDOWN:
				if (pMsg->wParam == 27)	// Esc
					break;
				if (GetFocus() != GetDlgItem(IDC_DELTA_COUNTER)) {
					// Select DPCM channel
					CFamiTrackerView::GetView()->SelectChannel(4);
					PreviewNote((unsigned char)pMsg->wParam);		// // //
					return TRUE;
				}
				break;
			case WM_KEYUP:
				if (GetFocus() != GetDlgItem(IDC_DELTA_COUNTER)) {
					PreviewRelease((unsigned char)pMsg->wParam);		// // //
					return TRUE;
				}
				break;
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorDPCM::OnBnClickedAdd()
{
	// Add sample to key list	
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));

	int Pitch = pPitchBox->GetCurSel();

	if (GetDocument()->IsSampleUsed(m_iSelectedSample)) {
		m_pInstrument->SetSampleIndex(m_iOctave, m_iSelectedKey, m_iSelectedSample + 1);
		m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		UpdateCurrentKey();
	}

	CListCtrl* pSampleListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	CListCtrl* pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (m_iSelectedKey < NOTE_RANGE && m_iSelectedSample < MAX_DSAMPLES) {
		pSampleListCtrl->SetItemState(m_iSelectedSample, 0, LVIS_FOCUSED | LVIS_SELECTED);
		pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		if (m_iSelectedSample < pSampleListCtrl->GetItemCount() - 1)
			m_iSelectedSample++;
		m_iSelectedKey++;
		pSampleListCtrl->SetSelectionMark(m_iSelectedSample);
		pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		pSampleListCtrl->SetItemState(m_iSelectedSample, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CInstrumentEditorDPCM::OnBnClickedRemove()
{
	CListCtrl *pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	// Remove sample from key list
	m_pInstrument->SetSampleIndex(m_iOctave, m_iSelectedKey, 0);
	UpdateCurrentKey();

	if (m_iSelectedKey > 0) {
		pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_iSelectedKey--;
		pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}


void CInstrumentEditorDPCM::OnEnChangeLoopPoint()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CInstrumentEditPanel::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here

	/*
	int Pitch = GetDlgItemInt(IDC_LOOP_POINT, 0, 0);
	m_pInstrument->SetSampleLoopOffset(m_iOctave, m_iSelectedKey, Pitch);
	*/
}

void CInstrumentEditorDPCM::OnBnClickedEdit()
{
	const CDSample *pSample = GetSelectedSample();
	if (pSample == NULL)
		return;

	CSampleEditorDlg Editor(this, new CDSample(*pSample));		// // // copy sample here

	INT_PTR nRes = Editor.DoModal();
	
	if (nRes == IDOK) {
		// Save edited sample
		SetSelectedSample(new CDSample(*Editor.GetDSample()));		// // //
		GetDocument()->SetModifiedFlag();
		GetDocument()->SetExceededFlag();		// // //
	}

	// Update sample list
	BuildSampleList();
}

void CInstrumentEditorDPCM::OnNMDblclkSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Preview sample when double-clicking the sample list
	OnBnClickedPreview();
	*pResult = 0;
}

void CInstrumentEditorDPCM::OnBnClickedPreview()
{
	const CDSample *pSample = GetSelectedSample();
	if (pSample != NULL)
		theApp.GetSoundGenerator()->PreviewSample(pSample, 0, 15);
}

void CInstrumentEditorDPCM::OnNMRClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	CMenu *pPopupMenu, PopupMenuBar;
	CPoint point;

	GetCursorPos(&point);
	PopupMenuBar.LoadMenu(IDR_SAMPLES_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);
	pPopupMenu->SetDefaultItem(IDC_PREVIEW);
	pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMRClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Create a popup menu for key list with samples
	CListCtrl *pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	
	m_iSelectedKey = GET_NOTE(pTableListCtrl->GetSelectionMark()) - 1;		// // //
	m_iOctave = GET_OCTAVE(pTableListCtrl->GetSelectionMark());

	CPoint point;
	CMenu PopupMenu;
	GetCursorPos(&point);
	PopupMenu.CreatePopupMenu();
	PopupMenu.AppendMenu(MF_STRING, 1, NO_SAMPLE_STR);

	// Fill menu
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (const CDSample *pDSample = GetDocument()->GetSample(i)) {
			PopupMenu.AppendMenu(MF_STRING, i + 2, pDSample->GetName());
		}
	}

	UINT Result = PopupMenu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this);

	if (Result == 1) {
		// Remove sample
		m_pInstrument->SetSampleIndex(m_iOctave, m_iSelectedKey, 0);
		UpdateCurrentKey();
	}
	else if (Result > 1) {
		// Add sample
		CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
		int Pitch = pPitchBox->GetCurSel();
		m_pInstrument->SetSampleIndex(m_iOctave, m_iSelectedKey, Result - 1);
		m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		UpdateCurrentKey();
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMDblclkTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Preview sample from key table

	if (int Sample = m_pInstrument->GetSampleIndex(m_iOctave, m_iSelectedKey)) {		// // //
		const CDSample *pSample = GetDocument()->GetSample(Sample - 1);
		int Pitch = m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey) & 0x0F;

		CListCtrl *pTableListCtrl = static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

		if (pSample && pSample->GetSize() > 0U &&
			pTableListCtrl->GetItemText(MIDI_NOTE(m_iOctave, m_iSelectedKey + 1), 2) != NO_SAMPLE_STR)
			theApp.GetSoundGenerator()->PreviewSample(pSample, 0, Pitch);
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnEnChangeDeltaCounter()
{
	BOOL Trans;

	if (!m_pInstrument)
		return;

	int Value = GetDlgItemInt(IDC_DELTA_COUNTER, &Trans, FALSE);

	if (Trans == TRUE) {
		if (Value < 0)
			Value = 0;
		if (Value > 127)
			Value = 127;
	}
	else {
		Value = -1;
	}

	if (m_pInstrument->GetSampleDeltaValue(m_iOctave, m_iSelectedKey) != Value)
		m_pInstrument->SetSampleDeltaValue(m_iOctave, m_iSelectedKey, Value);
}

void CInstrumentEditorDPCM::OnDeltaposDeltaSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	CEdit *pDeltaValue = static_cast<CEdit*>(GetDlgItem(IDC_DELTA_COUNTER));

	int Value = 0;

	if (!m_pInstrument)
		return;

	if (pNMUpDown->iPos <= 0 && pNMUpDown->iDelta < 0) {
		pDeltaValue->SetWindowText(_T("Off"));
		Value = -1;
	}
	else {
		Value = pNMUpDown->iPos + pNMUpDown->iDelta;
		if (Value < 0)
			Value = 0;
		if (Value > 127)
			Value = 127;
		SetDlgItemInt(IDC_DELTA_COUNTER, Value, FALSE);
	}

	m_pInstrument->SetSampleDeltaValue(m_iOctave, m_iSelectedKey, Value);

	*pResult = 0;
}
