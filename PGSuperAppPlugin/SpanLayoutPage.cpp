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

// SpanLayoutPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "SpanLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "EditHaunchDlg.h"
#include "Utilities.h"

#include <IFace\DocumentType.h>

#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage property page

IMPLEMENT_DYNCREATE(CSpanLayoutPage, CPropertyPage)

CSpanLayoutPage::CSpanLayoutPage() : CPropertyPage(CSpanLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_SpanLength = -1;
}

CSpanLayoutPage::~CSpanLayoutPage()
{
}

void CSpanLayoutPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   DDX_Control(pDX, IDC_START_SLAB_OFFSET, m_ctrlStartSlabOffset);
   DDX_Control(pDX,IDC_END_SLAB_OFFSET,m_ctrlEndSlabOffset);
   DDX_Control(pDX,IDC_ASSUMED_EXCESS_CAMBER,m_ctrlAssumedExcessCamber);

   // set units no matter what
   DDX_Tag(pDX,IDC_ASSUMED_EXCESS_CAMBER_UNIT,pDisplayUnits->GetComponentDimUnit());
   DDX_Tag(pDX,IDC_START_SLAB_OFFSET_UNIT,pDisplayUnits->GetComponentDimUnit());
   DDX_Tag( pDX,IDC_END_SLAB_OFFSET_UNIT,pDisplayUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());
   DDV_UnitValueGreaterThanZero(pDX,IDC_SPAN_LENGTH,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());

   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetSpanLength(pParent->m_pSpanData->GetIndex(),m_SpanLength);

      UpdateHaunchAndCamberData(pDX);
            }
            else
            {
      UpdateHaunchAndCamberControls();
   }
}

BEGIN_MESSAGE_MAP(CSpanLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanLayoutPage)
	ON_WM_CTLCOLOR()
	ON_COMMAND(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_EDIT_HAUNCH_BUTTON,&OnBnClickedEditHaunchButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage message handlers

BOOL CSpanLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   m_SpanIdx = pParent->m_pSpanData->GetIndex();
   m_PrevPierIdx = (PierIndexType)m_SpanIdx;
   m_NextPierIdx = m_PrevPierIdx+1;

   m_SpanLength = pParent->m_pSpanData->GetSpanLength();

   CPropertyPage::OnInitDialog();

   const CSpanData2* pPrevSpan = pParent->m_pSpanData->GetPrevPier()->GetPrevSpan();
   const CSpanData2* pNextSpan = pParent->m_pSpanData->GetNextPier()->GetNextSpan();

   CString strPrevPierName = pgsPierLabel::GetPierLabelEx(pParent->m_pSpanData->GetPrevPier()->IsAbutment(), m_PrevPierIdx).c_str();
   CString strNextPierName = pgsPierLabel::GetPierLabelEx(pParent->m_pSpanData->GetNextPier()->IsAbutment(), m_NextPierIdx).c_str();

   CString strPrevPierType = pgsPierLabel::GetPierTypeLabelEx(pParent->m_pSpanData->GetPrevPier()->IsAbutment(), m_PrevPierIdx).c_str();
   CString strNextPierType = pgsPierLabel::GetPierTypeLabelEx(pParent->m_pSpanData->GetNextPier()->IsAbutment(), m_NextPierIdx).c_str();

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %s"),LABEL_SPAN(m_SpanIdx));
   GetDlgItem(IDC_SPAN_LABEL)->SetWindowText(strSpanLabel);

   CString strSpanLengthBasis;
   strSpanLengthBasis.Format(_T("Span length is measured along the alignment between the %s Line at %s and the %s Line at %s."),strPrevPierType,strPrevPierName,strNextPierType,strNextPierName);
   GetDlgItem(IDC_SPAN_LENGTH_BASIS)->SetWindowText(strSpanLengthBasis);

   CString strSpanLengthNote;
   strSpanLengthNote.Format(_T("The length of Span %s is changed by moving all piers after %s. Only the length of Span %s is changed."),LABEL_SPAN(m_SpanIdx),LABEL_PIER_EX(pParent->m_pSpanData->GetPrevPier()->IsAbutment(),m_PrevPierIdx),LABEL_SPAN(m_SpanIdx));
   GetDlgItem(IDC_SPAN_LENGTH_NOTE)->SetWindowText(strSpanLengthNote);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanLayoutPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_SPANDETAILS_GENERAL );
}

