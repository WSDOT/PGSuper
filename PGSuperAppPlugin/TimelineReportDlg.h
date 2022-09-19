///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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


#include "resource.h"
#include <Reporting\TimelineManagerReportSpecification.h>

/////////////////////////////////////////////////////////////////////////////
// CTimelineReportDlg dialog

class CTimelineReportDlg : public CDialog
{
// Construction
public:
   CTimelineReportDlg(std::shared_ptr<CTimelineManagerReportSpecification>& pRptSpec, CWnd* pParent=nullptr);

// Dialog Data
	//{{AFX_DATA(CTimelineReportDlg)
	enum { IDD = IDD_TIMELINE_REPORT };
	//}}AFX_DATA

// Operations

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimelineReportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   std::shared_ptr<CTimelineManagerReportSpecification> m_pRptSpec;
   std::shared_ptr<WBFL::Reporting::ReportBrowser> m_pBrowser; // this is the actual browser window that displays the report

	// Generated message map functions
	//{{AFX_MSG(CDesignOutcomeDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPrint();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
   afx_msg void OnCmenuSelected(UINT id);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void CleanUp();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
