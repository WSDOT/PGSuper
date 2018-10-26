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

#if !defined(AFX_BRIDGEDESCLONGITUDINALREBAR_H__3D8001A3_0389_4E5C_A379_72A701728B20__INCLUDED_)
#define AFX_BRIDGEDESCLONGITUDINALREBAR_H__3D8001A3_0389_4E5C_A379_72A701728B20__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescLongitudinalRebar.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include "BridgeDescLongRebarGrid.h"

class CGirderDescDlg;

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongitudinalRebar dialog

class CGirderDescLongitudinalRebar : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDescLongitudinalRebar)

// Construction
public:
	CGirderDescLongitudinalRebar();
	~CGirderDescLongitudinalRebar();

// Dialog Data
	//{{AFX_DATA(CGirderDescLongitudinalRebar)
	enum { IDD = IDD_GIRDERDESC_LONGITUDINAL_REBAR };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescLongitudinalRebar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   void OnEnableDelete(bool canDelete);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescLongitudinalRebar)
	afx_msg void OnRestoreDefaults();
	virtual BOOL OnInitDialog();
	afx_msg void OnInsertrow();
	afx_msg void OnAppendRow();
	afx_msg void OnRemoveRows();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void RestoreToLibraryDefaults();

public:
   CGirderDescLongRebarGrid m_Grid;
   CLongitudinalRebarData m_RebarData;
   std::_tstring m_CurGrdName;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCLONGITUDINALREBAR_H__3D8001A3_0389_4E5C_A379_72A701728B20__INCLUDED_)
