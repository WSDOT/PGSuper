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
///////////////////////////////////////////////////////////////////////

// BridgeAnalysisReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Reporting\BridgeAnalysisReportDlg.h>
#include <Reporting\BridgeAnalysisReportSpecification.h>
#include <MFCTools\CustomDDX.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CBridgeAnalysisReportDlg dialog

IMPLEMENT_DYNAMIC(CBridgeAnalysisReportDlg, CSpanGirderReportDlg)

CBridgeAnalysisReportDlg::CBridgeAnalysisReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate,CWnd* pParent)
	: CSpanGirderReportDlg(pBroker,rptDesc,GirderAndChapters,pRptSpec,nIDTemplate, pParent)
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
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_BRIDGEANALYSISREPORT );
}

BOOL CBridgeAnalysisReportDlg::OnInitDialog()
{
   if ( m_pInitRptSpec )
   {
      boost::shared_ptr<CBridgeAnalysisReportSpecification> pRptSpec = boost::dynamic_pointer_cast<CBridgeAnalysisReportSpecification,CReportSpecification>(m_pInitRptSpec);
      m_bDesign = pRptSpec->ReportDesignResults();
      m_bRating = pRptSpec->ReportRatingResults();
   }

   CSpanGirderReportDlg::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
