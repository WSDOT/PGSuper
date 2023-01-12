///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#if !defined(AFX_SHEARPAGE_H__INCLUDED_)
#define AFX_SHEARPAGE_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ShearSteelPage.h : header file
//
#include "psgLibLib.h"
#include "resource.h"
#include <Units\Measure.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <psgLib\ShearData.h>

#include <psgLib\ShearSteelGrid.h>

class PSGLIBCLASS CShearSteelPageParent
{
public:
   virtual bool HasDeck() const = 0;
   virtual LPCTSTR GetIntentionalRougheningPrompt() const = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage dialog

class PSGLIBCLASS CShearSteelPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CShearSteelPage)

// Construction
public:
	CShearSteelPage();
	~CShearSteelPage();

// Dialog Data
	//{{AFX_DATA(CShearSteelPage)
   static const DWORD  IDD;
	//}}AFX_DATA
protected:
   std::unique_ptr<CShearSteelGrid> m_pGrid;
public:
   std::_tstring m_CurGrdName;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CShearSteelPage)
	public:
	virtual BOOL OnSetActive();
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
	afx_msg void OnHelp();
   afx_msg void OnClickedSymmetrical();
   afx_msg void OnRestoreDefaults();
   afx_msg void OnBnClickedAdditionalInterfaceShearBars();
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void FillBarComboBox(CComboBox* pCB);

public:
   void OnEnableDelete(bool canDelete);
   void DoRemoveRows();
   void DoInsertRow();

   CShearData2 m_ShearData;

   void GetRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade);

   virtual void GetLastZoneName(CString& strSymmetric, CString& strEnd);

    virtual void DoRestoreDefaults(); // Allow children to implement
protected:
    bool m_bAllowRestoreDefaults;
    CRebarMaterialComboBox m_cbRebar;

    virtual UINT GetHelpID();

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHEARPAGE_H__INCLUDED_)
