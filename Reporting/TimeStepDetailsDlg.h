///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include "resource.h"
#include <Reporting\TimeStepDetailsReportSpecification.h>

// CTimeStepDetailsDlg dialog

class CTimeStepDetailsDlg : public CDialog
{
	DECLARE_DYNAMIC(CTimeStepDetailsDlg)

public:
	CTimeStepDetailsDlg(IBroker* pBroker,std::shared_ptr<CTimeStepDetailsReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,IntervalIndexType intervalIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTimeStepDetailsDlg();

// Dialog Data
	enum { IDD = IDD_TIMESTEP_DETAILS };

   bool UseAllLocations();
   pgsPointOfInterest GetPOI();
   IntervalIndexType GetInterval();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   IBroker* m_pBroker;
   std::shared_ptr<CTimeStepDetailsReportSpecification> m_pRptSpec;

   pgsPointOfInterest m_InitialPOI;
   CGirderKey m_GirderKey;

   PoiList m_vPOI;
   IntervalIndexType m_IntervalIdx;
   bool m_bUseAllLocations;

   CSliderCtrl m_Slider;
   CStatic m_Location;
   int m_SliderPos;

   void UpdateSliderLabel();
   void UpdatePOI();

   void InitFromRptSpec();

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnClickedAllLocations();
   afx_msg void OnGirderLineChanged();
};
