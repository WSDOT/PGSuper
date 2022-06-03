///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\LoadFactors.h>

// CLoadFactorsDlg dialog

class CLoadFactorsDlg : public CDialog
{
	DECLARE_DYNAMIC(CLoadFactorsDlg)

public:
	CLoadFactorsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLoadFactorsDlg();

// Dialog Data
	enum { IDD = IDD_LOAD_FACTORS };

   CLoadFactors m_LoadFactors;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   static constexpr int nLimitStates = ((int)pgsTypes::FatigueI) + 1;
   Float64 DCmin[nLimitStates];   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   Float64 DWmin[nLimitStates];
   Float64 CRmin[nLimitStates];
   Float64 SHmin[nLimitStates];
   Float64 REmin[nLimitStates];
   Float64 PSmin[nLimitStates];
   Float64 LLIMmin[nLimitStates];
   Float64 DCmax[nLimitStates];
   Float64 DWmax[nLimitStates];
   Float64 CRmax[nLimitStates];
   Float64 SHmax[nLimitStates];
   Float64 REmax[nLimitStates];
   Float64 PSmax[nLimitStates];
   Float64 LLIMmax[nLimitStates];

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
};
