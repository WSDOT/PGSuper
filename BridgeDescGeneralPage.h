///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCGENERALPAGE_H__19E2FE14_D457_4B8F_960B_948E66E20966__INCLUDED_)
#define AFX_BRIDGEDESCGENERALPAGE_H__19E2FE14_D457_4B8F_960B_948E66E20966__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BridgeDescGeneralPage.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <IFace\BeamFactory.h>
#include <PgsExt\DeckDescription2.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescGeneralPage dialog

class CBridgeDescGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBridgeDescGeneralPage)

// Construction
public:
	CBridgeDescGeneralPage();
	~CBridgeDescGeneralPage();

// Dialog Data
	//{{AFX_DATA(CBridgeDescGeneralPage)
	enum { IDD = IDD_BRIDGEDESC_GENERAL };
	CSpinButtonCtrl	m_NumGdrSpinner;
   CStatic  m_AlignmentOffsetFormat;
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   Float64 m_AlignmentOffset;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescGeneralPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   CString GetGirderFamily() { return m_GirderFamilyName; }

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBridgeDescGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSameNumGirders();
	afx_msg void OnSameGirderName();
	afx_msg void OnNumGirdersChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGirderFamilyChanged();
   afx_msg void OnBeforeChangeGirderName();
	afx_msg void OnGirderNameChanged();
	afx_msg void OnDeckTypeChanged();
	afx_msg void OnGirderConnectivityChanged();
   afx_msg void OnSpacingDatumChanged();
   afx_msg void OnGirderSpacingTypeChanged();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnHelp();
	afx_msg void OnPostTension();
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void Init();
   void InitGirderName();
   void UpdateBridgeDescription();
   void FillGirderFamilyComboBox();
   void FillGirderNameComboBox();
   void FillGirderOrientationComboBox();
   void FillGirderSpacingTypeComboBox();
   void FillGirderSpacingMeasurementComboBox();
   void FillDeckTypeComboBox();
   void FillRefGirderOffsetTypeComboBox();
   void FillRefGirderComboBox();
   void FillGirderConnectivityComboBox();
   void UpdatePTControls();

   void UpdateGirderFactory();
   CString GetDeckString(pgsTypes::SupportedDeckType deckType);
   void UpdateSuperstructureDescription();
   BOOL UpdateGirderSpacingLimits();
   void UpdateGirderConnectivity();
   void UpdateMinimumGirderCount();

   void EnableNumGirderLines(BOOL bEnable);
   void EnableGirderName(BOOL bEnable);
   void EnableGirderSpacing(BOOL bEnable,BOOL bClearControls);
   bool AreAnyBearingsMeasuredAlongGirder();

   void UIHint(const CString& strText,UINT mask);

   CComPtr<IBeamFactory> m_Factory;
   GirderIndexType m_MinGirderCount;

   bool m_bSameNumberOfGirders;
   bool m_bSameGirderName;
   bool m_bPostTension;

   GirderIndexType m_nGirders;
   CString m_GirderName;
   CString m_GirderFamilyName;
   pgsTypes::GirderOrientationType m_GirderOrientation;
   Float64 m_GirderSpacing;
   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   pgsTypes::MeasurementType      m_GirderSpacingMeasurementType;
   pgsTypes::MeasurementLocation  m_GirderSpacingMeasurementLocation;

   GirderIndexType m_RefGirderIdx;
   Float64 m_RefGirderOffset;
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;

   Float64 m_MinGirderSpacing;
   Float64 m_MaxGirderSpacing;
   Float64 m_GirderSpacingTolerance;

   CDeckDescription2 m_Deck;
   std::vector<CDeckPoint> m_CacheDeckEdgePoints;

   CString m_strToolTipText; // buffer for storing tool tip text

   GirderIndexType m_GirderNameIdx;

   bool m_bSetActive; // true if call come from OnSetActive

   // cache values so that some controls can be blanked and then
   // restored
   CString m_strCacheNumGirders;
   CString m_strCacheGirderSpacing;
   CString m_strCacheJointSpacing;
   CString m_strCacheRefGirderOffset;
   int m_CacheGirderNameIdx;
   int m_CacheGirderSpacingMeasureIdx;
   int m_CacheUseSameGirderNameBtn;
   int m_CacheDeckTypeIdx;
   int m_CacheGirderConnectivityIdx;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCGENERALPAGE_H__19E2FE14_D457_4B8F_960B_948E66E20966__INCLUDED_)
