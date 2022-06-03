///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#if !defined(AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_)
#define AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanGirderLayoutPage.h : header file
//
#include "resource.h"

#include "GirderNameGrid.h"
#include "GirderSpacingGrid.h"
#include "GirderTopWidthGrid.h"

class CSpanDetailsDlg;
interface IBeamFactory;

/////////////////////////////////////////////////////////////////////////////
// CSpanGirderLayoutPage dialog
class CSpanGirderLayoutPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpanGirderLayoutPage)

// Construction
public:
	CSpanGirderLayoutPage();
	~CSpanGirderLayoutPage();

   void Init(CSpanDetailsDlg* pParent);

   GirderIndexType m_MinGirderCount;

   void GetBeamFactory(IBeamFactory** ppFactory);
   
   // Dialog Data
	//{{AFX_DATA(CSpanGirderLayoutPage)
	enum { IDD = IDD_GIRDERLAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	CSpinButtonCtrl	m_NumGdrSpinner;
   CComboBox         m_cbGirderSpacingType;
	//}}AFX_DATA

   void GirderTypeChanged();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpanGirderLayoutPage)
	virtual BOOL OnSetActive();
   afx_msg void OnHelp();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
   // array index is pgsTypes::MemberEndType constant
   CGirderNameGrid    m_GirderNameGrid;
   std::array<CGirderSpacingGrid, 2> m_SpacingGrid;
   CGirderTopWidthGrid m_TopWidthGrid;

   GirderIndexType m_nGirders;
   std::array<DWORD, 2> m_GirderSpacingMeasure;

   std::array<GirderIndexType, 2> m_RefGirderIdx;
   std::array<Float64, 2> m_RefGirderOffset;
   std::array<pgsTypes::OffsetMeasurementType, 2> m_RefGirderOffsetType;

// Implementation
protected:

   void UpdateChildWindowState();

	// Generated message map functions
	//{{AFX_MSG(CSpanGirderLayoutPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnNumGirdersChanged(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnSameGirderSpacing();
   afx_msg void OnCopySpacingToStart();
   afx_msg void OnCopySpacingToEnd();
	afx_msg void OnPrevPierGirderSpacingMeasureChanged();
	afx_msg void OnNextPierGirderSpacingMeasureChanged();
	//}}AFX_MSG
   afx_msg void OnChangeSameGirderSpacing();
   afx_msg void OnChangeSameGirderType();
   afx_msg void OnCbnSelchangeNgdrsCombo();
   DECLARE_MESSAGE_MAP()

   void GetPierSkewAngles(Float64& skew1,Float64& skew2);

   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   void FillGirderSpacingMeasurementComboBox(int nIDC, pgsTypes::MemberEndType end, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure);
   void FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end);
   void FillRefGirderComboBox(pgsTypes::MemberEndType end);

   GirderIndexType GetMinGirderCount();
   void UpdateGirderSpacingState();
   void UpdateGirderNumState();
   void UpdateGirderTypeState();
   void UpdateGirderTopWidthState();

   std::array<DWORD, 2> m_CacheGirderSpacingMeasure;
   std::array<CGirderSpacing2, 2> m_GirderSpacingCache;

   std::array<GirderIndexType, 2> m_CacheRefGirderIdx;
   std::array<Float64, 2> m_CacheRefGirderOffset;
   std::array<pgsTypes::OffsetMeasurementType, 2> m_CacheRefGirderOffsetType;

   std::vector<CGirderTypeGroup> m_GirderTypeCache;
   std::vector<CGirderTopWidthGroup> m_TopWidthCache;

   bool IsAbutment(pgsTypes::MemberEndType end);
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_)
