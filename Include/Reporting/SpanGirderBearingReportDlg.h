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
#include <Reporting\SpanItemReportDlg.h>
#include "resource.h"
#include <IFace/AnalysisResults.h>

// CSpanGirderReportDlg dialog


class CSpanGirderBearingReportDlg : public CSpanItemReportDlg
{
	DECLARE_DYNAMIC(CSpanGirderBearingReportDlg)

public:

   
   CSpanGirderBearingReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,Mode mode,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,UINT nIDTemplate = IDD_SPANGIRDERREPORT,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSpanGirderBearingReportDlg();

// Dialog Data

   CSegmentKey m_SegmentKey;
   ReactionLocation m_Bearing;

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

   virtual void UpdateGirderComboBox() override;
   virtual void UpdateBearingComboBox();

   virtual void InitFromRptSpec() override;


public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	BOOL OnInitDialog() override;
	afx_msg void OnGroupChanged() override;
   afx_msg void OnGirderChanged() override;
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
