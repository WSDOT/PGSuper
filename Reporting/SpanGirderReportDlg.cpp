///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderLabel.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CSpanGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CSpanGirderReportDlg, CDialog)

CSpanGirderReportDlg::CSpanGirderReportDlg(IBroker* pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec), m_Mode(mode)
{
   m_pBroker = pBroker;
}

CSpanGirderReportDlg::~CSpanGirderReportDlg()
{
}

void CSpanGirderReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

   if ( m_Mode == Mode::GroupAndChapters || m_Mode == Mode::GroupGirderAndChapters || m_Mode == Mode::GroupGirderSegmentAndChapters)
   {
	   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_SegmentKey.groupIndex);
   }

   if ( m_Mode == Mode::GroupGirderAndChapters || m_Mode == Mode::GroupGirderSegmentAndChapters || m_Mode == Mode::GirderAndChapters )
   {
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_SegmentKey.girderIndex);
   }

   if ( m_Mode == Mode::GroupGirderSegmentAndChapters)
   {
      DDX_CBIndex(pDX, IDC_SEGMENT, (int&)m_SegmentKey.segmentIndex);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      m_ChapterList.clear();

      int cSelChapters = 0;
      int cChapters = m_ChList.GetCount();
      for ( int ch = 0; ch < cChapters; ch++ )
      {
         if ( m_ChList.GetCheck( ch ) == 1 )
         {
            cSelChapters++;

            CString strChapter;
            m_ChList.GetText(ch,strChapter);

            // m_ChapterList needs to hold the Key name for the chapter information
            // but the chapter list box has the display name
            // Look up the chapter display name to get the chapter key
            std::vector<WBFL::Reporting::ChapterInfo> chInfos = m_RptDesc.GetChapterInfo();
            for (const auto& chInfo : chInfos)
            {
               if (chInfo.Name == std::_tstring(strChapter))
               {
                  m_ChapterList.push_back(chInfo.Key);
                  break;
               }
            }
         }
      }

      if ( cSelChapters == 0 )
      {
         pDX->PrepareCtrl(IDC_LIST);
         AfxMessageBox(IDS_E_NOCHAPTERS);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CSpanGirderReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
   ON_COMMAND(IDC_SELECT_ALL,OnSelectAll)
   ON_COMMAND(IDC_DESELECT_ALL,OnDeselectAll)
	ON_CBN_SELCHANGE(IDC_SPAN, OnGroupChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER, OnGirderChanged)
END_MESSAGE_MAP()


// CSpanGirderReportDlg message handlers
void CSpanGirderReportDlg::UpdateGirderComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_Mode == Mode::ChaptersOnly || m_Mode == Mode::GroupAndChapters )
   {
      return;
   }

   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = 0;
   if ( m_SegmentKey.groupIndex == ALL_GROUPS )
   {
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType i = 0; i < nGroups; i++ )
      {
         cGirders = Max(cGirders,pBridge->GetGirderCount(i));
      }
   }
   else
   {
      cGirders = pBridge->GetGirderCount( m_SegmentKey.groupIndex );
   }

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
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

   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);
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

void CSpanGirderReportDlg::UpdateChapterList()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Clear out the list box
   m_ChList.ResetContent();

   // Get the chapters in the report
   std::vector<WBFL::Reporting::ChapterInfo> chInfos = m_RptDesc.GetChapterInfo();

   // Populate the list box with the names of the chapters
   std::vector<WBFL::Reporting::ChapterInfo>::iterator iter;
   for ( iter = chInfos.begin(); iter != chInfos.end(); iter++ )
   {
      WBFL::Reporting::ChapterInfo chInfo = *iter;

      int idx = m_ChList.AddString( chInfo.Name.c_str() );
      if ( idx != LB_ERR ) // no error
      {
         m_ChList.SetCheck( idx, chInfo.Select ? 1 : 0 ); // turn the check on
         m_ChList.SetItemData( idx, chInfo.MaxLevel );
      }
   }

   // don't select any items in the chapter list
   m_ChList.SetCurSel(-1);
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
         CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SEGMENT);
         pSegmentBox->ShowWindow(SW_HIDE);
         pSegmentBox->EnableWindow(FALSE);
      }
      else
      {
         if (bIsPGSuper)
         {
            m_SegmentKey.segmentIndex = 0;
            CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SEGMENT);
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

      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SEGMENT);
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


      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SEGMENT);
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

      CComboBox* pSegmentBox = (CComboBox*)GetDlgItem(IDC_SEGMENT);
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

      // capture the distance between the bototm of the lable and the top of the list so we can maintain this distnace
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

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
   {
      InitFromRptSpec();
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanGirderReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
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

void CSpanGirderReportDlg::ClearChapterCheckMarks(BOOL bClear)
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,bClear ? BST_UNCHECKED : BST_CHECKED);
   }
}

void CSpanGirderReportDlg::InitChapterListFromSpec()
{
   ClearChapterCheckMarks();
   std::vector<WBFL::Reporting::ChapterInfo> chInfo = m_pInitRptSpec->GetChapterInfo();
   std::vector<WBFL::Reporting::ChapterInfo>::iterator iter;
   for ( iter = chInfo.begin(); iter != chInfo.end(); iter++ )
   {
      WBFL::Reporting::ChapterInfo& ch = *iter;
      int cChapters = m_ChList.GetCount();
      for ( int idx = 0; idx < cChapters; idx++ )
      {
         CString strChapter;
         m_ChList.GetText(idx,strChapter);
         if ( strChapter == ch.Name.c_str() )
         {
            m_ChList.SetCheck(idx,1);
            break;
         }
      }
   }
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

void CSpanGirderReportDlg::OnSelectAll()
{
   ClearChapterCheckMarks(FALSE);
}

void CSpanGirderReportDlg::OnDeselectAll()
{
   ClearChapterCheckMarks();
}
