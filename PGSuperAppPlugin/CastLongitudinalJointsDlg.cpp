///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// CastLongitudinalJointsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "CastLongitudinalJointsDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// CCastLongitudinalJointsDlg dialog

IMPLEMENT_DYNAMIC(CCastLongitudinalJointsDlg, CDialog)

CCastLongitudinalJointsDlg::CCastLongitudinalJointsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CCastLongitudinalJointsDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;
}

CCastLongitudinalJointsDlg::~CCastLongitudinalJointsDlg()
{
}

void CCastLongitudinalJointsDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of the dialog
      m_TimelineMgr.SetCastDeckEventByIndex(m_EventIndex,true);
      m_EventIndex = m_TimelineMgr.GetCastLongitudinalJointEventIndex();

      Float64 age;
      DDX_Text(pDX,IDC_AGE,age);
      DDV_GreaterThanZero(pDX,IDC_AGE,age);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastLongitudinalJointActivity().SetTotalCuringDuration(age);

      Float64 cure_duration;
      DDX_Text(pDX,IDC_CURING,cure_duration);
      DDV_NonNegativeDouble(pDX,IDC_CURING,cure_duration);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastLongitudinalJointActivity().SetActiveCuringDuration(cure_duration);
   }
   else
   {
      // Data going into the dialog
      Float64 age = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastLongitudinalJointActivity().GetTotalCuringDuration();
      DDX_Text(pDX,IDC_AGE,age);

      Float64 cure_duration = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastLongitudinalJointActivity().GetActiveCuringDuration();
      DDX_Text(pDX,IDC_CURING,cure_duration);
   }
}


BEGIN_MESSAGE_MAP(CCastLongitudinalJointsDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, &CCastLongitudinalJointsDlg::OnHelp)
END_MESSAGE_MAP()

BOOL CCastLongitudinalJointsDlg::OnInitDialog()
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


// CCastLongitudinalJointsDlg message handlers

void CCastLongitudinalJointsDlg::OnHelp()
{
   // TODO: Add your control notification handler code here
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_CAST_LONGITUDINAL_JOINTS);
}
