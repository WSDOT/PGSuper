///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// SpanGirderReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\SpanGirderReportDlg.h>

#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PsgLib\GirderLabel.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"


// CSpanGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CSpanGirderReportDlg, CSpanItemReportDlg)

CSpanGirderReportDlg::CSpanGirderReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CSpanItemReportDlg(pBroker, rptDesc, mode, pRptSpec, nIDTemplate, pParent)
{

}

CSpanGirderReportDlg::~CSpanGirderReportDlg()
{
}

void CSpanGirderReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_Mode == Mode::GroupAndChapters || m_Mode == Mode::GroupGirderAndChapters)
   {
       DDX_CBIndex(pDX, IDC_SPAN, (int&)m_SegmentKey.groupIndex);
   }

   if (m_Mode == Mode::GroupGirderAndChapters || m_Mode == Mode::GirderAndChapters)
   {
       DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_SegmentKey.girderIndex);
   }

   if ( m_Mode == Mode::GroupGirderSegmentAndChapters)
   {
      DDX_CBIndex(pDX, IDC_SPAN_ITEM, (int&)m_SegmentKey.segmentIndex);
   }

   CSpanItemReportDlg::DoDataExchange(pDX);

}

BEGIN_MESSAGE_MAP(CSpanGirderReportDlg, CSpanItemReportDlg)
    ON_CBN_SELCHANGE(IDC_SPAN, OnGroupChanged)
    ON_CBN_SELCHANGE(IDC_GIRDER, OnGirderChanged)
END_MESSAGE_MAP()


// CSpanGirderReportDlg message handlers
void CSpanGirderReportDlg::UpdateGirderComboBox()
{

   CSpanItemReportDlg::UpdateGirderComboBox();

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GET_IFACE(IBridge, pBridge);

   GirderIndexType cGirders = 0;
   if (m_SegmentKey.groupIndex == ALL_GROUPS)
   {
       GroupIndexType nGroups = pBridge->GetGirderGroupCount();
       for (GroupIndexType i = 0; i < nGroups; i++)
       {
           cGirders = Max(cGirders, pBridge->GetGirderCount(i));
       }
   }
   else
   {
       cGirders = pBridge->GetGirderCount(m_SegmentKey.groupIndex);
   }

   for (GirderIndexType j = 0; j < cGirders; j++)
   {
       CString strGdr;
       strGdr.Format(_T("Girder %s"), LABEL_GIRDER(j));
       pGdrBox->AddString(strGdr);
   }

   if (pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
   {
       pGdrBox->SetCurSel(0);
   }

   UpdateSegmentComboBox();
}

void CSpanGirderReportDlg::UpdateSegmentComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_Mode == Mode::ChaptersOnly || m_Mode == Mode::GroupAndChapters || m_Mode == Mode::GirderAndChapters)
   {
      return;
   }

   GET_IFACE(IBridge, pBridge);

   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_SPAN);
   GroupIndexType grpIdx = (GroupIndexType)pcbGroup->GetCurSel();

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   GirderIndexType gdrIdx = (GirderIndexType)pcbGirder->GetCurSel();

   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
   Uint16 curSel = pcbSegment->GetCurSel();
   pcbSegment->ResetContent();

   SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, gdrIdx);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CString strLabel;
      strLabel.Format(_T("Segment %d"), LABEL_SEGMENT(segIdx));
      pcbSegment->AddString(strLabel);
   }

   if (pcbSegment->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
   {
      pcbSegment->SetCurSel(0);
   }
}

