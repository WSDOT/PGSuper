///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_)
#define AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecDesignPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage dialog

class CSpecDesignPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDesignPage)
// Construction
public:
	CSpecDesignPage(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSpecDesignPage)
	enum { IDD = IDD_SPEC_DESIGN };
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpecDesignPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CSpecDesignPage)
	afx_msg void OnCheckA();
   afx_msg void OnDesignA();
	afx_msg void OnCheckHauling();
	afx_msg void OnCheckHd();
	afx_msg void OnCheckLifting();
	afx_msg void OnCheckSlope();
	afx_msg void OnCheckSplitting();
	afx_msg void OnCheckConfinement();
   afx_msg void OnBnClickedIsSupportLessThan();
   afx_msg void OnBnClicked90DayStrength();
   afx_msg void OnBnClickedCheckBottomFlangeClearance();
   afx_msg void OnBnClickedCheckInclindedGirder();
   afx_msg void OnBnClickedLlDeflection();
   afx_msg void OnBnClickedCheckHandlingWeight();
   //}}AFX_MSG
   afx_msg void OnHelp();
   afx_msg void OnFcTypeChanged();
   DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnSetActive();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpecDesignPage_H__6A54414E_08FE_4D7B_BB7A_846CEC129E71__INCLUDED_)
