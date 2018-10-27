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

#if !defined(AFX_LONGSTEELPAGE_H__CE0B8E34_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_LONGSTEELPAGE_H__CE0B8E34_312C_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LongSteelPage.h : header file
//

// for the grid
#ifndef _GXALL_H_
#include "gxwnd.h"
#include "gxctrl.h"
#endif

#include "LongSteelGrid.h"
#include <Units\Measure.h>

class CGirderMainSheet;

/////////////////////////////////////////////////////////////////////////////
// CLongSteelPage dialog

class CLongSteelPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CLongSteelPage)

// Construction
public:
	CLongSteelPage();
	~CLongSteelPage();

// Dialog Data
	//{{AFX_DATA(CLongSteelPage)
	enum { IDD = IDD_LONG_STEEL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   CLongSteelGrid m_Grid;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLongSteelPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLongSteelPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnInsertrow();
	afx_msg void OnRemoverows();
	afx_msg void OnAppendRow();
	//}}AFX_MSG
	afx_msg BOOL OnNcActivate(BOOL bActive);
   afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()

   CRebarMaterialComboBox m_cbRebar;

public:
   void GetRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade);
   void OnEnableDelete(bool canDelete);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LONGSTEELPAGE_H__CE0B8E34_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
