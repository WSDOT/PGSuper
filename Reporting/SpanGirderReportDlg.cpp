///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <PgsExt\GirderLabel.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CSpanGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CSpanGirderReportDlg, CDialog)

CSpanGirderReportDlg::CSpanGirderReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,RptDialogMode mode,boost::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_Span   = INVALID_INDEX;
   m_Girder = INVALID_INDEX;

   m_Mode = mode;

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
	   DDX_CBIndex(pDX, IDC_SPAN, m_Span);
   }

   if ( m_Mode == SpanGirderAndChapters || m_Mode == GirderAndChapters )
   {
      DDX_CBIndex(pDX, IDC_GIRDER, m_Girder);
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

            m_ChapterList.push_back(std::string(strChapter));
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
	ON_CBN_SELCHANGE(IDC_SPAN, OnSpanChanged)
END_MESSAGE_MAP()


// CSpanGirderReportDlg message handlers
void CSpanGirderReportDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( m_Mode == ChaptersOnly || m_Mode == SpanAndChapters )
      return;

   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = 0;
   if ( spanIdx == ALL_SPANS )
   {
      SpanIndexType nSpans = pBridge->GetSpanCount();
      for ( SpanIndexType i = 0; i < nSpans; i++ )
      {
         cGirders = max(cGirders,pBridge->GetGirderCount(i));
      }
   }
   else
   {
      cGirders = pBridge->GetGirderCount( spanIdx );
   }

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( "Girder %s", LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pGdrBox->SetCurSel(0);
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
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowTextA(m_RptDesc.GetReportName());

   // Fill up the span and girder combo boxes
   GET_IFACE( IBridge, pBridge );

   if ( m_Mode == SpanAndChapters || m_Mode == SpanGirderAndChapters )
   {
      // fill up the span box
      CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
      Uint32 cSpan = pBridge->GetSpanCount();
      for ( Uint32 i = 0; i < cSpan; i++ )
      {
         CString strSpan;
         strSpan.Format("Span %d",LABEL_SPAN(i));
         pSpanBox->AddString(strSpan);
      }
      pSpanBox->SetCurSel(m_Span);

      UpdateGirderComboBox(m_Span);
   }

   if ( m_Mode == SpanAndChapters )
   {
      // hide the girder combo box
      CComboBox* pGirderBox = (CComboBox*)GetDlgItem( IDC_GIRDER );
      pGirderBox->ShowWindow( SW_HIDE );
      pGirderBox->EnableWindow( FALSE );

      CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
      pGroupBox->SetWindowText("Select a Span");
   }
   else if ( m_Mode == GirderAndChapters )
   {
      m_Span = ALL_SPANS;
      UpdateGirderComboBox(m_Span);

      CWnd* pGroupBox = GetDlgItem(IDC_GROUP);
      pGroupBox->SetWindowText("Select a Girder Line");

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
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanGirderReportDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_REPORT );
}

void CSpanGirderReportDlg::OnSpanChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = SpanIndexType(pCB->GetCurSel());

   UpdateGirderComboBox(spanIdx);
}

void CSpanGirderReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
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
      boost::shared_ptr<CSpanReportSpecification> pRptSpec = boost::shared_dynamic_cast<CSpanReportSpecification>(m_pInitRptSpec);
      m_Span = pRptSpec->GetSpan();
   }
   else if ( m_Mode == SpanGirderAndChapters )
   {
      boost::shared_ptr<CSpanGirderReportSpecification> pRptSpec = boost::shared_dynamic_cast<CSpanGirderReportSpecification>(m_pInitRptSpec);
      m_Span   = pRptSpec->GetSpan();
      m_Girder = pRptSpec->GetGirder();
   }
   else if ( m_Mode == GirderAndChapters )
   {
      boost::shared_ptr<CGirderReportSpecification> pRptSpec = boost::shared_dynamic_cast<CGirderReportSpecification>(m_pInitRptSpec);
      m_Girder = pRptSpec->GetGirder();
   }
   else
   {
      ATLASSERT(false); // is there a new mode?
      m_Span = 0;
      m_Girder = 0;
   }

   UpdateData(FALSE);

   InitChapterListFromSpec();
}
