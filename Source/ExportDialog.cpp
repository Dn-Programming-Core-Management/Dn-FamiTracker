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

/*
 * This is the NSF (and other types) export dialog
 *
 */

#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTracker.h"
#include "ExportDialog.h"
#include "FamitrackerDoc.h"
#include "Compiler.h"
#include "Settings.h"
#include "CustomExporters.h"
#include "DocumentWrapper.h"
#include "MainFrm.h"

// Define internal exporters
const LPTSTR CExportDialog::DEFAULT_EXPORT_NAMES[] = {
	_T("NSF - Nintendo Sound File"),
	_T("NES - iNES ROM image"),
	_T("BIN - Raw music data"),
	_T("PRG - Clean 32kB ROM image"),
	_T("ASM - Assembly source"),
	_T("NSFe - Extended Nintendo Sound File"),		// // //
};

const exportFunc_t CExportDialog::DEFAULT_EXPORT_FUNCS[] = {
	&CExportDialog::CreateNSF,
	&CExportDialog::CreateNES,
	&CExportDialog::CreateBIN,
	&CExportDialog::CreatePRG,
	&CExportDialog::CreateASM,
	&CExportDialog::CreateNSFe,		// // //
};

const int CExportDialog::DEFAULT_EXPORTERS = 6;		// // //

// Remember last option when dialog is closed
int CExportDialog::m_iExportOption = 0;

// File filters
LPCTSTR CExportDialog::NSF_FILTER[]   = { _T("NSF file (*.nsf)"), _T(".nsf") };
LPCTSTR CExportDialog::NES_FILTER[]   = { _T("NES ROM image (*.nes)"), _T(".nes") };
LPCTSTR CExportDialog::RAW_FILTER[]   = { _T("Raw song data (*.bin)"), _T(".bin") };
LPCTSTR CExportDialog::DPCMS_FILTER[] = { _T("DPCM sample bank (*.bin)"), _T(".bin") };
LPCTSTR CExportDialog::PRG_FILTER[]   = { _T("NES program bank (*.prg)"), _T(".prg") };
LPCTSTR CExportDialog::ASM_FILTER[]	  = { _T("Assembly text (*.asm)"), _T(".asm") };
LPCTSTR CExportDialog::NSFE_FILTER[]  = { _T("NSFe file (*.nsfe)"), _T(".nsfe") };		// // //

// Compiler logger

class CEditLog : public CCompilerLog
{
public:
	CEditLog(CWnd *pEdit) : m_pEdit(static_cast<CEdit*>(pEdit)) {};
	void WriteLog(LPCTSTR text);
	void Clear();
private:
	CEdit *m_pEdit;
};

void CEditLog::WriteLog(LPCTSTR text)
{
	int Len = m_pEdit->GetWindowTextLength();
	m_pEdit->SetSel(Len, Len, 0);
	m_pEdit->ReplaceSel(text, 0);
	m_pEdit->RedrawWindow();
}

void CEditLog::Clear()
{
	m_pEdit->SetWindowText(_T(""));
	m_pEdit->RedrawWindow();
}

// CExportDialog dialog

IMPLEMENT_DYNAMIC(CExportDialog, CDialog)
CExportDialog::CExportDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CExportDialog::IDD, pParent)
{
}

CExportDialog::~CExportDialog()
{
}

void CExportDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExportDialog, CDialog)
	ON_BN_CLICKED(IDC_CLOSE, OnBnClickedClose)
	ON_BN_CLICKED(IDC_EXPORT, &CExportDialog::OnBnClickedExport)
	ON_BN_CLICKED(IDC_PLAY, OnBnClickedPlay)
END_MESSAGE_MAP()


// CExportDialog message handlers

void CExportDialog::OnBnClickedClose()
{
	EndDialog(0);
}

