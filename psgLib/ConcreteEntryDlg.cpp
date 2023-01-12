///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// NOTE: Duplicate code warning
//
// This dialog along with all its property pages are basically repeated in
// the PGSuperAppPlugin project. I could not get a single implementation to
// work because of issues with the module resources.
//
// If changes are made here, the same changes are likely needed in
// the other location.



// ConcreteEntryDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include <MfcTools\CustomDDX.h>
#include <Colors.h>
#include <System\Tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog


CConcreteEntryDlg::CConcreteEntryDlg(bool allowEditing,CWnd* pParent /*=nullptr*/,UINT iSelectPage /*=0*/)
	: CPropertySheet(_T("Concrete Details"),pParent,iSelectPage),
   m_bAllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CConcreteEntryDlg)
	//}}AFX_DATA_INIT
   Init();
}

CConcreteEntryDlg::~CConcreteEntryDlg()
{
}

BEGIN_MESSAGE_MAP(CConcreteEntryDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CConcreteEntryDlg)
	ON_MESSAGE(WM_KICKIDLE,&CConcreteEntryDlg::OnKickIdle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CConcreteEntryDlg::OnInitDialog()
{
   CPropertySheet::OnInitDialog();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_CONCRETE_ENTRY),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_General.m_EntryName;
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConcreteEntryDlg::Init()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags  |= PSP_HASHELP;
   m_PCIUHPC.m_psp.dwFlags |= PSP_HASHELP;
   m_AASHTO.m_psp.dwFlags   |= PSP_HASHELP;
   m_ACI.m_psp.dwFlags      |= PSP_HASHELP;
   m_CEBFIP.m_psp.dwFlags   |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_AASHTO);
   AddPage(&m_PCIUHPC);
   AddPage(&m_ACI);
   AddPage(&m_CEBFIP);
}
