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

#include "resource.h"
#include "EditBearingDlg.h"

#pragma once

// CPierDetailsBearingsPage dialog

class CPierDetailsBearingsPage : public CPropertyPage
{
public:
	DECLARE_DYNAMIC(CPierDetailsBearingsPage)

public:
	CPierDetailsBearingsPage();
	virtual ~CPierDetailsBearingsPage();

   void Initialize(const CBridgeDescription2* pBridge,const CPierData2* pPier);

   void UpdateLocalData();

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

   CMetaFileStatic m_Bearing;

   // Data for Bearing input
   BearingInputData m_BearingInputData;

   CBearingData2 m_Bearings[2]; // bearing data for each line in dialog

   PierIndexType m_PierIdx;
   bool m_IsBoundaryPier;
   bool m_IsAbutment;
   bool m_bStoredSingleBL; // if true, forced single bearing line into line 1

   bool m_IsMsgFromMe;

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
