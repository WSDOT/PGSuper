///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#if !defined(AFX_ProfilePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
#define AFX_ProfilePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProfilePage.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include "ProfileGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CProfilePage dialog

class CProfilePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CProfilePage)

// Construction
public:
	CProfilePage();
	~CProfilePage();

// Dialog Data
	//{{AFX_DATA(CProfilePage)
	enum { IDD = IDD_PROFILE_PAGE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   ProfileData2 m_ProfileData;
   IBroker* GetBroker();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CProfilePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CProfilePage)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnSort();
	virtual BOOL OnInitDialog() override;
	//}}AFX_MSG
   afx_msg void OnHelp();

   CProfileGrid m_Grid;

   friend CProfileGrid;

	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ProfilePage_H__92267AA2_B441_471E_8812_3D8C1DE65FDB__INCLUDED_)
