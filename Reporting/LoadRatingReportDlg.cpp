///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// LoadRatingReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\LoadRatingReportDlg.h>
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include "..\Documentation\PGSuper.hh"

#include <initguid.h>
#include <IFace\Tools.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderLabel.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLoadRatingReportDlg dialog

IMPLEMENT_DYNAMIC(CLoadRatingReportDlg, CDialog)

CLoadRatingReportDlg::CLoadRatingReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CDialog(nIDTemplate, pParent), m_RptDesc(rptDesc), m_pInitRptSpec(pRptSpec)
{
   m_Girder = 0;
   m_bReportAtAllPoi = false;

   m_pBroker = pBroker;
}

CLoadRatingReportDlg::~CLoadRatingReportDlg()
{
}

void CLoadRatingReportDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_ChList);

   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);

   int idx = (m_bReportAtAllPoi ? 1 : 0);
   DDX_CBIndex(pDX, IDC_POI, idx);
   m_bReportAtAllPoi = (idx == 0 ? false : true);

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


BEGIN_MESSAGE_MAP(CLoadRatingReportDlg, CDialog)
	ON_COMMAND(IDHELP, OnHelp)
END_MESSAGE_MAP()


// CLoadRatingReportDlg message handlers
void CLoadRatingReportDlg::UpdateGirderComboBox()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirders = 0;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType i = 0; i < nGroups; i++ )
   {
      cGirders = Max(cGirders,pBridge->GetGirderCount(i));
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

void CLoadRatingReportDlg::UpdateChapterList()
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

BOOL CLoadRatingReportDlg::OnInitDialog()
{
   if ( m_Girder == ALL_GIRDERS )
   {
      m_Girder = 0;
   }

   CWnd* pwndTitle = GetDlgItem(IDC_REPORT_TITLE);
   pwndTitle->SetWindowText(m_RptDesc.GetReportName());

   UpdateGirderComboBox();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_POI);
   pCB->AddString(_T("Report at controlling and 10th points"));
   pCB->AddString(_T("Report at all points"));

   CDialog::OnInitDialog();

   UpdateChapterList();

   if ( m_pInitRptSpec )
   {
      InitFromRptSpec();
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoadRatingReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
}

void CLoadRatingReportDlg::ClearChapterCheckMarks()
{
   int cChapters = m_ChList.GetCount();
   for ( int ch = 0; ch < cChapters; ch++ )
   {
      m_ChList.SetCheck(ch,0);
   }
}

void CLoadRatingReportDlg::InitChapterListFromSpec()
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

void CLoadRatingReportDlg::InitFromRptSpec()
{
   std::shared_ptr<CLoadRatingReportSpecification> pRptSpec = std::dynamic_pointer_cast<CLoadRatingReportSpecification>(m_pInitRptSpec);
   m_Girder = int(pRptSpec->GetGirderIndex());
   m_bReportAtAllPoi = pRptSpec->ReportAtAllPointsOfInterest();

   UpdateData(FALSE);

   InitChapterListFromSpec();
}
