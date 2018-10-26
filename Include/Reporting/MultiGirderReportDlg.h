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

#pragma once

#include <ReportManager\ReportManager.h>
#include "resource.h"

#include "SharedCTrls\MultiGirderSelectGrid.h" 

// CMultiGirderReportDlg dialog

class CMultiGirderReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CMultiGirderReportDlg)

public:
	CMultiGirderReportDlg(IBroker* pBroker,const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec,UINT nIDTemplate = IDD_MULTIGIRDERREPORT,CWnd* pParent = NULL);   // standard constructor
	virtual ~CMultiGirderReportDlg();

// Dialog Data
	enum { IDD = IDD_MULTIGIRDERREPORT };

   std::vector<SpanGirderHashType> m_SelGdrs;
   std::vector<std::_tstring> m_ChapterList;

private:
   CMultiGirderSelectGrid* m_pGrid;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void UpdateChapterList();
   void UpdateGirderComboBox();

   void ClearChapterCheckMarks();
   void InitChapterListFromSpec();
   void InitFromRptSpec();

   CCheckListBox	m_ChList;

   const CReportDescription& m_RptDesc;
   IBroker* m_pBroker;

   boost::shared_ptr<CReportSpecification> m_pInitRptSpec; // report spec for initializing the dialog

public:
	// Generated message map functions
	//{{AFX_MSG(CReportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
   afx_msg void OnBnClickedSelectAll();
   afx_msg void OnBnClickedClearAll();
};