void CSpanLayoutPage::UpdateHaunchAndCamberControls()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ISpecification,pSpec);

   pgsTypes::HaunchInputDepthType inputType = pParent->m_BridgeDesc.GetHaunchInputDepthType();

   if (inputType == pgsTypes::hidACamber)
   {
      // Input is via slab offset
      GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Slab Offset (\"A\" Dimension)"));
   }
   else if (inputType == pgsTypes::hidHaunchDirectly)
   {
      GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Haunch Depth"));
   }
   else
   {
      GetDlgItem(IDC_SLAB_OFFSET_LABEL)->SetWindowText(_T("Haunch+Slab Depth"));
   }

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   bool bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();

   if (deckType == pgsTypes::sdtNone)
   {
      EnableHaunchAndCamberControls(FALSE,FALSE,bCanAssumedExcessCamberInputBeEnabled && inputType == pgsTypes::hidACamber);
   }
   else if (inputType == pgsTypes::hidACamber)
   {
      pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();

      if (slabOffsetType == pgsTypes::sotBridge || slabOffsetType == pgsTypes::sotSegment)
      {
         EnableHaunchAndCamberControls(FALSE,TRUE,bCanAssumedExcessCamberInputBeEnabled);
   }
   else
   {
         EnableHaunchAndCamberControls(TRUE,TRUE,bCanAssumedExcessCamberInputBeEnabled);
   }

      // Not all span ends have slab offsets... hide controls if piers don't have it
   const CPierData2* pPrevPier = pParent->GetBridgeDescription()->GetPier(m_PrevPierIdx);
   const CPierData2* pNextPier = pParent->GetBridgeDescription()->GetPier(m_NextPierIdx);
      bool bHasStartSlabOffset = pPrevPier->HasSlabOffset();
      bool bHasEndSlabOffset = pNextPier->HasSlabOffset();

      if (slabOffsetType != pgsTypes::sotSegment)
   {
         Float64 slabOffset;
         CString strSlabOffset;
         if (bHasStartSlabOffset)
         {
            slabOffset = pPrevPier->GetSlabOffset(pgsTypes::Ahead); // will get correct value regardless of slaboffsettype
            strSlabOffset.Format(_T("%s"),FormatDimension(slabOffset,pDisplayUnits->GetComponentDimUnit(),false));
            m_ctrlStartSlabOffset.SetWindowText(strSlabOffset);

   }
   else
   {
      GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_START_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);
   }

         if (bHasEndSlabOffset)
   {
            slabOffset = pNextPier->GetSlabOffset(pgsTypes::Back); // will get correct value regardless of slaboffsettype
            strSlabOffset.Format(_T("%s"),FormatDimension(slabOffset,pDisplayUnits->GetComponentDimUnit(),false));
            m_ctrlEndSlabOffset.SetWindowText(strSlabOffset);
   }
   else
   {
      GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_END_SLAB_OFFSET)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);
   }
   }

      // Camber
      pgsTypes::AssumedExcessCamberType assumedExcessCamberType = pParent->GetBridgeDescription()->GetAssumedExcessCamberType();

      if (bCanAssumedExcessCamberInputBeEnabled && (bHasStartSlabOffset || bHasEndSlabOffset) && assumedExcessCamberType != pgsTypes::aecGirder)
   {
         m_ctrlAssumedExcessCamber.EnableWindow(assumedExcessCamberType == pgsTypes::aecSpan);

         Float64 camber = pParent->GetBridgeDescription()->GetSpan(m_SpanIdx)->GetAssumedExcessCamber(0);
         CString strCamber;
         strCamber.Format(_T("%s"),FormatDimension(camber,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlAssumedExcessCamber.SetWindowText(strCamber);
      }
      }
      else
      {
      // direct haunch input
      const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();
      Float64 Tdeck;
      if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
      {
         Tdeck = pDeck->GrossDepth + pDeck->PanelDepth;
   }
   else
   {
         Tdeck = pDeck->GrossDepth;
   }

      pgsTypes::HaunchInputLocationType haunchInputLocationType = pParent->m_BridgeDesc.GetHaunchInputLocationType();
      pgsTypes::HaunchLayoutType haunchLayoutType = pParent->m_BridgeDesc.GetHaunchLayoutType();
      pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pParent->m_BridgeDesc.GetHaunchInputDistributionType();

      // Disable controls unless we need them
      EnableHaunchAndCamberControls(FALSE,TRUE,false);

      Float64 haunchDepth;
      CString strHaunchVal;
      if (haunchInputLocationType == pgsTypes::hilSame4Bridge && 
        ((haunchInputDistributionType == pgsTypes::hidUniform) ||
         (haunchLayoutType == pgsTypes::hltAlongSpans && haunchInputDistributionType == pgsTypes::hidAtEnds)))
      {
         // Put whole bridge value into disabled controls if needed
         std::vector<Float64> allBridgeHaunches = pParent->m_BridgeDesc.GetDirectHaunchDepths();

         haunchDepth = allBridgeHaunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlStartSlabOffset.SetWindowText(strHaunchVal);

         haunchDepth = allBridgeHaunches.back() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlEndSlabOffset.SetWindowText(strHaunchVal);
      }
      else if (haunchInputLocationType == pgsTypes::hilSame4AllGirders && haunchLayoutType == pgsTypes::hltAlongSpans &&
         haunchInputDistributionType == pgsTypes::hidAtEnds)
      {
         // Data is input at both ends
         EnableHaunchAndCamberControls(TRUE,TRUE,false);

         std::vector<Float64> haunches = pParent->GetBridgeDescription()->GetSpan(m_SpanIdx)->GetDirectHaunchDepths(0);

         haunchDepth = haunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlStartSlabOffset.SetWindowText(strHaunchVal);

         haunchDepth = haunches.back() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlEndSlabOffset.SetWindowText(strHaunchVal);
      }
      else if (haunchInputLocationType == pgsTypes::hilSame4AllGirders && haunchLayoutType == pgsTypes::hltAlongSpans &&
         haunchInputDistributionType == pgsTypes::hidUniform)
      {
         // Data is input uniformly along span. Put in Start location
         GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
         m_ctrlStartSlabOffset.ShowWindow(SW_SHOW);
         GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->ShowWindow(SW_HIDE);
         m_ctrlEndSlabOffset.ShowWindow(SW_HIDE);
         GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->ShowWindow(SW_HIDE);

         m_ctrlStartSlabOffset.EnableWindow(TRUE);
         GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(TRUE);

         std::vector<Float64> haunches = pParent->GetBridgeDescription()->GetSpan(m_SpanIdx)->GetDirectHaunchDepths(0);
	
         haunchDepth = haunches.front() + (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         strHaunchVal.Format(_T("%s"),FormatDimension(haunchDepth,pDisplayUnits->GetComponentDimUnit(),false));
         m_ctrlStartSlabOffset.SetWindowText(strHaunchVal);
      }
   }
}

void CSpanLayoutPage::UpdateHaunchAndCamberData(CDataExchange* pDX)
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      return;
   }

   pgsTypes::HaunchInputDepthType inputType = pParent->m_BridgeDesc.GetHaunchInputDepthType();

   if (inputType == pgsTypes::hidACamber)
   {
      // not all piers have slab offsets... hide controls if piers don't have it
      CPierData2* pPrevPier = pParent->m_BridgeDesc.GetPier(m_PrevPierIdx);
      CPierData2* pNextPier = pParent->m_BridgeDesc.GetPier(m_NextPierIdx);
      bool bHasStartSlabOffset = pPrevPier->HasSlabOffset();
      bool bHasEndSlabOffset = pNextPier->HasSlabOffset();
  
      pgsTypes::SlabOffsetType slabOffsetType = pParent->m_BridgeDesc.GetSlabOffsetType();
      if (slabOffsetType == pgsTypes::sotBearingLine)
   {
         Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();


         CString strMinValError;
         strMinValError.Format(_T("Slab Offset value must be greater or equal to (%s)"),FormatDimension(minSlabOffset,pDisplayUnits->GetComponentDimUnit()));

         Float64 slabOffset;
         if (bHasStartSlabOffset)
      {
            DDX_UnitValueAndTag(pDX,IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT,slabOffset,pDisplayUnits->GetComponentDimUnit());
            if (::IsLT(slabOffset,minSlabOffset))
            {
               pDX->PrepareCtrl(IDC_START_SLAB_OFFSET);
               AfxMessageBox(strMinValError);
               pDX->Fail();
            }

            pPrevPier->SetSlabOffset(pgsTypes::Ahead, slabOffset);
            }

         if (bHasEndSlabOffset)
         {
            DDX_UnitValueAndTag(pDX,IDC_END_SLAB_OFFSET,IDC_END_SLAB_OFFSET_UNIT,slabOffset,pDisplayUnits->GetComponentDimUnit());
            if (::IsLT(slabOffset,minSlabOffset))
            {
               pDX->PrepareCtrl(IDC_END_SLAB_OFFSET);
               AfxMessageBox(strMinValError);
               pDX->Fail();
            }

            pNextPier->SetSlabOffset(pgsTypes::Back,slabOffset);
         }
      }

      GET_IFACE2(pBroker,ISpecification,pSpec);
      bool bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();
      pgsTypes::AssumedExcessCamberType assumedExcessCamberType = pParent->GetBridgeDescription()->GetAssumedExcessCamberType();

      if (bCanAssumedExcessCamberInputBeEnabled && (bHasStartSlabOffset || bHasEndSlabOffset) && assumedExcessCamberType == pgsTypes::aecSpan)
      {
         Float64 camber;
         DDX_UnitValueAndTag(pDX,IDC_ASSUMED_EXCESS_CAMBER,IDC_ASSUMED_EXCESS_CAMBER_UNIT,camber,pDisplayUnits->GetComponentDimUnit());
         pParent->m_BridgeDesc.GetSpan(m_SpanIdx)->SetAssumedExcessCamber(camber);
         }
      }
      else
      {
      // Direct input of haunch depths
      pgsTypes::HaunchInputLocationType haunchInputLocationType = pParent->m_BridgeDesc.GetHaunchInputLocationType();
      pgsTypes::HaunchLayoutType haunchLayoutType = pParent->m_BridgeDesc.GetHaunchLayoutType();
      pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pParent->m_BridgeDesc.GetHaunchInputDistributionType();

      if (haunchInputLocationType == pgsTypes::hilSame4AllGirders && haunchLayoutType == pgsTypes::hltAlongSpans &&
         (haunchInputDistributionType == pgsTypes::hidUniform || haunchInputDistributionType == pgsTypes::hidAtEnds))
      {
         // Only cases where we take input from dialog
         const CDeckDescription2* pDeck = pParent->m_BridgeDesc.GetDeckDescription();
         Float64 Tdeck;
         if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
         {
            Tdeck = pDeck->GrossDepth + pDeck->PanelDepth;
         }
         else
         {
            Tdeck = pDeck->GrossDepth;
         }

         Float64 minHaunch = pParent->m_BridgeDesc.GetMinimumAllowableHaunchDepth(inputType);;

         CString strMinValError;
         if (inputType == pgsTypes::hidHaunchPlusSlabDirectly)
         {
            strMinValError.Format(_T("Haunch+Slab Depth must be greater or equal to deck depth+fillet (%s)"),FormatDimension(minHaunch,pDisplayUnits->GetComponentDimUnit()));
         }
         else
         {
            strMinValError.Format(_T("Haunch Depth must be greater or equal to fillet (%s)"),FormatDimension(minHaunch,pDisplayUnits->GetComponentDimUnit()));
         }

         // Get current values out of the controls
         CDataExchange dx(this,TRUE);
         Float64 haunchDepth;
         DDX_UnitValueAndTag(&dx,IDC_START_SLAB_OFFSET,IDC_START_SLAB_OFFSET_UNIT,haunchDepth,pDisplayUnits->GetComponentDimUnit());
         if (::IsLT(haunchDepth,minHaunch))
         {
            pDX->PrepareCtrl(IDC_START_SLAB_OFFSET);
            AfxMessageBox(strMinValError);
            pDX->Fail();
         }

         haunchDepth -= (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
         std::vector<Float64> haunchDepths(1,haunchDepth); // only value needed if haunchInputDistributionType == pgsTypes::hidUniform

         if (haunchInputDistributionType == pgsTypes::hidAtEnds)
         {
            DDX_UnitValueAndTag(&dx,IDC_END_SLAB_OFFSET,IDC_END_SLAB_OFFSET_UNIT,haunchDepth,pDisplayUnits->GetComponentDimUnit());
            if (::IsLT(haunchDepth,minHaunch))
            {
               pDX->PrepareCtrl(IDC_END_SLAB_OFFSET);
               AfxMessageBox(strMinValError);
               pDX->Fail();
         }

            haunchDepth -= (inputType == pgsTypes::hidHaunchPlusSlabDirectly ? Tdeck : 0.0);
            haunchDepths.push_back(haunchDepth);
         }

         pParent->m_BridgeDesc.GetSpan(m_SpanIdx)->SetDirectHaunchDepths(haunchDepths);
      }
   }
}

