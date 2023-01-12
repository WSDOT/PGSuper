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

#include <MfcTools\WideDropDownComboBox.h>

// CClosureJointGeneralPage dialog

class CClosureJointGeneralPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CClosureJointGeneralPage)

public:
	CClosureJointGeneralPage();
	virtual ~CClosureJointGeneralPage();

// Dialog Data
	enum { IDD = IDD_CLOSURE_GENERAL };
   CWideDropDownComboBox m_cbEvent;
   CEdit	   m_ctrlEc;
	CEdit	   m_ctrlEci;
	CButton	m_ctrlEcCheck;
	CButton	m_ctrlEciCheck;
	CEdit	   m_ctrlFc;
	CEdit  	m_ctrlFci;

   bool m_bWasEventCreated;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void OnMoreConcreteProperties();
   afx_msg void OnUserEci();
   afx_msg void OnUserEc();
	afx_msg void OnChangeFci();
	afx_msg void OnChangeFc();
	afx_msg void OnChangeEci();
	afx_msg void OnChangeEc();
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHelp();
   afx_msg void OnEventChanged();
   afx_msg void OnEventChanging();
   afx_msg void OnConcreteStrength();

   void ExchangeConcreteData(CDataExchange* pDX);


	DECLARE_MESSAGE_MAP()

   void UpdateConcreteControls(bool bSkipEcCheckBoxes = false);
   void UpdateConcreteParametersToolTip();
   CString m_strTip;

   void FillGirderComboBox();
   void FillEventList();
   EventIndexType CreateEvent();

   int m_PrevEventIdx;

   void UpdateEci();
   void UpdateEc();
   void UpdateFci();
   void UpdateFc();

   CString m_strUserEc;
   CString m_strUserEci;

   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::SlabOffsetType m_SlabOffsetTypeCache;
   Float64 m_SlabOffset;
   CString m_strSlabOffsetCache;

   int m_LossMethod;
   int m_TimeDependentModel;
   Float64 m_AgeAtContinuity;

public:
   virtual BOOL OnInitDialog();
};
