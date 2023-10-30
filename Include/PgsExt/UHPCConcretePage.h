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

#include <PgsExt\PgsExtExp.h>
#include <MFCTools\CacheEdit.h>

// UHPCConcretePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUHPCConcretePage dialog

class PGSEXTCLASS CUHPCConcretePage : public CPropertyPage
{
// Construction
public:
	CUHPCConcretePage();

// Dialog Data
	//{{AFX_DATA(CUHPCConcretePage)
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUHPCConcretePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	Float64 m_ftcri;
	Float64 m_ftcr;
	Float64 m_ftloc;
	Float64 m_etloc;
	Float64 m_alpha_u;
	Float64 m_ecu;
	Float64 m_gamma_u;
	Float64 m_FiberLength;
	bool m_bExperimental_ecu;

   // Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CUHPCConcretePage)
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnSetActive();
   afx_msg void OnHelp();
	afx_msg void On_ecu();
	//}}AFX_MSG

	CCacheEdit m_wnd_ecu;

	DECLARE_MESSAGE_MAP()
};
