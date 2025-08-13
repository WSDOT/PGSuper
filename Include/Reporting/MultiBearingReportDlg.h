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

#include "SharedCTrls\MultiBearingSelectGrid.h" 

// CMultiGirderReportDlg dialog

class CMultiBearingReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CMultiBearingReportDlg)

public:
	CMultiBearingReportDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,const WBFL::Reporting::ReportDescription& rptDesc,
		std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec,
		UINT nIDTemplate = IDD_MULTIBEARINGREPORT,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CMultiBearingReportDlg();

// Dialog Data
	enum { IDD = IDD_MULTIBEARINGREPORT };

   std::vector<ReactionLocation> m_Bearings;
   std::vector<std::_tstring> m_ChapterList;

private:
   CMultiBearingSelectGrid* m_pGrid;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void UpdateChapterList();


   CCheckListBox	m_ChList;

   const WBFL::Reporting::ReportDescription& m_RptDesc;
   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;

   std::shared_ptr<WBFL::Reporting::ReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
   afx_msg void OnBnClickedSelectAll();
   afx_msg void OnBnClickedClearAll();
};
