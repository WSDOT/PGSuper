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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPECDESCRPAGE_H__C7F62023_4E36_11D2_9D5E_00609710E6CE__INCLUDED_)
#define AFX_SPECDESCRPAGE_H__C7F62023_4E36_11D2_9D5E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SpecDescrPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecDescrPage dialog

class CSpecDescrPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecDescrPage)

// Construction
public:
	CSpecDescrPage();
	~CSpecDescrPage();

// Dialog Data
	//{{AFX_DATA(CSpecDescrPage)
	enum { IDD = IDD_SPEC_DESCR };
	//}}AFX_DATA


   WBFL::LRFD::LRFDVersionMgr::Version GetSpecVersion();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecDescrPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecDescrPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
   afx_msg void OnSpecificationChanged();
	//}}AFX_MSG
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

   CCacheComboBox m_cbSpecification;

public:
   afx_msg void OnBnClickedUseCurrentVersion();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECDESCRPAGE_H__C7F62023_4E36_11D2_9D5E_00609710E6CE__INCLUDED_)
