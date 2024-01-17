///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <WBFLCore.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <Reporting\MomentCapacityReportSpecification.h>

// CSelectMomentCapacitySectionDlg dialog

class CSelectMomentCapacitySectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectMomentCapacitySectionDlg)

public:
   CSelectMomentCapacitySectionDlg(IBroker* pBroker,std::shared_ptr<CMomentCapacityReportSpecification>& pRptSpec,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectMomentCapacitySectionDlg();

// Dialog Data
	enum { IDD = IDD_MOMENT_CAPACITY_SECTION_POI   };

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
   IBroker* m_pBroker;
   std::shared_ptr<CMomentCapacityReportSpecification> m_pRptSpec;

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
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnHelp();
};
