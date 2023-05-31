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

#include <PgsExt\PrecastSegmentData.h>
#include "DrawPrecastSegmentControl.h"
#include <MfcTools\WideDropDownComboBox.h>

// CGirderSegmentGeneralPage dialog

class CGirderSegmentGeneralPage : public CPropertyPage, public IPrecastSegmentDataSource
{
	DECLARE_DYNAMIC(CGirderSegmentGeneralPage)

public:
	CGirderSegmentGeneralPage();
	virtual ~CGirderSegmentGeneralPage();

// Dialog Data
	enum { IDD = IDD_SEGMENT_GENERAL };
   CWideDropDownComboBox m_cbConstruction;
   CWideDropDownComboBox m_cbErection;
   CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;
   CDrawPrecastSegmentControl m_ctrlDrawSegment;

   std::array<CCacheEdit, 4> m_ctrlSectionLength;
   std::array<CCacheEdit, 4> m_ctrlSectionHeight;
   std::array<CCacheEdit, 4> m_ctrlBottomFlangeDepth;

   Float64 GetBottomFlangeDepth(pgsTypes::SegmentZoneType segZone);
   Float64 GetHeight(pgsTypes::SegmentZoneType segZone);
   Float64 GetLength(pgsTypes::SegmentZoneType segZone);
   Float64 GetSegmentLength();
   pgsTypes::SegmentVariationType GetSegmentVariation();

   // IPrecastSegmentDataSource
   virtual const CSplicedGirderData* GetGirder() const;
   virtual const CSegmentKey& GetSegmentKey() const;
   virtual SegmentIDType GetSegmentID() const;

   bool m_bWasEventCreated;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeFc();
   afx_msg void OnChangeEc();
   afx_msg void OnChangeEci();
	afx_msg void OnMoreConcreteProperties();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnVariationTypeChanged();
   afx_msg void OnConstructionEventChanged();
   afx_msg void OnConstructionEventChanging();
   afx_msg void OnErectionEventChanged();
   afx_msg void OnErectionEventChanging();

   void UpdateConcreteControls(bool bSkipEcCheckBoxes = false);
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void ExchangeConcreteData(CDataExchange* pDX);

   void FillVariationTypeComboBox();
   void GetSectionVariationControlState(BOOL* pbEnable);
   void GetSectionVariationControlState(pgsTypes::SegmentVariationType variationType,BOOL* pbEnable);
   void UpdateSegmentVariationParameters(pgsTypes::SegmentVariationType variationType);

   void UpdateFc();
   void UpdateFci();
   void UpdateEci();
   void UpdateEc();

   CString m_strUserEc;
   CString m_strUserEci;

   Float64 GetValue(UINT nIDC,const WBFL::Units::LengthData& lengthUnit);

   void FillEventList();
   EventIDType CreateEvent();
	
   int m_PrevConstructionEventIdx;
   int m_PrevErectionEventIdx; // capture the erection stage when the combo box drops down so we can restore the value if CreateEvent fails
   DECLARE_MESSAGE_MAP()

   int m_LossMethod;
   int m_TimeDependentModel;
   Float64 m_AgeAtRelease;

   CEdit m_ctrlStartHaunch;
   CEdit m_ctrlEndHaunch;

   void InitBottomFlangeDepthControls();
   void InitEndBlockControls();

public:
   afx_msg void OnSegmentChanged();
   afx_msg void OnConcreteStrength();
   afx_msg void OnBnClickedBottomFlangeDepth();
   afx_msg void OnHelp();
   virtual BOOL OnSetActive();

   void UpdateHaunchAndCamberControls();
   void UpdateHaunchAndCamberData(CDataExchange* pDX);
   void EnableHaunchAndCamberControls(BOOL bStartControls,BOOL bEndControls, bool bShowBoth);

};
