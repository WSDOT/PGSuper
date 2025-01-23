///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_SPECLOSSPAGE_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_)
#define AFX_SPECLOSSPAGE_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecLossPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecLossPage dialog

class CSpecLossPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecLossPage)

// Construction
public:
	CSpecLossPage();
	~CSpecLossPage();

// Dialog Data
	//{{AFX_DATA(CSpecLossPage)
	enum { IDD = IDD_SPEC_LOSSES };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecLossPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void InitComboBoxes();
   // Generated message map functions
	//{{AFX_MSG(CSpecLossPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   afx_msg void OnLossMethodChanged();
	afx_msg void OnShippingLossMethodChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   WBFL::LRFD::BDSManager::Edition m_SpecVersion;
   bool m_IsShippingEnabled;
   void EnableShippingLosses(BOOL bEnable);
   void EnableRefinedShippingTime(BOOL bEnable);
   void EnableApproximateShippingTime(BOOL bEnable);
   void EnableRelaxation(BOOL bEnable);
   void EnableElasticGains(BOOL bEnable, BOOL bEnableDeckShrinkage);
   void EnableTimeDependentModel(BOOL bEnable);
   void EnableTxDOT2013(BOOL bEnable);
   BOOL IsFractionalShippingLoss();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECLOSSPAGE_H__FE3DB4E5_D66A_11D2_88FA_006097C68A9C__INCLUDED_)
