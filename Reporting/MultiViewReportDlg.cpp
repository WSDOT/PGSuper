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
//
// MultiViewReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Reporting.h"
#include "MultiViewReportDlg.h"
#include "..\Documentation\PGSuper.hh"

#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>


#include "RMultiGirderSelectDlg.h"
#include <EAF\EAFDocument.h>



// CMultiViewReportDlg dialog

IMPLEMENT_DYNAMIC(CMultiViewReportDlg, CDialog)

CMultiViewReportDlg::CMultiViewReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,
    std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec, UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
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

   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_GirderKey.groupIndex);
   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_GirderKey.girderIndex);

   if ( pDX->m_bSaveAndValidate )
   {

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
   ON_COMMAND(IDC_SELECT_ALL, OnSelectAll)
   ON_COMMAND(IDC_DESELECT_ALL, OnDeselectAll)
END_MESSAGE_MAP()

BOOL CMultiViewReportDlg::OnInitDialog()
{
   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName().c_str());

   GET_IFACE(IBridge, pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   // Fill up the span and girder combo boxes

   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // fill up the span box
   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strLabel;
      if (bIsPGSuper)
      {
         strLabel.Format(_T("Span %s"), LABEL_SPAN(grpIdx));
      }
      else
      {
         strLabel.Format(_T("Group %d"), LABEL_GROUP(grpIdx));
      }

      pSpanBox->AddString(strLabel);
   }
   pSpanBox->SetCurSel((int)m_GirderKey.groupIndex);

   UpdateGirderComboBox(m_GirderKey.groupIndex);

   CDialog::OnInitDialog();

   UpdateChapterList();



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

void CMultiViewReportDlg::OnCbnSelchangeSpan()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = SpanIndexType(pCB->GetCurSel());

   UpdateGirderComboBox(spanIdx);
}

void CMultiViewReportDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
}

void CMultiViewReportDlg::ClearChapterCheckMarks(BOOL bClear)
{
   int cChapters = m_ChList.GetCount();
   for (int ch = 0; ch < cChapters; ch++)
   {
      m_ChList.SetCheck(ch, bClear ? BST_UNCHECKED : BST_CHECKED);
   }
}

void CMultiViewReportDlg::InitChapterListFromSpec()
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




void CMultiViewReportDlg::OnBnClickedSelectMultipleButton()
{

}



void CMultiViewReportDlg::OnSelectAll()
{
   ClearChapterCheckMarks(FALSE);
}

void CMultiViewReportDlg::OnDeselectAll()
{
   ClearChapterCheckMarks();
}
