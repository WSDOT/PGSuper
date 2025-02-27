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

// BridgeAnalysisReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\BridgeAnalysisReportDlg.h>
#include <Reporting\BridgeAnalysisReportSpecification.h>
#include <MFCTools\CustomDDX.h>
#include "..\Documentation\PGSuper.hh"

#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CBridgeAnalysisReportDlg dialog

IMPLEMENT_DYNAMIC(CBridgeAnalysisReportDlg, CSpanGirderReportDlg)

CBridgeAnalysisReportDlg::CBridgeAnalysisReportDlg(IBroker* pBroker,const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CSpanGirderReportDlg(pBroker,rptDesc, CSpanGirderReportDlg::Mode::GirderAndChapters,pRptSpec,nIDTemplate, pParent)
{
   m_bDesign = true;
   m_bRating = true;
}

CBridgeAnalysisReportDlg::~CBridgeAnalysisReportDlg()
{
}

void CBridgeAnalysisReportDlg::DoDataExchange(CDataExchange* pDX)
{
   CSpanGirderReportDlg::DoDataExchange(pDX);

   DDX_Check_Bool(pDX,IDC_DESIGN,m_bDesign);
   DDX_Check_Bool(pDX,IDC_RATING,m_bRating);
}


BEGIN_MESSAGE_MAP(CBridgeAnalysisReportDlg, CSpanGirderReportDlg)
	ON_COMMAND(IDHELP, OnHelp)
END_MESSAGE_MAP()


// CBridgeAnalysisReportDlg message handlers

void CBridgeAnalysisReportDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_REPORT );
}

BOOL CBridgeAnalysisReportDlg::OnInitDialog()
{
   if ( m_pInitRptSpec )
   {
      std::shared_ptr<CBridgeAnalysisReportSpecification> pRptSpec = std::dynamic_pointer_cast<CBridgeAnalysisReportSpecification,WBFL::Reporting::ReportSpecification>(m_pInitRptSpec);
      m_bDesign = pRptSpec->ReportDesignResults();
      m_bRating = pRptSpec->ReportRatingResults();
   }

   // if there aren't any load rating types selected, don't report for rating and disable the UI element
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);
   if (!pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && 
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) &&
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
       !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      m_bRating = false;
      GetDlgItem(IDC_RATING)->EnableWindow(FALSE);
   }


   CSpanGirderReportDlg::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeAnalysisReportDlg::UpdateChapterList()
{
   // Don't call base class version... we want to redefine the behavior
   //__super::UpdateChapterList();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Clear out the list box
   m_ChList.ResetContent();

   // Get the chapters in the report
   std::vector<WBFL::Reporting::ChapterInfo> chInfos = m_RptDesc.GetChapterInfo();

   // Populate the list box with the names of the chapters
   std::vector<WBFL::Reporting::ChapterInfo>::iterator iter;
   for ( iter = chInfos.begin(); iter != chInfos.end(); iter++ )
   {
      WBFL::Reporting::ChapterInfo chInfo = *iter;

      bool bIncludeChapter = false;
      if ( chInfo.Name == _T("Simple Span") && (analysisType == pgsTypes::Simple || analysisType == pgsTypes::Envelope) )
         bIncludeChapter = true;
      else if ( chInfo.Name == _T("Continuous Span") && (analysisType == pgsTypes::Continuous || analysisType == pgsTypes::Envelope) )
         bIncludeChapter = true;
      else if ( chInfo.Name == _T("Envelope of Simple/Continuous Spans") && (analysisType == pgsTypes::Envelope) )
         bIncludeChapter = true;

      if ( bIncludeChapter )
      {
         int idx = m_ChList.AddString( chInfo.Name.c_str() );
         if ( idx != LB_ERR ) // no error
         {
            m_ChList.SetCheck( idx, chInfo.Select ? 1 : 0 ); // turn the check on
            m_ChList.SetItemData( idx, chInfo.MaxLevel );
         }
      }
   }

   // don't select any items in the chapter list
   m_ChList.SetCurSel(-1);
}
