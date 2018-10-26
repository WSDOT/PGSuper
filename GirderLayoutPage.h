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

#if !defined(AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_)
#define AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpanGirderLayoutPage.h : header file
//
#include "PGSuperAppPlugin\resource.h"

#include "GirderNameGrid.h"
#include "GirderSpacingGrid.h"
#include "SameNumberOfGirdersHyperLink.h"
#include "SameGirderSpacingHyperLink.h"
#include "SameGirderTypeHyperLink.h"
#include "SameSlabOffsetHyperLink.h"

class CSpanDetailsDlg;

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

// Dialog Data
	//{{AFX_DATA(CSpanGirderLayoutPage)
	enum { IDD = IDD_GIRDERLAYOUT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	CSpinButtonCtrl	m_NumGdrSpinner;
	//}}AFX_DATA
   CSameNumberOfGirdersHyperLink m_NumGirdersHyperLink;
   CSameGirderSpacingHyperLink   m_GirderSpacingHyperLink;
   CSameGirderTypeHyperLink      m_GirderTypeHyperLink;


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
   CGirderSpacingGrid m_SpacingGrid[2];

   GirderIndexType m_nGirders;
   DWORD m_GirderSpacingMeasure[2];

   GirderIndexType m_RefGirderIdx[2];
   Float64 m_RefGirderOffset[2];
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType[2];

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
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
   afx_msg LRESULT OnChangeSameNumberOfGirders(WPARAM wParam,LPARAM lParam);
   afx_msg LRESULT OnChangeSameGirderSpacing(WPARAM wParam,LPARAM lParam);
   afx_msg LRESULT OnChangeSameGirderType(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   void GetPierSkewAngles(Float64& skew1,Float64& skew2);

   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   void FillGirderSpacingMeasurementComboBox(int nIDC, pgsTypes::MemberEndType end, ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure);
   void FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end);
   void FillRefGirderComboBox(pgsTypes::MemberEndType end);

   GirderIndexType GetMinGirderCount();
   void UpdateGirderSpacingState();

   DWORD m_CacheGirderSpacingMeasure[2];
   CGirderSpacing2 m_GirderSpacingCache[2];

   GirderIndexType m_CacheRefGirderIdx[2];
   Float64 m_CacheRefGirderOffset[2];
   pgsTypes::OffsetMeasurementType m_CacheRefGirderOffsetType[2];

   CGirderGroupData m_GirderGroupCache;

   void UpdateGirderCountHyperLinkText();
   void UpdateGirderTypeHyperLinkText();
   void UpdateGirderSpacingHyperLinkText();

   bool IsAbutment(pgsTypes::MemberEndType end);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SpanGirderLayoutPage_H__AE9D6B96_E89C_4D23_BD21_EFD39F5762FE__INCLUDED_)