void CSpanLayoutPage::EnableHaunchAndCamberControls(BOOL bEnableControls,BOOL bEnableButton,bool bShowCamber)
{
   GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->ShowWindow(SW_SHOW);
   m_ctrlStartSlabOffset.ShowWindow(SW_SHOW);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->ShowWindow(SW_SHOW);
   m_ctrlEndSlabOffset.ShowWindow(SW_SHOW);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->ShowWindow(SW_SHOW);

   m_ctrlStartSlabOffset.SetWindowText(_T(""));
   m_ctrlEndSlabOffset.SetWindowText(_T(""));
   m_ctrlAssumedExcessCamber.SetWindowText(_T(""));

   int show = bShowCamber ? SW_SHOW : SW_HIDE;
   m_ctrlAssumedExcessCamber.ShowWindow(show);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->ShowWindow(show);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL2)->ShowWindow(show);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->ShowWindow(show); 

   GetDlgItem(IDC_START_SLAB_OFFSET_LABEL)->EnableWindow(bEnableControls);
   m_ctrlStartSlabOffset.EnableWindow(bEnableControls);
   GetDlgItem(IDC_START_SLAB_OFFSET_UNIT)->EnableWindow(bEnableControls);
   GetDlgItem(IDC_END_SLAB_OFFSET_LABEL)->EnableWindow(bEnableControls);
   m_ctrlEndSlabOffset.EnableWindow(bEnableControls);
   GetDlgItem(IDC_END_SLAB_OFFSET_UNIT)->EnableWindow(bEnableControls);

   m_ctrlAssumedExcessCamber.EnableWindow(bEnableControls);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNIT)->EnableWindow(bEnableControls);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL2)->EnableWindow(bEnableControls);
   GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->EnableWindow(bEnableControls);

   GetDlgItem(IDC_EDIT_HAUNCH_BUTTON)->EnableWindow(bEnableButton);
}

void CSpanLayoutPage::OnBnClickedEditHaunchButton()
{
   // Need to validate main dialog data before we go into haunch dialog
   try
   {
      if (TRUE != UpdateData(TRUE))
      {
         return;
      }
   }
   catch (...)
   {
      ATLASSERT(0);
      return;
   }

      CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   CEditHaunchDlg dlg(&(pParent->m_BridgeDesc));
   if (dlg.DoModal() == IDOK)
      {
      // Cannot copy entire bridge description here because this dialog has hooks into pointers withing bridgedescr.
      // Use function to copy haunch and slab offset data
      pParent->m_BridgeDesc.CopyHaunchSettings(dlg.m_BridgeDesc);

      UpdateHaunchAndCamberControls();
   }
}
