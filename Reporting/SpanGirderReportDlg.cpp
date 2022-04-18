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

CSpanGirderReportDlg::CSpanGirderReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,RptDialogMode mode,std::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec), m_Mode(mode)
{
   m_Group  = 0;
   m_Girder = 0;

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

   if ( m_Mode == SpanAndChapters || m_Mode == SpanGirderAndChapters)
   {
	   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_Group);
   }

   if ( m_Mode == SpanGirderAndChapters || m_Mode == GirderAndChapters )
   {
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);
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

            m_ChapterList.push_back(std::_tstring(strChapter));
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
END_MESSAGE_MAP()


// CSpanGirderReportDlg message handlers
void CSpanGirderReportDlg::UpdateGirderComboBox(GroupIndexType grpIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_Mode == ChaptersOnly || m_Mode == SpanAndChapters )
   {
      return;
   }

   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = 0;
   if ( grpIdx == ALL_GROUPS )
   {
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType i = 0; i < nGroups; i++ )
      {
         cGirders = Max(cGirders,pBridge->GetGirderCount(i));
      }
   }
   else
   {
      cGirders = pBridge->GetGirderCount( grpIdx );
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
}

void CSpanGirderReportDlg::UpdateChapterList()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Clear out the list box
   m_ChList.ResetContent();

   // Get the chapters in the report
   std::vector<CChapterInfo> chInfos = m_RptDesc.GetChapterInfo();

   // Populate the list box with the names of the chapters
   std::vector<CChapterInfo>::iterator iter;
   for ( iter = chInfos.begin(); iter != chInfos.end(); iter++ )
   {
      CChapterInfo chInfo = *iter;

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
   if (m_Group == ALL_GROUPS)
   {
      m_Group = 0;
   }

   if ( m_Girder == ALL_GIRDERS )
   {
      m_Girder = 0;
   }

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // Fill up the span and girder combo boxes
   if ( m_Mode == SpanAndChapters || m_Mode == SpanGirderAndChapters )
   {
      // fill up the group list box
      CComboBox* pGroupBox = (CComboBox*)GetDlgItem( IDC_SPAN );
      GET_IFACE( IBridge, pBridge );
      GroupIndexType cGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType i = 0; i < cGroups; i++ )
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
      pGroupBox->SetCurSel((int)m_Group);

      UpdateGirderComboBox(m_Group);
   }

   if ( m_Mode == SpanAndChapters )
   {
      // hide the girder combo box
      CComboBox* pGirderBox = (CComboBox*)GetDlgItem( IDC_GIRDER );
      pGirderBox->ShowWindow( SW_HIDE );
      pGirderBox->EnableWindow( FALSE );

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
   else if ( m_Mode == GirderAndChapters )
   {
      m_Group = ALL_GROUPS;
      UpdateGirderComboBox(m_Group);

      CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
      pGroupBox->SetWindowText(_T("Select a Girder Line"));

      CComboBox* pSpanBox   = (CComboBox*)GetDlgItem( IDC_SPAN );
      CRect rSpan;
      pSpanBox->GetWindowRect(&rSpan);
      pSpanBox->ShowWindow( SW_HIDE );
      pSpanBox->EnableWindow( FALSE );

      CComboBox* pGirderBox = (CComboBox*)GetDlgItem( IDC_GIRDER );
      CRect rGirder;
      pGirderBox->GetWindowRect(&rGirder);
      rGirder.left = rSpan.left;

      ScreenToClient(&rGirder);

      pGirderBox->MoveWindow(&rGirder);
   }
   else if ( m_Mode == ChaptersOnly )
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
   GroupIndexType grpIdx = GroupIndexType(pCB->GetCurSel());

   UpdateGirderComboBox(grpIdx);
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
   std::vector<CChapterInfo> chInfo = m_pInitRptSpec->GetChapterInfo();
   std::vector<CChapterInfo>::iterator iter;
   for ( iter = chInfo.begin(); iter != chInfo.end(); iter++ )
   {
      CChapterInfo& ch = *iter;
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
   if ( m_Mode == SpanAndChapters )
   {
      std::shared_ptr<CSpanReportSpecification> pRptSpec = std::dynamic_pointer_cast<CSpanReportSpecification>(m_pInitRptSpec);
      m_Group = pRptSpec->GetSpan();
   }
   else if ( m_Mode == SpanGirderAndChapters )
   {
      std::shared_ptr<CGirderReportSpecification> pRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification>(m_pInitRptSpec);
      const CGirderKey& girderKey(pRptSpec->GetGirderKey());
      m_Group = girderKey.groupIndex;
      m_Girder = girderKey.girderIndex;
   }
   else if ( m_Mode == GirderAndChapters )
   {
      std::shared_ptr<CGirderLineReportSpecification> pRptSpec = std::dynamic_pointer_cast<CGirderLineReportSpecification>(m_pInitRptSpec);
      m_Girder = int(pRptSpec->GetGirderIndex());
   }
   else if ( m_Mode == ChaptersOnly )
   {
      m_Group  = INVALID_INDEX;
      m_Girder = INVALID_INDEX;
   }
   else
   {
      ATLASSERT(false); // is there a new mode?
      m_Group  = 0;
      m_Girder = 0;
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
