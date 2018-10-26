///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#if !defined(AFX_SPANCONNECTIONSPAGE_H__70CF49BD_8383_4AC9_B56F_7ED32BF03166__INCLUDED_)
#define AFX_SPANCONNECTIONSPAGE_H__70CF49BD_8383_4AC9_B56F_7ED32BF03166__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanConnectionsPage.h : header file
//
#include "PGSuperAppPlugin\resource.h"

#include <PgsExt\PierData.h>
#include "ConnectionComboBox.h"

/////////////////////////////////////////////////////////////////////////////
// CSpanConnectionsPage dialog

class CSpanConnectionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpanConnectionsPage)

// Construction
public:
	CSpanConnectionsPage();
	~CSpanConnectionsPage();

// Dialog Data
	//{{AFX_DATA(CSpanConnectionsPage)
	enum { IDD = IDD_SPANCONNECTIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpanConnectionsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   afx_msg void OnHelp();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpanConnectionsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangePrevPierBackConnection();
	afx_msg void OnSelchangePrevPierAheadConnection();
	afx_msg void OnSelchangeNextPierBackConnection();
	afx_msg void OnSelchangeNextPierAheadConnection();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


   CConnectionComboBox m_cbAheadBoundaryCondition, m_cbBackBoundaryCondition;

   void InitializeComboBoxes();
   void LabelGroupBoxes();
   void FillWithConnections(CComboBox* pCB);
   void FillWithBoundaryConditions(CComboBox* pCB,bool bIncludeContinuity);

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPANCONNECTIONSPAGE_H__70CF49BD_8383_4AC9_B56F_7ED32BF03166__INCLUDED_)
