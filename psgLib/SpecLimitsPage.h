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

#if !defined(AFX_SPECLIMITSPAGE_H__D150B0F9_D673_4BB0_B36F_9E6C2CD6909B__INCLUDED_)
#define AFX_SPECLIMITSPAGE_H__D150B0F9_D673_4BB0_B36F_9E6C2CD6909B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecLimitsPage.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSpecLimitsPage dialog

class CSpecLimitsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecLimitsPage)

// Construction
public:
	CSpecLimitsPage();
	~CSpecLimitsPage();

// Dialog Data
	//{{AFX_DATA(CSpecLimitsPage)
	enum { IDD = IDD_SPEC_LIMITS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecLimitsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecLimitsPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
   afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnBnClickedCheckGirderSag();
   virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECLIMITSPAGE_H__D150B0F9_D673_4BB0_B36F_9E6C2CD6909B__INCLUDED_)
