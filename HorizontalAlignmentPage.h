///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2006  Washington State Department of Transportation
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
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#if !defined(AFX_HORIZONTALALIGNMENTPAGE_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
#define AFX_HORIZONTALALIGNMENTPAGE_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HorizontalAlignmentPage.h : header file
//

#include "resource.h"
#include "HorizontalAlignmentGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CHorizontalAlignmentPage dialog

class CHorizontalAlignmentPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CHorizontalAlignmentPage)

// Construction
public:
	CHorizontalAlignmentPage();
	~CHorizontalAlignmentPage();

// Dialog Data
	//{{AFX_DATA(CHorizontalAlignmentPage)
	enum { IDD = IDD_ALIGNMENT_HORIZONTAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   AlignmentData2 m_AlignmentData;
   IBroker* GetBroker();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHorizontalAlignmentPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHorizontalAlignmentPage)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnSort();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg void OnHelp();

   CComPtr<IDirectionDisplayUnitFormatter> m_DirFormatter;
   CComPtr<IAngleDisplayUnitFormatter> m_AngleFormatter;

   CHorizontalAlignmentGrid m_Grid;

   friend CHorizontalAlignmentGrid;

	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HORIZONTALALIGNMENTPAGE_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
