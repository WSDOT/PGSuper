///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_SPECMOMENTPAGE_H__ABC7A990_4226_4FE3_875C_75C41CE262DC__INCLUDED_)
#define AFX_SPECMOMENTPAGE_H__ABC7A990_4226_4FE3_875C_75C41CE262DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecMomentPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecMomentPage dialog

class CSpecMomentPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecMomentPage)

// Construction
public:
	CSpecMomentPage();
	~CSpecMomentPage();

// Dialog Data
	//{{AFX_DATA(CSpecMomentPage)
	enum { IDD = IDD_SPEC_MOMENT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecMomentPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecMomentPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECMOMENTPAGE_H__ABC7A990_4226_4FE3_875C_75C41CE262DC__INCLUDED_)
