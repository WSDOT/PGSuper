///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// BridgeDescFramingPage.h : header file
//
#include <PgsExt\PierData.h>
#include "BridgeDescFramingGrid.h"
#include "PGSuperAppPlugin\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage dialog

class CBridgeDescFramingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescFramingPage)

// Construction
public:
	CBridgeDescFramingPage();
	~CBridgeDescFramingPage();

// Dialog Data
	//{{AFX_DATA(CBridgeDescFramingPage)
	enum { IDD = IDD_BRIDGEDESC_FRAMING };
	CStatic	m_OrientationFormat;
	CStatic	m_StationFormat;
   CStatic  m_AlignmentOffsetFormat;
	//}}AFX_DATA

   CBridgeDescFramingGrid m_Grid;
   Float64 m_AlignmentOffset;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescFramingPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnAddPier();
	afx_msg void OnRemovePier();
   afx_msg void OnLayoutBySpanLengths();
   afx_msg void OnAddTemporarySupport();
   afx_msg void OnRemoveTemporarySupport();
	//}}AFX_VIRTUAL

// Implementation
public:
   void EnableRemovePierBtn(BOOL bEnable);
   void EnableRemoveTemporarySupportBtn(BOOL bEnable);

protected:
   // Generated message map functions
	//{{AFX_MSG(CBridgeDescFramingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg BOOL OnNcActivate(BOOL bActive);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnOrientPiers();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
