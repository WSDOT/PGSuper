///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#if !defined(AFX_GIRDERSTRAIGHTSTRANDPAGE_H__73E59733_FCDE_11D2_AD4C_00105A9AF985__INCLUDED_)
#define AFX_GIRDERSTRAIGHTSTRANDPAGE_H__73E59733_FCDE_11D2_AD4C_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GirderStraightStrandPage.h : header file
//
#include "GirderStrandGrid.h"
#include <Units\Measure.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderTemporaryStrandPage dialog

class CGirderTemporaryStrandPage : public CPropertyPage, public CGirderStrandGridClient
{
	DECLARE_DYNCREATE(CGirderTemporaryStrandPage)

   friend CGirderMainSheet;

// Construction
public:
	CGirderTemporaryStrandPage();
	~CGirderTemporaryStrandPage();

// Dialog Data
	//{{AFX_DATA(CGirderTemporaryStrandPage)
	enum { IDD = IDD_GIRDER_TEMPORARY_STRANDS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

private:
   CGirderStrandGrid m_TemporaryGrid;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderTemporaryStrandPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderTemporaryStrandPage)
	afx_msg void OnAddTemporaryStrand();
	afx_msg void OnDelTemporaryStrand();
	virtual BOOL OnInitDialog();
	afx_msg void OnAppendTemporaryStrand();
	//}}AFX_MSG
	afx_msg BOOL OnNcActivate(BOOL bActive);
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

public:
   void OnEnableDelete(const CGirderStrandGrid* pgrid, bool canDelete);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERSTRAIGHTSTRANDPAGE_H__73E59733_FCDE_11D2_AD4C_00105A9AF985__INCLUDED_)
