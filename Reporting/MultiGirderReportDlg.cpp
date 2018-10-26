///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// MultiGirderReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\MultiGirderReportDlg.h>

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

// CMultiGirderReportDlg dialog

IMPLEMENT_DYNAMIC(CMultiGirderReportDlg, CDialog)

CMultiGirderReportDlg::CMultiGirderReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_pBroker = pBroker;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   m_pGrid = new CMultiGirderSelectGrid();
}

CMultiGirderReportDlg::~CMultiGirderReportDlg()
{
   delete m_pGrid;
}

void CMultiGirderReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

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

      // Girders
      m_GirderKeys = m_pGrid->GetData();
      if ( m_GirderKeys.empty() )
      {
         pDX->PrepareCtrl(IDC_SELECT_GRID);
         AfxMessageBox(IDS_E_NOGIRDERS);
         pDX->Fail();
      }

   }


}


BEGIN_MESSAGE_MAP(CMultiGirderReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
   ON_BN_CLICKED(IDC_SELECT_ALL, &CMultiGirderReportDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CMultiGirderReportDlg::OnBnClickedClearAll)
END_MESSAGE_MAP()

void CMultiGirderReportDlg::UpdateChapterList()
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

BOOL CMultiGirderReportDlg::OnInitDialog()
{
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

 	m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   CDialog::OnInitDialog();

   // need list of groups/girders
   GET_IFACE( IBridge, pBridge );
   GroupGirderOnCollection coll;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      std::vector<bool> gdrson;
      gdrson.assign(nGirders, false); // set all to false

      coll.push_back(gdrson);
   }

   // set selected girders
   std::vector<CGirderKey>::iterator iter(m_GirderKeys.begin());
   std::vector<CGirderKey>::iterator end(m_GirderKeys.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& girderKey(*iter);

      if (girderKey.groupIndex < nGroups)
      {
         std::vector<bool>& rgdrson = coll[girderKey.groupIndex];
         if (girderKey.girderIndex < (GirderIndexType)rgdrson.size())
         {
            rgdrson[girderKey.girderIndex] = true;
         }
         else
         {
            ATLASSERT(0); // might be a problem?
         }
      }
      else
      {
         ATLASSERT(0); // might be a problem?
      }
   }

   m_pGrid->CustomInit(coll,pgsGirderLabel::GetGirderLabel);


   UpdateChapterList();

   if ( m_pInitRptSpec )
      InitFromRptSpec();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CMultiGirderReportDlg::OnHelp() 
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_MULTIGIRDER_REPORT );
}

void CMultiGirderReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CMultiGirderReportDlg::InitChapterListFromSpec()
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

void CMultiGirderReportDlg::InitFromRptSpec()
{
   boost::shared_ptr<CMultiGirderReportSpecification> pRptSpec = boost::shared_dynamic_cast<CMultiGirderReportSpecification>(m_pInitRptSpec);
   ATLASSERT(pRptSpec); // is there a new mode?
   m_GirderKeys = pRptSpec->GetGirderKeys();

   UpdateData(FALSE);

   InitChapterListFromSpec();
}

void CMultiGirderReportDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CMultiGirderReportDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}
