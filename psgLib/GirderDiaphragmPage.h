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

#if !defined(AFX_GIRDERDIAPHRAGMPAGE_H__CE0B8E36_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_GIRDERDIAPHRAGMPAGE_H__CE0B8E36_312C_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderDiaphragmPage.h : header file
//

#include "DiaphragmGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CGirderDiaphragmPage dialog

class CGirderDiaphragmPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDiaphragmPage)

// Construction
public:
	CGirderDiaphragmPage();
	~CGirderDiaphragmPage();

// Dialog Data
	//{{AFX_DATA(CGirderDiaphragmPage)
	enum { IDD = IDD_GIRDER_DIAPHRAGM };
	//}}AFX_DATA

   void OnEnableDelete(bool canDelete);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDiaphragmPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDiaphragmPage)
		// NOTE: the ClassWizard will add member functions here
   afx_msg void OnAdd();
   afx_msg void OnDelete();
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

public:
   CDiaphragmGrid m_Grid;
   pgsTypes::SupportedDiaphragmTypes m_DiaphragmTypes;

   afx_msg void OnEdit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDIAPHRAGMPAGE_H__CE0B8E36_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
