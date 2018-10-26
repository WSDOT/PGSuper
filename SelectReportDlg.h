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

#if !defined(AFX_SELECTREPORTDLG_H__42E8C624_2CDB_47EF_AB13_C0E6A8B944A6__INCLUDED_)
#define AFX_SELECTREPORTDLG_H__42E8C624_2CDB_47EF_AB13_C0E6A8B944A6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectReportDlg.h : header file
//

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CSelectReportDlg dialog

class CSelectReportDlg : public CDialog
{
// Construction
public:
   CSelectReportDlg(std::vector<std::string>& rptNames,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectReportDlg)
	enum { IDD = IDD_SELECTREPORT };
	//}}AFX_DATA
	std::string	m_ReportName;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectReportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   std::vector<std::string> m_RptNames;

	// Generated message map functions
	//{{AFX_MSG(CSelectReportDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnLbnDblclkList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTREPORTDLG_H__42E8C624_2CDB_47EF_AB13_C0E6A8B944A6__INCLUDED_)
