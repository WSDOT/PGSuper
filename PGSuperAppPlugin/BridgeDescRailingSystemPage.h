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

#if !defined(AFX_BRIDGEDESCRAILINGSYSTEMPAGE_H__0D7F1CBE_871A_4E5F_950A_526C01DEBEDB__INCLUDED_)
#define AFX_BRIDGEDESCRAILINGSYSTEMPAGE_H__0D7F1CBE_871A_4E5F_950A_526C01DEBEDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescRailingSystemPage.h : header file
//

#include <PsgLib\RailingSystem.h>
#include <MfcTools\WideDropDownComboBox.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescRailingSystemPage dialog

class CBridgeDescRailingSystemPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescRailingSystemPage)

// Construction
public:
	CBridgeDescRailingSystemPage();
	~CBridgeDescRailingSystemPage();

// Dialog Data
	//{{AFX_DATA(CBridgeDescRailingSystemPage)
	enum { IDD = IDD_BRIDGEDESC_RAILINGSYSTEM };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescRailingSystemPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   void SetConcreteTypeLabel(UINT nID,pgsTypes::ConcreteType type);

   CRailingSystem m_LeftRailingSystem;
   CRailingSystem m_RightRailingSystem;
   EventIndexType m_EventIndex;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeDescRailingSystemPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnLeftSidewalk();
	afx_msg void OnRightSidewalk();
	afx_msg void OnLeftInteriorBarrier();
	afx_msg void OnRightInteriorBarrier();
   afx_msg void OnLeftMoreProperties();
   afx_msg void OnRightMoreProperties();
   afx_msg void OnHelp();
   afx_msg void OnLeftExteriorBarrierChanged();
   afx_msg void OnRightExteriorBarrierChanged();
   afx_msg void OnLeftUserEc();
   afx_msg void OnChangeLeftFc();
   afx_msg void OnChangeLeftDensity();
   afx_msg void OnRightUserEc();
   afx_msg void OnChangeRightFc();
   afx_msg void OnChangeRightDensity();
   afx_msg void OnEventChanging();
   afx_msg void OnEventChanged();

   afx_msg HBRUSH OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor);
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void OnMoreProperties(CRailingSystem* pRailingSystem,CString* pStrUserEc);

   void FillTrafficBarrierComboBoxes();
   void EnableSidewalkDimensions(BOOL bEnable,BOOL bLeft);
   void EnableInteriorBarrier(BOOL bEnable,int nID);

   void UpdateRightEc();
   void UpdateLeftEc();

   CString m_strLeftUserEc;
   CString m_strRightUserEc;

   CString m_CacheWidth[2];
   CString m_CacheLeftDepth[2];
   CString m_CacheRightDepth[2];
   int m_CacheInteriorBarrierIdx[2];   // state of the interior barrier combo box
   int m_CacheInteriorBarrierCheck[2]; // state of the interior barrier check box

   void UpdateLeftConcreteParametersToolTip();
   void UpdateRightConcreteParametersToolTip();
   CString UpdateConcreteParametersToolTip(CRailingSystem* pRailingSystem);
   CString m_strToolTip[2];

   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;
   Float64 m_MinNWCDensity;
   Float64 m_MaxLWCDensity;

   CWideDropDownComboBox m_LeftBrrCB;
   CWideDropDownComboBox m_RightBrrCB;

   bool IsDensityInRange(Float64 density,pgsTypes::ConcreteType type);

   int m_PrevEventIdx;
   void FillEventList();
   EventIndexType CreateEvent();

public:
   virtual BOOL OnKillActive();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCRAILINGSYSTEMPAGE_H__0D7F1CBE_871A_4E5F_950A_526C01DEBEDB__INCLUDED_)
