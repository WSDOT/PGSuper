///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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


// CCEBFIPConcretePage dialog

class CCEBFIPConcretePage : public CPropertyPage
{
	DECLARE_DYNAMIC(CCEBFIPConcretePage)

public:
	CCEBFIPConcretePage();
	virtual ~CCEBFIPConcretePage();

// Dialog Data
   bool m_bUserParameters;
   pgsTypes::CEBFIPCementType m_CementType;
   Float64 m_S;
   Float64 m_BetaSc;

protected:
   bool m_bUseCEBFIPParameters;

   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

   void UpdateParameters();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnHelp();
   afx_msg void OnCbnSelchangeCementType();
   afx_msg void OnBnClickedUser();
};
