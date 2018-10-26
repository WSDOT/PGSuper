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

#if !defined(AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_)
#define AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescGirderMaterialsPage.h : header file
//

#include "SameGirderTypeHyperLink.h"
#include "SlabOffsetHyperLink.h"

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
   
   CSameGirderTypeHyperLink m_GirderTypeHyperLink;
   CSlabOffsetHyperLink m_SlabOffsetHyperLink;

   bool m_bUseSameGirderType;

   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::SlabOffsetType m_SlabOffsetTypeCache;

   Float64 m_SlabOffset[2];
   CString m_strSlabOffsetCache[2];

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
	afx_msg void OnCopyMaterial();
	virtual BOOL OnInitDialog();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnStrandTypeChanged();
	afx_msg void OnHelp();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeGirderFc();
	afx_msg void OnMoreConcreteProperties();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnChangeGirderName();
	//}}AFX_MSG
   afx_msg LRESULT OnChangeSameGirderType(WPARAM wParam,LPARAM lParam);
   afx_msg LRESULT OnChangeSlabOffsetType(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   void ExchangeConcreteData(CDataExchange* pDX);

   void UpdateGirderTypeHyperLink();
   void UpdateGirderTypeControls();

   void UpdateSlabOffsetHyperLink();
   void UpdateSlabOffsetControls();

   void UpdateConcreteControls();
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void FillGirderComboBox();

   void UpdateEci();
   void UpdateEc();

   CString m_strUserEc;
   CString m_strUserEci;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCGIRDERMATERIALSPAGE_H__8E728BBC_7C32_451C_8144_FD67D9A6B578__INCLUDED_)
