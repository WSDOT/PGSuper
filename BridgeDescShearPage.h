///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCSHEARPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEDESCSHEARPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescShearPage.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include "BridgeDescShearGrid.h"
#include <Units\Measure.h>
#include <pgsExt\ShearData.h>

class CGirderMainSheet;
class CGirderDescDlg;


/////////////////////////////////////////////////////////////////////////////
// CGirderDescShearPage dialog

class CGirderDescShearPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDescShearPage)

// Construction
public:
	CGirderDescShearPage();
	~CGirderDescShearPage();


// Dialog Data
	//{{AFX_DATA(CGirderDescShearPage)
	enum { IDD = IDD_GIRDERDESC_SHEAR_STEEL };
	CComboBox	m_TfBarSize;
	CComboBox	m_BarSize;
	CComboBox	m_LastZone;
	//}}AFX_DATA

   CGirderDescShearGrid m_Grid;
   std::_tstring m_CurGrdName;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescShearPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescShearPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRemoveRows();
	afx_msg void OnInsertRow();
	afx_msg void OnRestoreDefaults();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void FillBarComboBox(CComboBox* pCB);

public:
	void RestoreToLibraryDefaults();
   void OnEnableDelete(bool canDelete);
   void FillLastZone(ZoneIndexType siz);
   void DoRemoveRows();
   void DoInsertRow();

   CShearData m_ShearData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCSHEARPAGE_H__CE0B8E35_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
