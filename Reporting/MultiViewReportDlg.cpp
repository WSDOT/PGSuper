///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
//
// MultiViewReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "MultiViewReportDlg.h"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <PgsExt\GirderLabel.h>

#include "HtmlHelp\HelpTopics.hh"
#include "RMultiGirderSelectDlg.h"

// CMultiViewReportDlg dialog

IMPLEMENT_DYNAMIC(CMultiViewReportDlg, CDialog)

CMultiViewReportDlg::CMultiViewReportDlg(IBroker* pBroker,const CReportDescription& rptDesc, boost::shared_ptr<CReportSpecification>& pRptSpec,
                                         SpanIndexType span, GirderIndexType girder,
                                         UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_Span   = span;
   m_Girder = girder;
   m_pBroker = pBroker;
}

CMultiViewReportDlg::~CMultiViewReportDlg()
{
}

void CMultiViewReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST, m_ChList);

   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_Span);
   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);

   if ( pDX->m_bSaveAndValidate )
   {
      // Girder list
      // Make list of one if single girder is checked
      BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
      if (enab_sgl)
      {
         m_GirderList.clear();
         SpanGirderHashType hash =  HashSpanGirder(m_Span, m_Girder);
         m_GirderList.push_back(hash);
      }
      else if (m_GirderList.empty())
      {
         AfxMessageBox(_T("You must select at least one girder"));
         pDX->Fail();
      }

      // Chapters
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

BEGIN_MESSAGE_MAP(CMultiViewReportDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_SPAN, &CMultiViewReportDlg::OnCbnSelchangeSpan)
   ON_BN_CLICKED(IDHELP, &CMultiViewReportDlg::OnHelp)
   ON_BN_CLICKED(IDC_RADIO1, &CMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CMultiViewReportDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SELECT_MULTIPLE_BUTTON, &CMultiViewReportDlg::OnBnClickedSelectMultipleButton)
END_MESSAGE_MAP()

BOOL CMultiViewReportDlg::OnInitDialog()
{
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   CButton* pRad = (CButton*)GetDlgItem(IDC_RADIO1);
   pRad->SetCheck(BST_CHECKED);
   OnBnClickedRadio();

   // Fill up the span and girder combo boxes
   GET_IFACE( IBridge, pBridge );

   // fill up the span box
   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   SpanIndexType cSpan = pBridge->GetSpanCount();
   for ( SpanIndexType i = 0; i < cSpan; i++ )
   {
      CString strSpan;
      strSpan.Format(_T("Span %d"),LABEL_SPAN(i));
      pSpanBox->AddString(strSpan);
   }
   pSpanBox->SetCurSel((int)m_Span);

   UpdateGirderComboBox(m_Span);

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void CMultiViewReportDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = pBridge->GetGirderCount( spanIdx );

   for ( GirderIndexType j = 0; j < cGirders; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pGdrBox->SetCurSel(0);
}

void CMultiViewReportDlg::UpdateChapterList()
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

void CMultiViewReportDlg::OnCbnSelchangeSpan()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = SpanIndexType(pCB->GetCurSel());

   UpdateGirderComboBox(spanIdx);
}

void CMultiViewReportDlg::OnHelp()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_REPORT );
}


void CMultiViewReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CMultiViewReportDlg::InitChapterListFromSpec()
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

void CMultiViewReportDlg::InitFromRptSpec()
{
   boost::shared_ptr<CMultiViewSpanGirderReportSpecification> pRptSpec = boost::dynamic_pointer_cast<CMultiViewSpanGirderReportSpecification>(m_pInitRptSpec);
   if (pRptSpec)
   {
      m_GirderList.clear();

      std::vector<SpanGirderHashType> girders = pRptSpec->GetGirderList();
      bool first(true);
      for (std::vector<SpanGirderHashType>::iterator it=girders.begin(); it!=girders.end(); it++)
      {
         if (first) // take first girder from list as default single girder
         {
            UnhashSpanGirder(*it, &m_Span, &m_Girder);
         }

         m_GirderList.push_back(*it);
      }
   }
   else
   {
      ATLASSERT(0);
      return;
   }

   UpdateData(FALSE);

   InitChapterListFromSpec();
}

void CMultiViewReportDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_GirderList.size() == 0 )
   {
      OnBnClickedSelectMultipleButton();
   }
}

void CMultiViewReportDlg::OnBnClickedSelectMultipleButton()
{

   CRMultiGirderSelectDlg dlg;
   dlg.m_SelGdrs = m_GirderList;

   if (dlg.DoModal()==IDOK)
   {
      m_GirderList = dlg.m_SelGdrs;

      // update button text
      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderList.size());
      GetDlgItem(IDC_SELECT_MULTIPLE_BUTTON)->SetWindowText(msg);
   }
}

std::vector<SpanGirderHashType> CMultiViewReportDlg::GetGirderList() const
{
   return m_GirderList;
}
