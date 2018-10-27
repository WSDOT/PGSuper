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
// EditTimelineDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "EditTimelineDlg.h"
#include "TimelineEventDlg.h"
#include <EAF\EAFDocument.h>
#include <IFace\Project.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CEditTimelineDlg dialog

IMPLEMENT_DYNAMIC(CEditTimelineDlg, CDialog)

CEditTimelineDlg::CEditTimelineDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CEditTimelineDlg::IDD, pParent)
{

}

CEditTimelineDlg::~CEditTimelineDlg()
{
}

void CEditTimelineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      int result = m_TimelineManager.Validate();
      if ( result != TLM_SUCCESS )
      {
         pDX->PrepareCtrl(IDC_GRID);
         CString strMsg = m_TimelineManager.GetErrorMessage(result);
         strMsg += _T("\r\n\r\nPlease correct the timeline.");
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CEditTimelineDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &CEditTimelineDlg::OnAddEvent)
   ON_BN_CLICKED(IDC_REMOVE, &CEditTimelineDlg::OnRemoveEvent)
   ON_BN_CLICKED(ID_HELP, &CEditTimelineDlg::OnHelp)
END_MESSAGE_MAP()


// CEditTimelineDlg message handlers

BOOL CEditTimelineDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   BOOL bReadOnly = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? FALSE : TRUE);

   m_Grid.SubclassDlgItem(IDC_GRID, this);
   m_Grid.CustomInit(bReadOnly);
   
   CDialog::OnInitDialog();

   m_Grid.Refresh();

   if ( bReadOnly )
   {
      GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_REMOVE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditTimelineDlg::OnAddEvent()
{
   CTimelineEventDlg dlg(m_TimelineManager,INVALID_INDEX/*new event*/,TRUE/*want to edit the details*/);
   if ( dlg.DoModal() == IDOK )
   {
      bool bDone = false;
      bool bAdjustTimeline = false;
      while ( !bDone )
      {
         EventIndexType eventIdx;
         int result = m_TimelineManager.AddTimelineEvent(*dlg.m_pTimelineEvent,bAdjustTimeline,&eventIdx);
         if ( result == TLM_SUCCESS )
         {
            bDone = true;
            m_Grid.Refresh();
         }
         else
         {
            CString strProblem;
            if (result == TLM_OVERLAPS_PREVIOUS_EVENT )
            {
               strProblem = _T("This event begins before the activities in the previous event have completed.");
            }
            else
            {
               strProblem = _T("The activities in this event end after the next event begins.");
            }

            CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

            CString strMsg;
            strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
            if ( AfxMessageBox(strMsg,MB_YESNO | MB_ICONQUESTION) == IDYES )
            {
               bAdjustTimeline = true;
            }
            else
            {
               return;
            }
         }
      }
   }
}

void CEditTimelineDlg::OnRemoveEvent()
{
   m_Grid.RemoveEvents();
   m_Grid.Refresh();
}

void CEditTimelineDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_TIMELINE_MANAGER);
}
