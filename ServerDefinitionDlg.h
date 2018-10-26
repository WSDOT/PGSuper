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

#if !defined(AFX_SERVERDEFINITIONDLG_H__1FA6E7C8_E40B_47CD_967A_9D429C8FE2BA__INCLUDED_)
#define AFX_SERVERDEFINITIONDLG_H__1FA6E7C8_E40B_47CD_967A_9D429C8FE2BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerDefinitionDlg.h : header file
//

#include "PGSuperCatalogServers.h"

/////////////////////////////////////////////////////////////////////////////
// CServerDefinitionDlg dialog

class CServerDefinitionDlg : public CDialog
{
// Construction
public:
	CServerDefinitionDlg(const CPGSuperCatalogServers& servers,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServerDefinitionDlg)
	enum { IDD = IDD_SERVERDATA };
	CString	m_ServerName;
	CString	m_ServerAddress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerDefinitionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   const CPGSuperCatalogServers& m_Servers;

// Implementation
protected:
   CString m_OriginalName;

	// Generated message map functions
	//{{AFX_MSG(CServerDefinitionDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERDEFINITIONDLG_H__1FA6E7C8_E40B_47CD_967A_9D429C8FE2BA__INCLUDED_)
