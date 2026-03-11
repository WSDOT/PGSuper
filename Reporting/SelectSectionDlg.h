///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
#include "PoiReportSpecification.h"
#include "afxcmn.h"
#include <Reporting/SectionPropertiesReportSpecification.h>

// CSelectSectionDlg dialog

class CSelectSectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectSectionDlg)

public:
	CSelectSectionDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<CSectionPropertiesReportSpecification>& pRptSpec,const pgsPointOfInterest& initialPoi,CWnd* pParent = nullptr);   // standard constructor
	CSelectSectionDlg(std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<CSectionPropertiesReportSpecification>& pRptSpec, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSelectSectionDlg();

// Dialog Data
	enum { IDD = IDD_POI_SELECT };

   pgsPointOfInterest GetPointOfInterest();

   pgsPointOfInterest m_InitialPOI;
   IntervalIndexType m_IntervalIdx;
   CGirderKey m_GirderKey;


protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	std::shared_ptr<WBFL::EAF::Broker> m_pBroker;
	std::shared_ptr<CSectionPropertiesReportSpecification> m_pRptSpec;

public:
	virtual BOOL OnInitDialog() override;

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
	void FillIntervalCtrl();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnGroupChanged();
	afx_msg void OnGirderChanged();


};