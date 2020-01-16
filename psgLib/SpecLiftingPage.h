///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#if !defined(AFX_SPECLIFTINGPAGE_H__97C0ED93_E149_11D2_AD38_00105A9AF985__INCLUDED_)
#define AFX_SPECLIFTINGPAGE_H__97C0ED93_E149_11D2_AD38_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecLiftingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecLiftingPage dialog

class CSpecLiftingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecLiftingPage)

// Construction
public:
	CSpecLiftingPage();
	~CSpecLiftingPage();

// Dialog Data
	//{{AFX_DATA(CSpecLiftingPage)
	enum { IDD = IDD_SPEC_LIFTING };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecLiftingPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:

protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecLiftingPage)
	afx_msg void OnCheckLiftingNormalMaxMax();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

	void DoCheckMax();
public:
   virtual BOOL OnSetActive();

   void EnableControls(BOOL bEnable);
   afx_msg void OnCbnSelchangeWindType();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECLIFTINGPAGE_H__97C0ED93_E149_11D2_AD38_00105A9AF985__INCLUDED_)
