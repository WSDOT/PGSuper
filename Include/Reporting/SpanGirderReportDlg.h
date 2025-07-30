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

#pragma once

#include <ReportManager\ReportManager.h>
#include "resource.h"
#include <Reporting\SpanItemReportDlg.h>

// CSpanGirderReportDlg dialog


class CSpanGirderReportDlg : public CSpanItemReportDlg
{
	DECLARE_DYNAMIC(CSpanGirderReportDlg)

public:

   CSpanGirderReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate = IDD_SPANITEMREPORT,CWnd* pParent = nullptr);   // standard constructor
   ~CSpanGirderReportDlg();

// Dialog Data
	enum { IDD = IDD_SPANITEMREPORT };

   CSegmentKey m_SegmentKey;

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   void UpdateGirderComboBox() override;
   void UpdateSegmentComboBox();

   void InitFromRptSpec() override;


public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	BOOL OnInitDialog() override;
   afx_msg void OnGirderChanged() override;
   afx_msg void OnGroupChanged() override;
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
