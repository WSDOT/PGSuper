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

class CBridgeDescrBearings : public CPropertyPage
{
	DECLARE_DYNAMIC(CBridgeDescrBearings)

public:
	CBridgeDescrBearings();
	virtual ~CBridgeDescrBearings();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BRIDGEDESC_BEARING };
#endif

   // Data for Bearing input
   BearingInputData m_BearingInputData;
   CMetaFileStatic m_Bearing;

   bool m_IsMsgFromMe;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedEditBearings();
   afx_msg void OnCbnSelchangeBrgType();
   afx_msg void OnCbnSelchangeBrgShape();
   afx_msg void OnCbnSelchangeBrgCount();
   afx_msg void OnHelp();
};