BOOL CExportDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());

	// Check PAL button if it's a PAL song
	if (pDoc->GetMachine() == PAL) {
		CheckDlgButton(IDC_NTSC, 0);
		CheckDlgButton(IDC_PAL, 1);
		CheckDlgButton(IDC_DUAL, 0);
	}
	else {
		CheckDlgButton(IDC_NTSC, 1);
		CheckDlgButton(IDC_PAL, 0);
		CheckDlgButton(IDC_DUAL, 0);
	}

	SetDlgItemText(IDC_NAME, CString(pDoc->GetSongName()));
	SetDlgItemText(IDC_ARTIST, CString(pDoc->GetSongArtist()));
	SetDlgItemText(IDC_COPYRIGHT, CString(pDoc->GetSongCopyright()));

	// Fill the export box
	CComboBox *pTypeBox = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));

	// Add built in exporters
	for (int i = 0; i < DEFAULT_EXPORTERS; ++i)
		pTypeBox->AddString(DEFAULT_EXPORT_NAMES[i]);

	// Add selections for each custom plugin name
	CStringArray names;
	theApp.GetCustomExporters()->GetNames( names );

	for( int i = 0; i < names.GetCount(); ++i )
		pTypeBox->AddString( names[ i ] );

	// Set default selection
	pTypeBox->SetCurSel(m_iExportOption);

#ifdef _DEBUG
	GetDlgItem(IDC_PLAY)->ShowWindow(SW_SHOW);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportDialog::OnBnClickedExport()
{
	CComboBox *pTypeCombo = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));
	CString ItemText;

	m_iExportOption = pTypeCombo->GetCurSel();
	pTypeCombo->GetLBText(m_iExportOption, ItemText);

	// Check built in exporters
	for (int i = 0; i < DEFAULT_EXPORTERS; ++i) {
		if (!ItemText.Compare(DEFAULT_EXPORT_NAMES[i])) {
			(this->*DEFAULT_EXPORT_FUNCS[i])();
			return;
		}
	}

	//selection is the name of a custom exporter
	CreateCustom( ItemText );
}

void CExportDialog::CreateNSF()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CString	DefFileName = pDoc->GetFileTitle();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));
	CString Name, Artist, Copyright;
	CString filter = LoadDefaultFilter(NSF_FILTER[0], NSF_FILTER[1]);
	int MachineType = 0;

	// Collect header info
	GetDlgItemText(IDC_NAME, Name);
	GetDlgItemText(IDC_ARTIST, Artist);
	GetDlgItemText(IDC_COPYRIGHT, Copyright);

	if (IsDlgButtonChecked(IDC_NTSC) != 0)
		MachineType = 0;
	else if (IsDlgButtonChecked(IDC_PAL) != 0)
		MachineType = 1;
	else if (IsDlgButtonChecked(IDC_DUAL) != 0)
		MachineType = 2;

	USES_CONVERSION;

	pDoc->SetSongName(T2A(Name.GetBuffer()));
	pDoc->SetSongArtist(T2A(Artist.GetBuffer()));
	pDoc->SetSongCopyright(T2A(Copyright.GetBuffer()));

	CFileDialog FileDialog(FALSE, NSF_FILTER[1], DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportNSF(FileDialog.GetPathName(), MachineType);

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateNSFe()		// // //
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CString	DefFileName = pDoc->GetFileTitle();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));
	CString Name, Artist, Copyright;
	CString filter = LoadDefaultFilter(NSFE_FILTER[0], NSFE_FILTER[1]);
	int MachineType = 0;

	// Collect header info
	GetDlgItemText(IDC_NAME, Name);
	GetDlgItemText(IDC_ARTIST, Artist);
	GetDlgItemText(IDC_COPYRIGHT, Copyright);

	if (IsDlgButtonChecked(IDC_NTSC) != 0)
		MachineType = 0;
	else if (IsDlgButtonChecked(IDC_PAL) != 0)
		MachineType = 1;
	else if (IsDlgButtonChecked(IDC_DUAL) != 0)
		MachineType = 2;

	USES_CONVERSION;

	pDoc->SetSongName(T2A(Name.GetBuffer()));
	pDoc->SetSongArtist(T2A(Artist.GetBuffer()));
	pDoc->SetSongCopyright(T2A(Copyright.GetBuffer()));

	CFileDialog FileDialog(FALSE, NSF_FILTER[1], DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportNSFE(FileDialog.GetPathName(), MachineType);

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateNES()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CString	DefFileName = pDoc->GetFileTitle();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));
	CString filter = LoadDefaultFilter(NES_FILTER[0], NES_FILTER[1]);

	CFileDialog FileDialog(FALSE, NES_FILTER[1], DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportNES(FileDialog.GetPathName(), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateBIN()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));
	CString MusicFilter = LoadDefaultFilter(RAW_FILTER[0], RAW_FILTER[1]);
	CString DPCMFilter = LoadDefaultFilter(DPCMS_FILTER[0], DPCMS_FILTER[1]);

	const CString DEFAULT_MUSIC_NAME = _T("music.bin");		// // //
	const CString DEFAULT_SAMPLE_NAME = _T("samples.bin");

	CFileDialog FileDialogMusic(FALSE, RAW_FILTER[1], DEFAULT_MUSIC_NAME, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, MusicFilter);
	CFileDialog FileDialogSamples(FALSE, DPCMS_FILTER[1], DEFAULT_SAMPLE_NAME, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, DPCMFilter);

	FileDialogMusic.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialogMusic.DoModal() == IDCANCEL)
		return;

	CString SampleDir = FileDialogMusic.GetPathName();		// // //
	if (pDoc->GetSampleCount() > 0) {
		if (FileDialogSamples.DoModal() == IDCANCEL)
			return;
		SampleDir = FileDialogSamples.GetPathName();
	}
	else {
		int Pos = SampleDir.ReverseFind(_T('\\'));
		ASSERT(Pos != -1);
		SampleDir = SampleDir.Left(Pos + 1) + DEFAULT_SAMPLE_NAME;
		if (PathFileExists(SampleDir)) {
			CString msg;
			AfxFormatString1(msg, IDS_EXPORT_SAMPLES_FILE, DEFAULT_SAMPLE_NAME);
			if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
				return;
		}
	}

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportBIN(FileDialogMusic.GetPathName(), SampleDir);

	theApp.GetSettings()->SetPath(FileDialogMusic.GetPathName(), PATH_NSF);
}

