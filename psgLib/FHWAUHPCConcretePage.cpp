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

// FHWAUHPCConcretePage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include "FHWAUHPCConcretePage.h"
#include <MfcTools\CustomDDX.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFHWAUHPCConcretePage dialog


CFHWAUHPCConcretePage::CFHWAUHPCConcretePage() : CPropertyPage(IDD_FHWAUHPC_CONCRETE)
{
	//{{AFX_DATA_INIT(CFHWAUHPCConcretePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CFHWAUHPCConcretePage::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CPropertyPage::DoDataExchange(pDX);

   DDX_Control(pDX, IDC_ECU, m_wnd_ecu);

   //{{AFX_DATA_MAP(CFHWAUHPCConcretePage)
   //}}AFX_DATA_MAP

   DDX_UnitValueAndTag(pDX, IDC_FT_CRI, IDC_FT_CRI_UNIT, m_ftcri, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FT_CR, IDC_FT_CR_UNIT, m_ftcr, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FT_LOC, IDC_FT_LOC_UNIT, m_ftloc, pDisplayUnits->Stress);
   
   //if (!pDX->m_bSaveAndValidate)
   //   m_etloc *= 1.0e3;
   DDX_Text(pDX, IDC_ET_LOC, m_etloc);
   DDV_GreaterThanZero(pDX, IDC_ET_LOC, m_etloc);
   //if (pDX->m_bSaveAndValidate)
   //   m_etloc /= 1.0e3;

   DDX_Text(pDX, IDC_ALPHA_U, m_alpha_u);

   DDX_Check_Bool(pDX, IDC_ECU_CHECK, m_bExperimental_ecu);

   // Internally, e_cu is a negative value since it is compression. In the UI we want it to be
   // a positive value. Change the sign going into and out of the UI.
   if (!pDX->m_bSaveAndValidate)
      m_ecu *= -1;

   if (m_wnd_ecu.IsWindowEnabled()) DDX_Text(pDX, IDC_ECU, m_ecu);

   if (pDX->m_bSaveAndValidate)
      m_ecu *= -1;

   if (m_bExperimental_ecu) DDV_GreaterThanZero(pDX, IDC_ECU, m_ecu);

   DDX_UnitValueAndTag(pDX, IDC_FIBER, IDC_FIBER_UNIT, m_FiberLength, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero(pDX, IDC_FIBER, m_FiberLength, pDisplayUnits->ComponentDim);
}


BEGIN_MESSAGE_MAP(CFHWAUHPCConcretePage, CPropertyPage)
   //{{AFX_MSG_MAP(CFHWAUHPCConcretePage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_ECU_CHECK, On_ecu)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFHWAUHPCConcretePage message handlers
void CFHWAUHPCConcretePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_FHWAUHPC );
}

void CFHWAUHPCConcretePage::On_ecu()
{
   m_wnd_ecu.EnableWindow(IsDlgButtonChecked(IDC_ECU_CHECK) == BST_CHECKED);
}


inline BOOL CALLBACK EnableChildWindow(HWND hwnd, LPARAM lParam)
{
   ::EnableWindow(hwnd, (int)lParam);
   return TRUE;
}

BOOL CFHWAUHPCConcretePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

   On_ecu();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
