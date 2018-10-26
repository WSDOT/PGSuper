///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// GirderSegmentSpacingPage.h : header file
//

#include "resource.h"
#include "SegmentSpacingGrid.h"
#include "SameGirderSpacingHyperLink.h"
#include <PsgLib\ConnectionLibraryEntry.h>
#include <PgsExt\BridgeDescription2.h>


/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentSpacingPage dialog

class CGirderSegmentSpacingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderSegmentSpacingPage)

// Construction
public:
	CGirderSegmentSpacingPage();
	~CGirderSegmentSpacingPage();

// Dialog Data
	//{{AFX_DATA(CGirderSegmentSpacingPage)
	enum { IDD = IDD_SEGMENT_SPACING };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

   CSameGirderSpacingHyperLink   m_GirderSpacingHyperLink;
   CComboBox                     m_cbGirderSpacingMeasurement;

   Uint32 m_GirderSpacingMeasure; // this is a hash of measurement location and measurement type


   CSegmentSpacingGrid m_SpacingGrid;

   GirderIndexType m_RefGirderIdx;
   Float64 m_RefGirderOffset;
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;
   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   pgsTypes::MeasurementLocation m_GirderSpacingMeasurementLocation;

   void Init(const CPierData2* pPierData);
   void Init(const CTemporarySupportData& tsData);

   bool AllowConnectionChange(pgsTypes::PierFaceType side, const CString& conectionName);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderSegmentSpacingPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void CommonInit(const CGirderSpacing2* pSpacing,const CBridgeDescription2* pBridge);

	// Generated message map functions
	//{{AFX_MSG(CGirderSegmentSpacingPage)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnSpacingDatumChanged();
   afx_msg void OnHelp();
	//}}AFX_MSG
   afx_msg LRESULT OnChangeSameGirderSpacing(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   void ToggleGirderSpacingType();

   bool m_bIsPier;
   CString m_strSupportLabel;

   CGirderSpacingData2 m_SpacingCache;
   long m_GirderSpacingMeasureCache;

   void OnPierSpacingDatumChanged(UINT nIDC,pgsTypes::PierFaceType pierFace);

   void FillGirderSpacingMeasurementComboBox();

   void FillRefGirderOffsetTypeComboBox();
   void FillRefGirderComboBox();

   void UpdateGirderSpacingHyperLinkText();
   
   void DisableAll();
   void UpdateLinkedNote();
   void UpdateChildWindowState();
   void UpdateGirderSpacingState();

public:
   const CBridgeDescription2* GetBridgeDescription();
   bool IsContinuousSegment();
   Float64 GetStation();
   LPCTSTR GetOrientation();
   void SetGroupTitle();
};
