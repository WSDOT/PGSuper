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

#include "resource.h"
#include "EditBearingDlg.h"

#pragma once

// CSpanGdrDetailsBearingsPage dialog

class CSpanGdrDetailsBearingsPage : public CPropertyPage
{
public:
	DECLARE_DYNAMIC(CSpanGdrDetailsBearingsPage)

public:
	CSpanGdrDetailsBearingsPage();
	virtual ~CSpanGdrDetailsBearingsPage();

   // Works for a single girder or an entire span if gdrIdx==INVALID_IDX
   void Initialize(const CBridgeDescription2* pBridge,const CPierData2* pBackPier,const CPierData2* pAheadPier, GirderIndexType gdrIdx);

// Dialog Data
   

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PIER_BEARINGS_PAGE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   void ShowCtrls();
   void SaveData();
   void UpdateLocalData();

   CMetaFileStatic m_Bearing;

   PierIndexType m_StartPierIdx;
   PierIndexType m_EndPierIdx;
   GirderIndexType m_GdrIdx;

   pgsTypes::BearingType m_MyBearingType; // this dialog works for both single girders and spans. This tells us which one

   bool m_IsMsgFromMe;

public:
   // Data for Bearing input
   BearingInputData m_BearingInputData;
   std::array<CBearingData2, 2> m_Bearings; // local cache of bearing data at start(0) and end(1) of span or girder (use pgsTypes::MemberEndType to access array)

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedEditBearings();
   afx_msg void OnCbnSelchangeBrgType();
   afx_msg void OnCbnSelchangeBrgShape1();
   afx_msg void OnCbnSelchangeBrgCount1();
   afx_msg void OnCbnSelchangeBrgShape2();
   afx_msg void OnCbnSelchangeBrgCount2();
   afx_msg void OnHelp();
};
