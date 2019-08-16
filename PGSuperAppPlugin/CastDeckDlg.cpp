///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "CastDeckDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// CCastDeckDlg dialog

IMPLEMENT_DYNAMIC(CCastDeckDlg, CDialog)

CCastDeckDlg::CCastDeckDlg(LPCTSTR lpszTitle,const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CCastDeckDlg::IDD, pParent)
{
   m_strTitle = lpszTitle;
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;

   EventIndexType castDeckEventIdx = m_TimelineMgr.GetCastDeckEventIndex();
   if (m_EventIndex != castDeckEventIdx)
   {
      // initializes the cast deck activity in the target event with the
      // current cast deck activity details
      auto& castDeckActivity = m_TimelineMgr.GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity();
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->SetCastDeckActivity(castDeckActivity);
   }

   m_bExpanded = FALSE;
}

CCastDeckDlg::~CCastDeckDlg()
{
}

CCastDeckActivity CCastDeckDlg::GetCastDeckActivity(CDataExchange* pTheDX)
{
   CDataExchange dx(this, TRUE);
   CDataExchange* pDX = (pTheDX ? pTheDX : &dx);

   CCastDeckActivity castDeckActivity;
   castDeckActivity.Enable();

   int iCastingType;
   DDX_Radio(pDX, IDC_CAST_DECK_1, iCastingType);
   CCastDeckActivity::CastingType castingType = (CCastDeckActivity::CastingType)iCastingType;
   castDeckActivity.SetCastingType(castingType);

   Float64 age;
   DDX_Text(pDX, IDC_AGE, age);
   castDeckActivity.SetConcreteAgeAtContinuity(age);

   Float64 cure_duration;
   DDX_Text(pDX, IDC_CURING, cure_duration);
   castDeckActivity.SetCuringDuration(cure_duration);

   if (castingType == CCastDeckActivity::Staged)
   {
      Float64 time_between_casting;
      DDX_Text(pDX, IDC_TIME_BETWEEN_CASTING, time_between_casting);
      castDeckActivity.SetTimeBetweenCasting(time_between_casting);

      m_ctrlGrid.GetData(castDeckActivity);

      pgsTypes::DeckCastingRegionBoundary boundary;
      DDX_CBItemData(pDX, IDC_REGION_BOUNDARY, boundary);
      castDeckActivity.SetDeckCastingRegionBoundary(boundary);
   }

   return castDeckActivity;
}

void CCastDeckDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of the dialog
      CCastDeckActivity castDeckActivity = GetCastDeckActivity(pDX);

      DDV_GreaterThanZero(pDX,IDC_AGE,castDeckActivity.GetConcreteAgeAtContinuity());
      DDV_NonNegativeDouble(pDX,IDC_CURING,castDeckActivity.GetCuringDuration());

      if (castDeckActivity.GetCastingType() == CCastDeckActivity::Staged)
      {
         DDV_NonNegativeDouble(pDX, IDC_TIME_BETWEEN_CASTING, castDeckActivity.GetTimeBetweenCasting());
      }

      m_TimelineMgr.GetEventByIndex(m_EventIndex)->SetCastDeckActivity(castDeckActivity);
      int result = m_TimelineMgr.ValidateEvent(m_TimelineMgr.GetEventByIndex(m_EventIndex));
      if (result != TLM_SUCCESS)
      {
         CString strProblem = m_TimelineMgr.GetErrorMessage(result);
         CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));
         CString strMsg;
         strMsg.Format(_T("%s\n\n%s"), strProblem, strRemedy);
         if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDYES)
         {
            result = m_TimelineMgr.SetCastDeckEventByIndex(m_EventIndex, true);
            ATLASSERT(result == TLM_SUCCESS);
         }
         else
         {
            pDX->Fail();
         }
      }
   }
   else
   {
      // Data going into the dialog
      CCastDeckActivity castDeckActivity = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity();

      int iCastingType = (int)(castDeckActivity.GetCastingType());
      DDX_Radio(pDX, IDC_CAST_DECK_1, iCastingType);

      Float64 age = castDeckActivity.GetConcreteAgeAtContinuity();
      DDX_Text(pDX,IDC_AGE,age);

      Float64 cure_duration = castDeckActivity.GetCuringDuration();
      DDX_Text(pDX,IDC_CURING,cure_duration);

      Float64 time_between_casting = castDeckActivity.GetTimeBetweenCasting();
      DDX_Text(pDX, IDC_TIME_BETWEEN_CASTING, time_between_casting);

      m_ctrlGrid.SetData(castDeckActivity);

      pgsTypes::DeckCastingRegionBoundary boundary = castDeckActivity.GetDeckCastingRegionBoundary();
      DDX_CBItemData(pDX, IDC_REGION_BOUNDARY, boundary);
   }
}


