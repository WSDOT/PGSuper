///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDuctEntryDlg dialog


CDuctEntryDlg::CDuctEntryDlg(bool allowEditing,
                                         CWnd* pParent /*=nullptr*/)
	: CDialog(CDuctEntryDlg::IDD, pParent),
   m_bAllowEditing(allowEditing)
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

   DDX_UnitValueAndTag(pDX, IDC_ND, IDC_ND_UNIT, m_ND, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero(pDX, IDC_ND, m_ND, pDisplayUnits->ComponentDim);

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
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDuctEntryDlg message handlers
void CDuctEntryDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DUCT_DIALOG );
}


BOOL CDuctEntryDlg::OnInitDialog() 
{
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Name;
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);

   CDialog::OnInitDialog();
	
   EnableToolTips(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDuctEntryDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_Z:
         m_strTip = _T("LRFD C5.9.1.6\n3\" OD and less, Z = 1/2\"\nOver 3\" OD to 4\", Z = 3/4\"\nOver 4\" OD, Z = 1\"");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}
