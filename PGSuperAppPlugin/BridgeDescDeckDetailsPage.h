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

#if !defined(AFX_BRIDGEDESCDECKDETAILSPAGE_H__4050F4AA_347A_4641_8D01_1068EB44B3CC__INCLUDED_)
#define AFX_BRIDGEDESCDECKDETAILSPAGE_H__4050F4AA_347A_4641_8D01_1068EB44B3CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescDeckDetailsPage.h : header file
//

class CBridgeDescDlg;

#include "resource.h"
#include <PgsExt\DeckDescription.h>
#include "BridgeDescDeckPointGrid.h"
#include <PgsExt\HaunchShapeComboBox.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckDetailsPage dialog

class CBridgeDescDeckDetailsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescDeckDetailsPage)

// Construction
public:
	CBridgeDescDeckDetailsPage();
	~CBridgeDescDeckDetailsPage();

// Dialog Data
	//{{AFX_DATA(CBridgeDescDeckDetailsPage)
	enum { IDD = IDD_BRIDGEDESC_DECKDETAILS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	CEdit	m_ctrlEc;
	CButton m_ctrlEcCheck;
	CEdit	m_ctrlFc;
   CCacheEdit m_ctrlLeftOverhangEdgeDepth;
   CCacheEdit m_ctrlRightOverhangEdgeDepth;
   CCacheComboBox m_ctrlOverhangTaper;
   CCacheEdit m_ctrlPanelDepth;
   CCacheEdit m_ctrlPanelSupportWidth;
   //}}AFX_DATA

   void EnableRemove(BOOL bEnable);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDeckDetailsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeDescDeckDetailsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
   afx_msg void OnUserEc();
	afx_msg void OnChangeSlabFc();
   afx_msg void OnWearingSurfaceTypeChanged();
	afx_msg void OnMoreConcreteProperties();
   afx_msg void OnAddDeckEdgePoint();
   afx_msg void OnRemoveDeckEdgePoint();
   afx_msg void OnConditionFactorTypeChanged();
   afx_msg void OnDeckEventChanged();
   afx_msg void OnDeckEventChanging();
   afx_msg void OnDeckEventDetails();
   afx_msg void OnOverlayEventChanged();
   afx_msg void OnOverlayEventChanging();
	//}}AFX_MSG
   BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

   void ExchangeConcreteData(CDataExchange* pDX);
   void UpdateEc();

   CString m_strUserEc;

   void UpdateConcreteControls();
   void UpdateConcreteParametersToolTip();
   
   void UpdateHaunchAndCamberControls();
   void UpdateDeckRelatedControls();

   CString m_strTip;
   CBridgeDescDeckPointGrid m_Grid;

   int m_PrevDeckEventIdx;
   int m_PrevOverlayEventIdx;

   void FillEventList();
   EventIndexType CreateEvent();

   Float64 m_AgeAtContinuity; // Age when deck becomes composite with girders (days)

   CHaunchShapeComboBox m_cbHaunchShape;

public:
   afx_msg void OnStnClickedOlayDensityUnit();
   afx_msg void OnBnClickedOlayWeightLabel();
   afx_msg void OnBnClickedOlayDepthLabel();
   afx_msg void OnBnClickedEditHaunchButton();
   afx_msg void OnCbnSelchangeHaunchShape2();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDECKDETAILSPAGE_H__4050F4AA_347A_4641_8D01_1068EB44B3CC__INCLUDED_)
