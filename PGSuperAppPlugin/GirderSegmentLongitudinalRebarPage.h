///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#pragma once


#include "BridgeDescLongRebarGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage dialog

class CGirderSegmentLongitudinalRebarPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderSegmentLongitudinalRebarPage)

// Construction
public:
	CGirderSegmentLongitudinalRebarPage();
	~CGirderSegmentLongitudinalRebarPage();

// Dialog Data
	//{{AFX_DATA(CGirderSegmentLongitudinalRebarPage)
	enum { IDD = IDD_SEGMENT_LONGITUDINAL_REBAR };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderSegmentLongitudinalRebarPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   void OnEnableDelete(bool canDelete);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderSegmentLongitudinalRebarPage)
	afx_msg void OnRestoreDefaults();
	virtual BOOL OnInitDialog();
	afx_msg void OnInsertrow();
	afx_msg void OnAppendRow();
	afx_msg void OnRemoveRows();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void RestoreToLibraryDefaults();

   CRebarMaterialComboBox m_cbRebar;

public:
   CGirderDescLongRebarGrid m_Grid;
};
