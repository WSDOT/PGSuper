///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_)
#define AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescGirderMaterialsPage.h : header file
//

#include "PGSuperAppPlugin\resource.h"

class CGirderDescDlg;

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage dialog

class CGirderDescGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDescGeneralPage)

// Construction
public:
	CGirderDescGeneralPage();
	~CGirderDescGeneralPage();

// Dialog Data
	//{{AFX_DATA(CGirderDescGeneralPage)
	enum { IDD = IDD_GIRDERDESC_GENERAL };
	CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;
	//}}AFX_DATA
   
   bool m_bUseSameGirderType;

   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::SlabOffsetType m_SlabOffsetTypeCache;

   Float64 m_SlabOffset[2];
   CString m_strSlabOffsetCache[2];

   pgsTypes::FilletType m_FilletType;
   pgsTypes::FilletType m_FilletTypeCache;

   Float64 m_Fillet;
   CString m_strFilletCache;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescGeneralPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnStrandTypeChanged();
	afx_msg void OnHelp();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeGirderFc();
   afx_msg void OnChangeEc();
   afx_msg void OnChangeEci();
	afx_msg void OnMoreConcreteProperties();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnChangeGirderName();
   afx_msg void OnBeforeChangeGirderName();
	afx_msg void OnConcreteStrength();
   afx_msg void OnConditionFactorTypeChanged();
   afx_msg void OnConstructionEventChanged();
   afx_msg void OnConstructionEventChanging();
   afx_msg void OnErectionEventChanged();
   afx_msg void OnErectionEventChanging();
   //}}AFX_MSG
   afx_msg void OnChangeSameGirderType();
   afx_msg void OnChangeSlabOffsetType();
   afx_msg void OnChangeFilletType();
	DECLARE_MESSAGE_MAP()

   void FillEventList();
   EventIDType CreateEvent();
	
   int m_PrevConstructionEventIdx;
   int m_PrevErectionEventIdx; // capture the erection stage when the combo box drops down so we can restore the value if CreateEvent fails

   void ExchangeConcreteData(CDataExchange* pDX);

   void UpdateGirderTypeControls();
   void UpdateSlabOffsetControls();

   void UpdateConcreteControls(bool bSkipEcCheckBoxes=false);
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void FillGirderComboBox();

   void UpdateEci();
   void UpdateEc();
   void UpdateFci();
   void UpdateFc();

   CString m_strUserEc;
   CString m_strUserEci;

   pgsTypes::LossMethod m_LossMethod;
   pgsTypes::TimeDependentModel m_TimeDependentModel;
   Float64 m_AgeAtRelease;

   int m_GirderNameIdx; // combo box index of current girder name just before it is changed
                        // (needed to revert the combobox if user doesn't want to change)
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_)
