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

#include <PgsExt\PgsExtExp.h>


// CCEBFIPConcretePage dialog

class PGSEXTCLASS CCEBFIPConcretePage : public CPropertyPage
{
	DECLARE_DYNAMIC(CCEBFIPConcretePage)

public:
	CCEBFIPConcretePage();
	virtual ~CCEBFIPConcretePage();

// Dialog Data

   bool m_bUserParameters;
   Float64 m_S;
   Float64 m_BetaSc;
   pgsTypes::CEBFIPCementType m_CementType;

protected:
   bool m_bUseCEBFIPParameters;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;


	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnHelp();
   afx_msg void OnUserParameters();
   afx_msg void OnCementType();
   afx_msg void OnCompute();

   void UpdateParameters();
};