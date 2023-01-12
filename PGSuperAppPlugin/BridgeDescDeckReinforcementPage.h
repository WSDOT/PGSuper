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

#if !defined(AFX_BRIDGEDESCDECKREINFORCEMENTPAGE_H__143D3CF3_E7B8_492D_9A05_FA3A7BA1B8B6__INCLUDED_)
#define AFX_BRIDGEDESCDECKREINFORCEMENTPAGE_H__143D3CF3_E7B8_492D_9A05_FA3A7BA1B8B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescDeckReinforcementPage.h : header file
//

#include "resource.h"
#include "BridgeDescDeckRebarGrid.h"
#include <PgsExt\DeckRebarData.h>

class CBridgeDescDlg;

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckReinforcementPage dialog

class CBridgeDescDeckReinforcementPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescDeckReinforcementPage)

// Construction
public:
	CBridgeDescDeckReinforcementPage();
	~CBridgeDescDeckReinforcementPage();

// Dialog Data
	//{{AFX_DATA(CBridgeDescDeckReinforcementPage)
	enum { IDD = IDD_BRIDGEDESC_DECKREINFORCEMENT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   void EnableAddBtn(bool bEnable);
   void EnableRemoveBtn(bool bEnable);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDeckReinforcementPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeDescDeckReinforcementPage)
	afx_msg void OnHelp();
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

   void FillRebarComboBox(CComboBox* pcbRebar);

   CString m_strTip;

public:
   CBridgeDescDeckRebarGrid m_Grid;

private:
   CRebarMaterialComboBox m_cbRebar;
   CDeckRebarData m_RebarData;
   friend CBridgeDescDeckRebarGrid;

   void GetRebarMaterial(WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDECKREINFORCEMENTPAGE_H__143D3CF3_E7B8_492D_9A05_FA3A7BA1B8B6__INCLUDED_)
