///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#pragma once

#include <ReportManager\ReportManager.h>
#include "SpanGirderReportDlg.h"

#include "resource.h"

class CBridgeAnalysisReportDlg : public CSpanGirderReportDlg
{
	DECLARE_DYNAMIC(CBridgeAnalysisReportDlg)

public:
	CBridgeAnalysisReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate = IDD_BRIDGEANALYSISREPORT,
      CWnd* pParent = NULL);   // standard constructor
	virtual ~CBridgeAnalysisReportDlg();

// Dialog Data
	enum { IDD = IDD_BRIDGEANALYSISREPORT };

   bool m_bDesign;
   bool m_bRating;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual void UpdateChapterList();

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	afx_msg void OnHelp();
	//}}AFX_MSG

   virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
