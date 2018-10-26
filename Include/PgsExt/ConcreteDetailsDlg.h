///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "ConcreteGeneralPage.h"
#include "AASHTOConcretePage.h"
#include "ACIConcretePage.h"
#include "CEBFIPConcretePage.h"

// ConcreteDetailsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg dialog

class PGSEXTCLASS CConcreteDetailsDlg : public CPropertySheet
{
// Construction
public:
	CConcreteDetailsDlg(bool bFinalProperties,bool bEnableComputeTimeParameters = true,bool bEnableCopyFromLibrary = true,CWnd* pParent = NULL,UINT iSelectPage=0);

   // text strings to in in display units... Ec comes out in display units
   static CString UpdateEc(const CString& strFc,const CString& strDensity,const CString& strK1,const CString& strK2);

// Dialog Data
	//{{AFX_DATA(CConcreteDetailsDlg)
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConcreteDetailsDlg)
	//}}AFX_VIRTUAL
   bool m_bFinalProperties; // if true, properties are based on f'c28 otherwise they are based on f'ci
   bool m_bEnableComputeTimeParamters; // if true, interface will be provided that allows the user to input f'ci and f'c and
   // time parameters to fit the f'c(t) function through f'ci and f'c will be computed
   bool m_bEnableCopyFromLibrary; // if true, the Copy From Library button is displayed in the UI

   // Common to all property pages
   Float64 m_TimeAtInitialStrength; // time when initial strength (fci) is reached
   Float64 m_fci; // initial strength (used if m_bFinalProperties is false)
   Float64 m_fc28; // 28 day strength (used if m_bFinalProperties is true)
   Float64 m_Eci; // modulus based on initial strength
   Float64 m_Ec28; // modulus based on 28 day strength
   bool m_bUserEci;
   bool m_bUserEc28;

   CConcreteGeneralPage m_General;
   CAASHTOConcretePage m_AASHTO;
   CACIConcretePage m_ACI;
   CCEBFIPConcretePage m_CEBFIP;

   // Implementation
protected:
   void Init();

	// Generated message map functions
	//{{AFX_MSG(CConcreteDetailsDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