BEGIN_MESSAGE_MAP(CCastDeckDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, &CCastDeckDlg::OnHelp)
   ON_CONTROL_RANGE(BN_CLICKED, IDC_CAST_DECK_1, IDC_CAST_DECK_2, OnBnClickedCastDeck)
   ON_CBN_SELCHANGE(IDC_REGION_BOUNDARY, OnDeckRegionsChanged)
END_MESSAGE_MAP()

BOOL CCastDeckDlg::OnInitDialog()
{
   m_ctrlGrid.SubclassDlgItem(IDC_DECK_REGION_GRID, this);
   m_ctrlGrid.CustomInit();

   m_ctrlDeckRegion.SubclassDlgItem(IDC_DECK_REGIONS, this);

   FillRegionBoundaryControl();

   CDialog::OnInitDialog();

   // this would be better if we didn't need to access the global
   // data with IBridge and the broker. However we can't get what
   // we needed from m_TimelineManager.GetBridgeDescription(). The determination
   // of cantilevers is not part of the basic bridge description
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   if (nSpans == 1)
   {
      BOOL bUseBack1, bUseAhead1, bUseBack2, bUseAhead2;
      m_ctrlGrid.GetPierUsage(0, pBridge, &bUseBack1, &bUseAhead1);
      PierIndexType nPiers = pBridge->GetPierCount();
      m_ctrlGrid.GetPierUsage(nPiers - 1, pBridge, &bUseBack2, &bUseAhead2);
      if (bUseBack1 == FALSE && bUseAhead1 == FALSE && bUseBack2 == FALSE && bUseAhead2 == FALSE)
      {
         // single span, not cantilevers... there is only a positive moment region
         // disable staged casting option
         GetDlgItem(IDC_CAST_DECK_2)->EnableWindow(FALSE);
         CheckDlgButton(IDC_CAST_DECK_1, BST_CHECKED);
      }
   }
   

   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_CAST_DECK_1)->EnableWindow(FALSE);
      GetDlgItem(IDC_CAST_DECK_2)->EnableWindow(FALSE);

      GetDlgItem(IDC_AGE)->EnableWindow(FALSE);
      GetDlgItem(IDC_AGE_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_CURING)->EnableWindow(FALSE);
      GetDlgItem(IDC_CURING_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_TIME_BETWEEN_CASTING)->EnableWindow(FALSE);

      // hide the ok button, change Cancel to Close and make Close the default button
      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   SetWindowText(m_strTitle);

   UpdateControls(IsDlgButtonChecked(IDC_CAST_DECK_1) ? CCastDeckActivity::Continuous : CCastDeckActivity::Staged);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


// CCastDeckDlg message handlers

void CCastDeckDlg::OnHelp()
{
   // TODO: Add your control notification handler code here
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_CAST_DECK);
}

void CCastDeckDlg::OnBnClickedCastDeck(UINT nIDC)
{
   CCastDeckActivity::CastingType castingType = (CCastDeckActivity::CastingType)(nIDC - IDC_CAST_DECK_1);
   UpdateControls(castingType);
   m_ctrlDeckRegion.Invalidate();
   m_ctrlDeckRegion.UpdateWindow();
   OnDeckRegionsChanged();
}

void CCastDeckDlg::OnDeckRegionsChanged()
{
   m_ctrlDeckRegion.Invalidate();
   m_ctrlDeckRegion.UpdateWindow();
}

void CCastDeckDlg::UpdateControls(CCastDeckActivity::CastingType castingType)
{
   int nCmdShow = (castingType == CCastDeckActivity::Continuous ? SW_HIDE : SW_SHOW);
   GetDlgItem(IDC_TIME_BETWEEN_CASTING_LABEL)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_TIME_BETWEEN_CASTING)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_TIME_BETWEEN_CASTING_UNIT)->ShowWindow(nCmdShow);
   m_ctrlGrid.ShowWindow(nCmdShow);
   GetDlgItem(IDC_PLACEMENT_REGION_LABEL)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_REGION_BOUNDARY_LABEL)->ShowWindow(nCmdShow);
   GetDlgItem(IDC_REGION_BOUNDARY)->ShowWindow(nCmdShow);
}

void CCastDeckDlg::FillRegionBoundaryControl()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REGION_BOUNDARY);
   pCB->SetItemData(pCB->AddString(_T("normal to the alignment")),(DWORD_PTR)(pgsTypes::dcrbNormalToAlignment));
   pCB->SetItemData(pCB->AddString(_T("parallel to the piers")),(DWORD_PTR)(pgsTypes::dcrbParallelToPier));
}
