///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <psgLib\HorizShearGrid.h>
#include <PgsExt\ShearData.h>

class CShearSteelPage;

// CAdditionalInterfaceShearBarDlg dialog

class CAdditionalInterfaceShearBarDlg : public CDialog
{
	DECLARE_DYNAMIC(CAdditionalInterfaceShearBarDlg)

public:
	CAdditionalInterfaceShearBarDlg(CShearSteelPage* pParent);   // standard constructor
	virtual ~CAdditionalInterfaceShearBarDlg();

   bool m_bAreZonesSymmetrical;
   WBFL::Materials::Rebar::Type m_RebarType;
   WBFL::Materials::Rebar::Grade m_RebarGrade;

   CShearData2::HorizontalInterfaceZoneVec m_HorizontalInterfaceZones;

   void GetLastZoneName(CString& strSymmetric, CString& strEnd);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EDIT_ADDITIONAL_INTERFACE_STEEL };
#endif

   void OnEnableHorizDelete(BOOL canDelete);
   void DoRemoveHorizRows();
   void DoInsertHorizRow();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

   // Generated message map functions
   //{{AFX_MSG(CAdditionalInterfaceShearBarDlg)
   afx_msg void OnRemoveHorizRows();
   afx_msg void OnInsertHorizRow();
   afx_msg void OnHelp();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CShearSteelPage* m_pParent;
   std::unique_ptr<CHorizShearGrid> m_pHorizGrid;
};
