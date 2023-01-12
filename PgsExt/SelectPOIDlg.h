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
#pragma once

#include <PgsExt\PointOfInterest.h>

// CSelectPOIDlg dialog

class CSelectPOIDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectPOIDlg)

public:
	CSelectPOIDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectPOIDlg();

// Dialog Data
	enum { IDD = IDD_SELECT_POI };

   pgsPointOfInterest m_InitialPOI;
   IntervalIndexType m_IntervalIdx;
   CGirderKey m_GirderKey;

   pgsPointOfInterest GetPOI();

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog() override;

   PoiList m_vPOI;

   CComboBox m_cbGroup;
   CComboBox m_cbGirder;
   CSliderCtrl m_Slider;
   CStatic m_Label;
   int m_SliderPos;

   void UpdateGirderComboBox(GroupIndexType groupIdx);
   void UpdateSliderLabel();
   void UpdatePOI();
   void FillIntervalCtrl();

public:
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
};
