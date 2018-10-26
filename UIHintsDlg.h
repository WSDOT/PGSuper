///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_UIHINTSDLG_H__8E25A905_1177_46FC_A1B0_6F61651F0234__INCLUDED_)
#define AFX_UIHINTSDLG_H__8E25A905_1177_46FC_A1B0_6F61651F0234__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UIHintsDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CUIHintsDlg dialog

class CUIHintsDlg : public CDialog
{
// Construction
public:
	CUIHintsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUIHintsDlg)
	enum { IDD = IDD_UIHINTS };
	CString	m_strText;
	BOOL	m_bDontShowAgain;
	//}}AFX_DATA
   CString m_strTitle;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIHintsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUIHintsDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UIHINTSDLG_H__8E25A905_1177_46FC_A1B0_6F61651F0234__INCLUDED_)
