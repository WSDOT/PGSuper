///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

// CTxDOTOptionalDesignBridgeInputPage dialog

class CTxDOTOptionalDesignBridgeInputPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignBridgeInputPage)

public:
	CTxDOTOptionalDesignBridgeInputPage();
	virtual ~CTxDOTOptionalDesignBridgeInputPage();

// Dialog Data
	enum { IDD = IDD_BRIDGE_INPUT_PAGE };

   CString m_Bridge;
   CString m_BridgeID;
   CString m_JobNumber;
   CString m_Engineer;
   CString m_Company;
   CString m_Comments;

   CString m_SpanNo;
   CString m_BeamNo;
   CString m_BeamType;
   Float64 m_BeamSpacing;
   Float64 m_SpanLength;
   Float64 m_SlabThickness;
   Float64 m_RelativeHumidity;
   Float64 m_LldfMoment;
   Float64 m_LldfShear;

   Float64 m_EcSlab;
   Float64 m_EcBeam;
   Float64 m_FcSlab;

   Float64 m_Ft;
   Float64 m_Fb;
   Float64 m_Mu;

   Float64 m_WNonCompDc;
   Float64 m_WCompDc;
   Float64 m_WCompDw;

   CString m_SelectedProjectCriteriaLibrary;

   // Store a pointer to our data source
   CTxDOTOptionalDesignData* m_pData;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

private:
   void LoadDialogData();
   void SaveDialogData();
   void LoadGirderNames();
   bool CheckLibraryData();
   void LoadProjectCriteriaLibraryNames();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnHelpFinder();
   afx_msg void OnCbnSelchangeProjectCriteria();
};
