///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// DuctEntryDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "DuctEntryDlg.h"
#include "..\htmlhelp\HelpTopics.hh"
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDuctEntryDlg dialog


CDuctEntryDlg::CDuctEntryDlg(bool allowEditing,
                                         CWnd* pParent /*=NULL*/)
	: CDialog(CDuctEntryDlg::IDD, pParent),
   m_AllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CConnectionEntryDlg)
	m_Name = _T("");
	//}}AFX_DATA_INIT
}


void CDuctEntryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_MetaFileStatic(pDX, IDC_ZDIM, m_DuctPicture,_T("ZDIM"), _T("Metafile"), EMF_FIT );

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_OD, IDC_OD_UNIT, m_OD, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_OD, m_OD, pDisplayUnits->ComponentDim );

   DDX_UnitValueAndTag(pDX, IDC_ID, IDC_ID_UNIT, m_ID, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_ID, m_ID, pDisplayUnits->ComponentDim );

   DDX_UnitValueAndTag(pDX, IDC_Z, IDC_Z_UNIT, m_Z, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_Z, m_Z, pDisplayUnits->ComponentDim );

   //{{AFX_DATA_MAP(CConnectionEntryDlg)
	DDX_Text(pDX, IDC_NAME, m_Name);
	//}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
   {
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Duct Name cannot be blank"));
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CDuctEntryDlg, CDialog)
	//{{AFX_MSG_MAP(CDuctEntryDlg)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDuctEntryDlg message handlers
LRESULT CDuctEntryDlg::OnCommandHelp(WPARAM, LPARAM lParam)
{
#pragma Reminder("UPDATE: need to add help topic")
   AfxMessageBox(_T("Implement help topic"));
   //::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DUCT_DIALOG );
   return TRUE;
}


BOOL CDuctEntryDlg::OnInitDialog() 
{
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Name;
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += _T(" (Read Only)");
   }
   SetWindowText(head);

   CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