void CExportDialog::CreatePRG()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));
	CString Filter = LoadDefaultFilter(PRG_FILTER[0], PRG_FILTER[1]);

	CFileDialog FileDialog(FALSE, PRG_FILTER[1], _T("music.prg"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportPRG(FileDialog.GetPathName(), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateASM()
{
	// Currently not included
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CCompiler Compiler(pDoc, new CEditLog(GetDlgItem(IDC_OUTPUT)));

	CString Filter = LoadDefaultFilter(ASM_FILTER[0], ASM_FILTER[1]);
	CFileDialog FileDialogMusic(FALSE, ASM_FILTER[1], _T("music.asm"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter);

	FileDialogMusic.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialogMusic.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	CWaitCursor wait;

	Compiler.ExportASM(FileDialogMusic.GetPathName());

	theApp.GetSettings()->SetPath(FileDialogMusic.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateCustom( CString name )
{
	theApp.GetCustomExporters()->SetCurrentExporter( name );

	CString custom_exporter_extension = theApp.GetCustomExporters()->GetCurrentExporter().getExt();
	CString custom_filter_name = CString("Custom Song Data (*") + custom_exporter_extension + CString(")");
	CString default_custom_file_name = CString("music") + custom_exporter_extension;

	CString Filter = LoadDefaultFilter(custom_filter_name, custom_exporter_extension);
	CFileDialog FileDialogCustom(FALSE, custom_exporter_extension, default_custom_file_name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter);

	if(FileDialogCustom.DoModal() == IDCANCEL)
		return;

	CString fileName( FileDialogCustom.GetPathName() );	

	int Track = static_cast<CMainFrame*>(theApp.m_pMainWnd)->GetSelectedTrack();
	CFamiTrackerDocWrapper documentWrapper(CFamiTrackerDoc::GetDoc(), Track);

	if(theApp.GetCustomExporters()->GetCurrentExporter().Export( &documentWrapper, CStringA(fileName) ))
	{
		AfxMessageBox(_T("Successfully exported!"));
	}
}

void CExportDialog::OnBnClickedPlay()
{
#ifdef _DEBUG

//	if (m_strFile.GetLength() == 0)
//		return;

	char *file = "d:\\test.nsf";

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CCompiler Compiler(pDoc, new CEditLog(static_cast<CEdit*>(GetDlgItem(IDC_OUTPUT))));

	Compiler.ExportNSF(file, (IsDlgButtonChecked(IDC_PAL) != 0));

	// Play exported file (available in debug)
	ShellExecute(NULL, _T("open"), file, NULL, NULL, SW_SHOWNORMAL);

#endif
}
