///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
// CastDeckDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "CastDeckDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>



// CCastDeckDlg dialog

IMPLEMENT_DYNAMIC(CCastDeckDlg, CDialog)

CCastDeckDlg::CCastDeckDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CCastDeckDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;
}

CCastDeckDlg::~CCastDeckDlg()
{
}

void CCastDeckDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of the dialog
      m_TimelineMgr.SetCastDeckEventByIndex(m_EventIndex,true);
      m_EventIndex = m_TimelineMgr.GetCastDeckEventIndex();

      Float64 age;
      DDX_Text(pDX,IDC_AGE,age);
      DDV_GreaterThanZero(pDX,IDC_AGE,age);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().SetConcreteAgeAtContinuity(age);

      Float64 cure_duration;
      DDX_Text(pDX,IDC_CURING,cure_duration);
      DDV_NonNegativeDouble(pDX,IDC_CURING,cure_duration);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().SetCuringDuration(cure_duration);
   }
   else
   {
      // Data going into the dialog
      Float64 age = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().GetConcreteAgeAtContinuity();
      DDX_Text(pDX,IDC_AGE,age);

      Float64 cure_duration = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().GetCuringDuration();
      DDX_Text(pDX,IDC_CURING,cure_duration);
   }
}


BEGIN_MESSAGE_MAP(CCastDeckDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, &CCastDeckDlg::OnHelp)
END_MESSAGE_MAP()

BOOL CCastDeckDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_AGE)->EnableWindow(FALSE);
      GetDlgItem(IDC_AGE_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_CURING)->EnableWindow(FALSE);
      GetDlgItem(IDC_CURING_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


// CCastDeckDlg message handlers

void CCastDeckDlg::OnHelp()
{
   // TODO: Add your control notification handler code here
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_CAST_DECK);
}
