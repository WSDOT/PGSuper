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
#include <Reporting\SpanItemReportDlg.h>

#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PsgLib\GirderLabel.h>
#include <EAF\EAFDocument.h>
#include "..\Documentation\PGSuper.hh"


// CSpanGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CSpanItemReportDlg, CDialog)

CSpanItemReportDlg::CSpanItemReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec), m_Mode(mode)
{
   m_pBroker = pBroker;
}

CSpanItemReportDlg::~CSpanItemReportDlg()
{
}

void CSpanItemReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST, m_ChList);



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


BEGIN_MESSAGE_MAP(CSpanItemReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
   ON_COMMAND(IDC_SELECT_ALL,OnSelectAll)
   ON_COMMAND(IDC_DESELECT_ALL,OnDeselectAll)
END_MESSAGE_MAP()


// CSpanGirderReportDlg message handlers
void CSpanItemReportDlg::UpdateGirderComboBox()
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
   if ( m_GirderKey.groupIndex == ALL_GROUPS )
   {
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for ( GroupIndexType i = 0; i < nGroups; i++ )
      {
         cGirders = Max(cGirders,pBridge->GetGirderCount(i));
      }
   }
   else
   {
      cGirders = pBridge->GetGirderCount( m_GirderKey.groupIndex );
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



void CSpanItemReportDlg::UpdateChapterList()
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

BOOL CSpanItemReportDlg::OnInitDialog()
{

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
   {
      InitFromRptSpec();
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanItemReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
}

void CSpanItemReportDlg::OnGroupChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   m_GirderKey.groupIndex = GroupIndexType(pCB->GetCurSel());

   UpdateGirderComboBox();
}

void CSpanItemReportDlg::OnGirderChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER);
   m_GirderKey.girderIndex = GirderIndexType(pCB->GetCurSel());

}

void CSpanItemReportDlg::ClearChapterCheckMarks(BOOL bClear)
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,bClear ? BST_UNCHECKED : BST_CHECKED);
   }
}

void CSpanItemReportDlg::InitChapterListFromSpec()
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

void CSpanItemReportDlg::InitFromRptSpec()
{

}

void CSpanItemReportDlg::OnSelectAll()
{
   ClearChapterCheckMarks(FALSE);
}

void CSpanItemReportDlg::OnDeselectAll()
{
   ClearChapterCheckMarks();
}
