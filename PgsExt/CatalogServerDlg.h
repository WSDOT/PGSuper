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

#if !defined(AFX_CATALOGSERVERDLG_H__F378C140_8D1B_4031_B402_0739BCC54F06__INCLUDED_)
#define AFX_CATALOGSERVERDLG_H__F378C140_8D1B_4031_B402_0739BCC54F06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CatalogServerDlg.h : header file
//

#include <PgsExt\CatalogServers.h>

/////////////////////////////////////////////////////////////////////////////
// CCatalogServerDlg dialog

class CCatalogServerDlg : public CDialog
{
// Construction
public:
	CCatalogServerDlg(CWnd* pParent = nullptr);   // standard constructor
	CCatalogServerDlg(const CString& strExt,const CString& appName,CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCatalogServerDlg)
	enum { IDD = IDD_CATALOGSERVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


   CString m_Server;
   CCatalogServers m_Servers;
   CString m_TemplateFileExt;
   CString m_AppName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCatalogServerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void UpdateServerList();
   void UpdateButtonState();

	// Generated message map functions
	//{{AFX_MSG(CCatalogServerDlg)
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	virtual BOOL OnInitDialog() override;
	afx_msg void OnDblclkServers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnHelp();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CATALOGSERVERDLG_H__F378C140_8D1B_4031_B402_0739BCC54F06__INCLUDED_)
