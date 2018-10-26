///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_SHEARSTEELPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_SHEARSTEELPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ShearSteelPage.h : header file
//
#include "ShearSteelGrid.h"
#include <Units\Measure.h>

class CGirderMainSheet;


/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage dialog

class CShearSteelPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CShearSteelPage)

// Construction
public:
	CShearSteelPage();
	~CShearSteelPage();

// Dialog Data
	//{{AFX_DATA(CShearSteelPage)
	enum { IDD = IDD_EDIT_SHEAR_STEEL };
	CComboBox	m_TfBarSize;
	CComboBox	m_BarSize;
	CComboBox	m_LastZone;
	CComboBox	m_MaterialName;
	//}}AFX_DATA

   CShearSteelGrid m_Grid;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CShearSteelPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CShearSteelPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRemoveRows();
	afx_msg void OnInsertRow();
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void OnEnableDelete(bool canDelete);
   void FillLastZone(int siz);
   void DoRemoveRows();
   void DoInsertRow();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHEARSTEELPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
