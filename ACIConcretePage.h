///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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


// CACIConcretePage dialog

class CACIConcretePage : public CPropertyPage
{
	DECLARE_DYNAMIC(CACIConcretePage)

public:
	CACIConcretePage();
	virtual ~CACIConcretePage();

// Dialog Data
	enum { IDD = IDD_ACI_CONCRETE };

   bool m_bUserParameters;
   Float64 m_A;
   Float64 m_B;
   pgsTypes::CureMethod m_CureMethod;
   pgsTypes::CementType m_CementType;

protected:
   bool m_bUseACIParameters;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
   afx_msg void OnUserParameters();
   afx_msg void OnCureMethod();
   afx_msg void OnCementType();

   void UpdateParameters();

	DECLARE_MESSAGE_MAP()
};