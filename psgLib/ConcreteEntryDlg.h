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

#pragma once

// ConcreteEntryDlg.h : header file
//

#include "ConcreteEntryGeneralPage.h"
#include "AASHTOConcretePage.h"
#include "PCIUHPCConcretePage.h"
#include "FHWAUHPCConcretePage.h"
#include "ACIConcretePage.h"
#include "CEBFIPConcretePage.h"

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog

class CConcreteEntryDlg : public CPropertySheet
{
// Construction
public:
	CConcreteEntryDlg(bool allowEditing,CWnd* pParent = nullptr,UINT iSelectPage =0);   // standard constructor
   ~CConcreteEntryDlg();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDlg)
	virtual BOOL OnInitDialog();
   //}}AFX_VIRTUAL

// Dialog Data
	//{{AFX_DATA(CConcreteEntryDlg)
	//}}AFX_DATA
   bool m_bAllowEditing;

   CConcreteEntryGeneralPage m_General;
   CAASHTOConcretePage m_AASHTO;
   CPCIUHPCConcretePage m_PCIUHPC;
   CFHWAUHPCConcretePage m_FHWAUHPC;
   CACIConcretePage m_ACI;
   CCEBFIPConcretePage m_CEBFIP;

protected:
   void Init();

   DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