BOOL CSpanGirderReportDlg::OnInitDialog()
{
   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName().c_str());

   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // It is possible that a span was removed from the bridge, and if so, the report description for this report 
   // will have a higher group that that exists. Nip in the bud here so we don't crash
   GET_IFACE(IBridge, pBridge);
   GroupIndexType numGroups = pBridge->GetGirderGroupCount();
   if (m_SegmentKey.groupIndex != ALL_GROUPS && m_SegmentKey.groupIndex > numGroups - 1)
   {
      m_SegmentKey.groupIndex = numGroups - 1;
   }

   // Fill up the span and girder combo boxes
   if ( m_Mode == Mode::GroupAndChapters || m_Mode == Mode::GroupGirderAndChapters || m_Mode == Mode::GroupGirderSegmentAndChapters)
   {
      // make sure we are starting off with a valid segment key
      if (m_Mode == Mode::GroupAndChapters)
      {
         if (m_SegmentKey.groupIndex == ALL_GROUPS) m_SegmentKey.groupIndex = 0;
      }
      else if (m_Mode == Mode::GroupGirderAndChapters)
      {
         if (m_SegmentKey.groupIndex == ALL_GROUPS) m_SegmentKey.groupIndex = 0;
         if (m_SegmentKey.girderIndex == ALL_GIRDERS) m_SegmentKey.girderIndex = 0;
      }
      else if(m_Mode == Mode::GroupGirderSegmentAndChapters)
      {
         if (m_SegmentKey.groupIndex == ALL_GROUPS) m_SegmentKey.groupIndex = 0;
         if (m_SegmentKey.girderIndex == ALL_GIRDERS) m_SegmentKey.girderIndex = 0;
         if (m_SegmentKey.segmentIndex == ALL_SEGMENTS) m_SegmentKey.segmentIndex = 0;
      }

      // fill up the group list box
      CComboBox* pGroupBox = (CComboBox*)GetDlgItem( IDC_SPAN );
      for ( GroupIndexType i = 0; i < numGroups; i++ )
      {
         CString strGroup;
         if (bIsPGSuper)
         {
            strGroup.Format(_T("Span %s"), LABEL_SPAN(i));
         }
         else
         {
            strGroup.Format(_T("Group %d"), LABEL_GROUP(i));
         }

         pGroupBox->AddString(strGroup);
      }
      pGroupBox->SetCurSel((int)m_SegmentKey.groupIndex);

      if (m_Mode == Mode::GroupAndChapters || m_Mode == Mode::GroupGirderAndChapters)
      {
         CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
         pSegmentBox->ShowWindow(SW_HIDE);
         pSegmentBox->EnableWindow(FALSE);
      }
      else
      {
         if (bIsPGSuper)
         {
            m_SegmentKey.segmentIndex = 0;
            CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
            pSegmentBox->ShowWindow(SW_HIDE);
            pSegmentBox->EnableWindow(FALSE);
         }
         else
         {
            ASSERT(m_Mode == Mode::GroupGirderSegmentAndChapters);
            CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
            pGroupBox->SetWindowText(_T("Select a Segment"));
         }
      }

      UpdateGirderComboBox();
   }
   else if ( m_Mode == Mode::GroupAndChapters )
   {
      if (m_SegmentKey.groupIndex == ALL_GROUPS) m_SegmentKey.groupIndex = 0;

      // hide the girder combo box
      CComboBox* pGirderBox = (CComboBox*)GetDlgItem( IDC_GIRDER );
      pGirderBox->ShowWindow( SW_HIDE );
      pGirderBox->EnableWindow( FALSE );

      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
      pSegmentBox->ShowWindow(SW_HIDE);
      pSegmentBox->EnableWindow(FALSE);

      CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
      CString str;
      if (bIsPGSuper)
      {
         str = _T("Select a Span");
      }
      else
      {
         str = _T("Select a Group");
      }

      pGroupBox->SetWindowText(str);
   }
   else if ( m_Mode == Mode::GirderAndChapters )
   {
      if (m_SegmentKey.girderIndex == ALL_GIRDERS) m_SegmentKey.girderIndex = 0;

      m_SegmentKey.groupIndex = ALL_GROUPS;
      UpdateGirderComboBox();

      CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
      pGroupBox->SetWindowText(_T("Select a Girder Line"));

      CComboBox* pSpanBox   = (CComboBox*)GetDlgItem( IDC_SPAN );
      CRect rSpan;
      pSpanBox->GetWindowRect(&rSpan);
      pSpanBox->ShowWindow( SW_HIDE );
      pSpanBox->EnableWindow( FALSE );


      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
      if (pSegmentBox)
      {
         pSegmentBox->ShowWindow(SW_HIDE);
         pSegmentBox->EnableWindow(FALSE);
      }

      CComboBox* pGirderBox = (CComboBox*)GetDlgItem( IDC_GIRDER );
      CRect rGirder;
      pGirderBox->GetWindowRect(&rGirder);
      rGirder.left = rSpan.left;

      ScreenToClient(&rGirder);

      pGirderBox->MoveWindow(&rGirder);
   }
   else if ( m_Mode == Mode::ChaptersOnly )
   {
      // hide the span and girder combo boxes, the group box,
      // and resize the chapter list

      CWnd* pSpanBox   = GetDlgItem(IDC_SPAN);
      CWnd* pGirderBox = GetDlgItem(IDC_GIRDER);
      CWnd* pGroupBox  = GetDlgItem(IDC_GROUP);
      CWnd* pLabel     = GetDlgItem(IDC_LABEL);
      CWnd* pList      = GetDlgItem(IDC_LIST);

      pSpanBox->ShowWindow( SW_HIDE );
      pSpanBox->EnableWindow( FALSE );

      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SPAN_ITEM);
      if (pSegmentBox)
      {
         pSegmentBox->ShowWindow(SW_HIDE);
         pSegmentBox->EnableWindow(FALSE);
      }

      pGirderBox->ShowWindow( SW_HIDE );
      pGirderBox->EnableWindow( FALSE );

      pGroupBox->ShowWindow( SW_HIDE );
      pGroupBox->EnableWindow( FALSE );

      CRect rGroup;
      pGroupBox->GetWindowRect(&rGroup);

      CRect rLabel;
      pLabel->GetWindowRect(&rLabel);

      CRect rList;
      pList->GetWindowRect(&rList);

      // capture the distance between the bottom of the label and the top of the list so we can maintain this distance
      int dy = rList.top - rLabel.bottom;

      // move label to where the top of the group box is
      int h = rLabel.Height();
      rLabel.top = rGroup.top;
      rLabel.bottom = rLabel.top + h;
      rList.top = rLabel.bottom + dy;

      ScreenToClient(&rLabel);
      ScreenToClient(&rList);

      pLabel->MoveWindow(&rLabel);
      pList->MoveWindow(&rList);
   }

   if (m_pInitRptSpec)
   {
       InitFromRptSpec();
   }

   CSpanItemReportDlg::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanGirderReportDlg::OnGroupChanged()
{
    CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
    m_SegmentKey.groupIndex = GroupIndexType(pCB->GetCurSel());

    UpdateGirderComboBox();
}

