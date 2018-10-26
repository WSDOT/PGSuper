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

#if !defined(AFX_STATUSMESSAGEDIALOG_H__E371357F_A5AF_4D06_AEEF_60EDC174B783__INCLUDED_)
#define AFX_STATUSMESSAGEDIALOG_H__E371357F_A5AF_4D06_AEEF_60EDC174B783__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatusMessageDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatusMessageDialog dialog

class CStatusMessageDialog : public CDialog
{
// Construction
public:
	CStatusMessageDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatusMessageDialog)
	enum { IDD = IDD_STATUS_DIALOG };
	CButton	m_CancelBtn;
	CString	m_Message;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusMessageDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   UINT m_HelpID;
   bool m_IsSevere;
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStatusMessageDialog)
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUSMESSAGEDIALOG_H__E371357F_A5AF_4D06_AEEF_60EDC174B783__INCLUDED_)
