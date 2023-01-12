///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include <EAF\EAFAboutDlg.h>
#include <afxlinkctrl.h>

class CAboutDlg : public CDialog
{
   DECLARE_DYNAMIC(CAboutDlg)

public:
   CAboutDlg(UINT nResourceID,UINT nIDTemplate=0,CWnd* pParent=nullptr);
   virtual ~CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
   afx_msg void OnAppListSelChanged();
   afx_msg void OnMoreInfo();
   afx_msg HBRUSH OnCtlColor(CDC*, CWnd*, UINT);

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   UINT m_ResourceID;
   CMFCLinkCtrl m_WSDOT;
   CMFCLinkCtrl m_TxDOT;
   CMFCLinkCtrl m_KDOT;
   CMFCLinkCtrl m_BridgeSight;

   CListBox m_AppList;
   CStatic m_Description;
};