void CSpanGirderReportDlg::OnGirderChanged()
{
    CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER);
    m_SegmentKey.girderIndex = GirderIndexType(pCB->GetCurSel());

    UpdateSegmentComboBox();
}

void CSpanGirderReportDlg::InitFromRptSpec()
{
   if ( m_Mode == Mode::GroupAndChapters )
   {
      std::shared_ptr<CSpanReportSpecification> pRptSpec = std::dynamic_pointer_cast<CSpanReportSpecification>(m_pInitRptSpec);
      m_SegmentKey.groupIndex = pRptSpec->GetSpan();
   }
   else if ( m_Mode == Mode::GroupGirderAndChapters )
   {
      std::shared_ptr<CGirderReportSpecification> pRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification>(m_pInitRptSpec);
      const CGirderKey& girderKey(pRptSpec->GetGirderKey());
      m_SegmentKey = CSegmentKey(girderKey, INVALID_INDEX);
   }
   else if (m_Mode == Mode::GroupGirderSegmentAndChapters)
   {
      std::shared_ptr<CSegmentReportSpecification> pRptSpec = std::dynamic_pointer_cast<CSegmentReportSpecification>(m_pInitRptSpec);
      m_SegmentKey = pRptSpec->GetSegmentKey();
   }
   else if ( m_Mode == Mode::GirderAndChapters )
   {
      std::shared_ptr<CGirderLineReportSpecification> pRptSpec = std::dynamic_pointer_cast<CGirderLineReportSpecification>(m_pInitRptSpec);
      m_SegmentKey.girderIndex = int(pRptSpec->GetGirderIndex());
   }
   else if ( m_Mode == Mode::ChaptersOnly )
   {
      m_SegmentKey = CSegmentKey(INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);
   }
   else
   {
      ATLASSERT(false); // is there a new mode?
      m_SegmentKey = CSegmentKey(0, 0, 0);
   }

   UpdateData(FALSE);

   InitChapterListFromSpec();
}

