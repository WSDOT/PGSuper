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

#if !defined(AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_)
#define AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// exportCADData.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// exportCADData dialog

class exportCADData : public CDialog
{
// Construction
public:
    exportCADData(IBroker* pBroker,CWnd* pParent = NULL);
	~exportCADData(); 

// Dialog Data
	//{{AFX_DATA(exportCADData)
	enum { IDD = IDD_EXPORT_TXDOTCADDATA };
	int		m_Girder;
	int		m_Span;
	BOOL	m_IsExtended;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(exportCADData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	CComPtr<IBroker> m_pBroker;

	// Generated message map functions
	//{{AFX_MSG(exportCADData)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnSelchangeSpan();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

};


   //{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTCADDATA_H__6F026124_5AB7_42E4_B64F_3BB2C5BD08B8__INCLUDED_)
