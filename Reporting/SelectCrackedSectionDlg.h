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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#pragma once
#include "resource.h"
#include "afxwin.h"
#include "afxcmn.h"

#include <PgsExt\ReportPointOfInterest.h>
#include <Reporting\CrackedSectionReportSpecification.h>

// CSelectCrackedSectionDlg dialog

class CSelectCrackedSectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectCrackedSectionDlg)

public:
	CSelectCrackedSectionDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<CCrackedSectionReportSpecification>& pRptSpec,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectCrackedSectionDlg();

// Dialog Data
	enum { IDD = IDD_CRACKED_SECTION_POI };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;

   pgsPointOfInterest m_InitialPOI;

   CGirderKey m_GirderKey;
   int m_MomentType;

   pgsPointOfInterest GetPOI();

private:
   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;
   std::shared_ptr<CCrackedSectionReportSpecification> m_pRptSpec;

   PoiList m_vPOI;

   CComboBox m_cbGroup;
   CComboBox m_cbGirder;
   CSliderCtrl m_Slider;
   CStatic m_Label;
   int m_SliderPos;

   void UpdateGirderComboBox();
   void UpdateSliderLabel();
   void UpdatePOI();

   void InitFromRptSpec();

public:
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnSpanChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnHelp();
};
