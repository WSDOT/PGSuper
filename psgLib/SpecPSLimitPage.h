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

#if !defined(AFX_SpecStrandPage_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_)
#define AFX_SpecStrandPage_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecStrandPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecStrandPage dialog

class CSpecStrandPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecStrandPage)

// Construction
public:
	CSpecStrandPage();
	~CSpecStrandPage();

// Dialog Data
	//{{AFX_DATA(CSpecStrandPage)
	enum { IDD = IDD_SPEC_STRANDS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecStrandPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecStrandPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
	afx_msg void OnPsChecked();
	afx_msg void OnPtChecked();
	afx_msg void OnCheckPsAfterTransfer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void EnableControls(BOOL bEnable,UINT nSR,UINT nLR);
public:
   virtual BOOL OnSetActive();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpecStrandPage_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_)
