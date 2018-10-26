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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPECCASTINGYARDPAGE_H__22F9FD44_4F00_11D2_9D5F_00609710E6CE__INCLUDED_)
#define AFX_SPECCASTINGYARDPAGE_H__22F9FD44_4F00_11D2_9D5F_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SpecCastingYardPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecCastingYardPage dialog

class CSpecCastingYardPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecCastingYardPage)

// Construction
public:
	CSpecCastingYardPage();
	~CSpecCastingYardPage();

// Dialog Data
	//{{AFX_DATA(CSpecCastingYardPage)
	enum { IDD = IDD_SPEC_CASTING_YARD };
	CStatic	m_StaticSlope07;
	CStatic	m_StaticSlope06;
	CStatic	m_StaticSlope05;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecCastingYardPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   bool m_DoCheckStrandSlope;
   bool m_DoCheckHoldDown;
   bool m_DoCheckSplitting;

// Implementation
protected:
	void DoCheckHoldDown();
	void DoCheckStrandSlope();
	void DoCheckMax();
	void DoCheckAnchorage();
	// Generated message map functions
	//{{AFX_MSG(CSpecCastingYardPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckMax();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECCASTINGYARDPAGE_H__22F9FD44_4F00_11D2_9D5F_00609710E6CE__INCLUDED_)
