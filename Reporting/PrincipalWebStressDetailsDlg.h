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

#include "resource.h"
#include <Reporting\PrincipalWebStressDetailsReportSpecification.h>
#include "afxcmn.h"

// CPrincipalWebStressDetailsDlg dialog

class CPrincipalWebStressDetailsDlg : public CDialog
{
	DECLARE_DYNAMIC(CPrincipalWebStressDetailsDlg)

public:
	CPrincipalWebStressDetailsDlg(IBroker* pBroker,std::shared_ptr<CPrincipalWebStressDetailsReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,
                                 IntervalIndexType intervalIdx, bool bReportAxial, bool bReportShear, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPrincipalWebStressDetailsDlg();

// Dialog Data
	enum { IDD = IDD_PRINCIPAL_WEBSTRESS_DETAILS };

   bool UseAllLocations();
   pgsPointOfInterest GetPOI();
   IntervalIndexType GetInterval();
   bool GetReportShear();
   bool GetReportAxial();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   IBroker* m_pBroker;
   std::shared_ptr<CPrincipalWebStressDetailsReportSpecification> m_pPwsRptSpec;

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

   void InitFromPrincipalWebStressRptSpec();

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnClickedAllLocations();
   afx_msg void OnGirderLineChanged();
   BOOL m_bReportShearStress;
   BOOL m_bReportAxial;
};
