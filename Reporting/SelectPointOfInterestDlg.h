///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporting\SpanGirderReportSpecification.h>
#include "afxcmn.h"

// CSelectPointOfInterestDlg dialog

class CSelectPointOfInterestDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectPointOfInterestDlg)

public:
	CSelectPointOfInterestDlg(IBroker* pBroker,std::shared_ptr<CPointOfInterestReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,PoiAttributeType poiReferenceAttribute,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectPointOfInterestDlg();

// Dialog Data
	enum { IDD = IDD_POI_SELECT };

   pgsPointOfInterest GetPointOfInterest();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   IBroker* m_pBroker;
   std::shared_ptr<CPointOfInterestReportSpecification> m_pRptSpec;

   PoiAttributeType m_PoiReferenceAttribute;
   pgsPointOfInterest m_InitialPOI;
   CGirderKey m_GirderKey;

   PoiList m_vPOI;

   CSliderCtrl m_Slider;
   CStatic m_Label;
   int m_SliderPos;

   void UpdateSliderLabel();
   void UpdatePOI();

   void InitFromRptSpec();

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
