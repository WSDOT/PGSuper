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
// GeometryControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GeometryControlDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CGeometryControlDlg dialog

IMPLEMENT_DYNAMIC(CGeometryControlDlg, CDialog)

CGeometryControlDlg::CGeometryControlDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CGeometryControlDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;
}

CGeometryControlDlg::~CGeometryControlDlg()
{
}

void CGeometryControlDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of the dialog
      CButton* pButG = (CButton*)GetDlgItem(IDC_GCE_CHECK);
      CButton* pBut1 = (CButton*)GetDlgItem(IDC_RADIO1);
      if (pButG->GetCheck() == BST_CHECKED)
      {
         m_GeometryControlActivityType = pgsTypes::gcaGeometryControlEvent;
      }
      else if (pBut1->GetCheck() == BST_CHECKED)
      {
         m_GeometryControlActivityType = pgsTypes::gcaSpecCheckEvent;
      }
      else
      {
         m_GeometryControlActivityType = pgsTypes::gcaGeometryReportingEvent;
      }

      CGeometryControlActivity geometryControlActivity;
      geometryControlActivity.SetGeometryControlEventType(m_GeometryControlActivityType);

      m_TimelineMgr.GetEventByIndex(m_EventIndex)->SetGeometryControlActivity(geometryControlActivity);
      int result = m_TimelineMgr.ValidateEvent(m_TimelineMgr.GetEventByIndex(m_EventIndex));
      if (result != TLM_SUCCESS)
      {
         pDX->Fail();
      }
   }
   else
   {
      // Data going into the dialog
      const CGeometryControlActivity& GeometryControlActivity = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetGeometryControlActivity();
      m_GeometryControlActivityType = GeometryControlActivity.GetGeometryControlEventType();

      if (m_GeometryControlActivityType == pgsTypes::gcaGeometryControlEvent)
      {
         CButton* pBut = (CButton*)GetDlgItem(IDC_GCE_CHECK);
         pBut->SetCheck(BST_CHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO1);
         pBut->SetCheck(BST_CHECKED);
         pBut->EnableWindow(FALSE);
         pBut = (CButton*)GetDlgItem(IDC_RADIO2);
         pBut->SetCheck(BST_CHECKED);
         pBut->EnableWindow(FALSE);
      }
      else if (m_GeometryControlActivityType == pgsTypes::gcaSpecCheckEvent)
      {
         CButton* pBut = (CButton*)GetDlgItem(IDC_GCE_CHECK);
         pBut->SetCheck(BST_UNCHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO1);
         pBut->SetCheck(BST_CHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO2);
         pBut->SetCheck(BST_UNCHECKED);
      }
      else
      {
         // Default to report only
         CButton* pBut = (CButton*)GetDlgItem(IDC_GCE_CHECK);
         pBut->SetCheck(BST_UNCHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO1);
         pBut->SetCheck(BST_UNCHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO2);
         pBut->SetCheck(BST_CHECKED);
      }
   }
}


BEGIN_MESSAGE_MAP(CGeometryControlDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, &CGeometryControlDlg::OnHelp)
   ON_BN_CLICKED(IDC_RADIO1,&CGeometryControlDlg::OnBnClickedRadio1)
   ON_BN_CLICKED(IDC_GCE_CHECK,&CGeometryControlDlg::OnBnClickedGceCheck)
   ON_BN_CLICKED(IDC_RADIO2,&CGeometryControlDlg::OnBnClickedRadio2)
END_MESSAGE_MAP()

BOOL CGeometryControlDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
      GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
      GetDlgItem(IDC_GCE_CHECK)->EnableWindow(FALSE);

      // hide the ok button, change Cancel to Close and make Close the default button
      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


// CGeometryControlDlg message handlers

void CGeometryControlDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_GEOMETRY_CONTOL_EVENT);
}

void CGeometryControlDlg::OnBnClickedGceCheck()
{
   CButton* pBut = (CButton*)GetDlgItem(IDC_GCE_CHECK);
   if (pBut->GetCheck() == BST_CHECKED)
   {
      pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      pBut->EnableWindow(FALSE);
      pBut = (CButton*)GetDlgItem(IDC_RADIO2);
      pBut->SetCheck(BST_CHECKED);
      pBut->EnableWindow(FALSE);
   }
   else
   {
      pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      pBut->EnableWindow(TRUE);
      pBut = (CButton*)GetDlgItem(IDC_RADIO2);
      pBut->SetCheck(BST_UNCHECKED);
      pBut->EnableWindow(TRUE);
   }
}

void CGeometryControlDlg::OnBnClickedRadio1()
{
}

void CGeometryControlDlg::OnBnClickedRadio2()
{
}