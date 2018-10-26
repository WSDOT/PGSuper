///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#if !defined(AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_)
#define AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecDeflectionsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDeflectionsPage dialog

class CSpecDeflectionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDeflectionsPage)

// Construction
public:
	CSpecDeflectionsPage();
	~CSpecDeflectionsPage();

// Dialog Data
	//{{AFX_DATA(CSpecDeflectionsPage)
	enum { IDD = IDD_SPEC_DEFLECTIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecDeflectionsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void DoCheckLlDeflection();
	// Generated message map functions
	//{{AFX_MSG(CSpecDeflectionsPage)
	afx_msg void OnCheckLlDeflection();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECDEFLECTIONSPAGE_H__34909A2B_1E2E_496E_9C60_B1021A9AC123__INCLUDED_)
