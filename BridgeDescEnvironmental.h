///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCENVIRONMENTAL_H__E197A145_2F6F_11D3_AD8C_00105A9AF985__INCLUDED_)
#define AFX_BRIDGEDESCENVIRONMENTAL_H__E197A145_2F6F_11D3_AD8C_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescEnvironmental.h : header file
//

#include "PGSuperAppPlugin\resource.h"


class CBridgeDescDlg;
/////////////////////////////////////////////////////////////////////////////
// CBridgeDescEnvironmental dialog

class CBridgeDescEnvironmental : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescEnvironmental)

// Construction
public:
	CBridgeDescEnvironmental();
	~CBridgeDescEnvironmental();

// Dialog Data
	//{{AFX_DATA(CBridgeDescEnvironmental)
	enum { IDD = IDD_BRIDGEDESC_ENVIRONMENTAL };
	int		m_Exposure;
	Float64	m_RelHumidity;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescEnvironmental)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeDescEnvironmental)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCENVIRONMENTAL_H__E197A145_2F6F_11D3_AD8C_00105A9AF985__INCLUDED_)
