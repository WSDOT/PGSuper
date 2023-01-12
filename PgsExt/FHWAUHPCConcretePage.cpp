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

// FHWAUNPCConcretePage.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include "PGSuperUnits.h"
#include <PgsExt\ConcreteDetailsDlg.h>
#include <PgsExt\FHWAUHPCConcretePage.h>
#include <System\Tokenizer.h>
#include "CopyConcreteEntry.h"
#include <Lrfd\Lrfd.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\Bridge.h>

#include <PGSuperColors.h>
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFHWAUHPCConcretePage dialog


CFHWAUHPCConcretePage::CFHWAUHPCConcretePage() : CPropertyPage()
{
	//{{AFX_DATA_INIT(CFHWAUHPCConcretePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   Construct(IDD_FHWAUHPC_CONCRETE);
}


void CFHWAUHPCConcretePage::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CPropertyPage::DoDataExchange(pDX);

   //{{AFX_DATA_MAP(CFHWAUHPCConcretePage)
   //}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_ECU, m_wnd_ecu);

   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   bool bValidate = pParent->m_General.GetConcreteType() == pgsTypes::FHWA_UHPC ? true : false;

   DDX_UnitValueAndTag(pDX, IDC_FT_CRI, IDC_FT_CRI_UNIT, m_ftcri, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FT_CR, IDC_FT_CR_UNIT, m_ftcr, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FT_LOC, IDC_FT_LOC_UNIT, m_ftloc, pDisplayUnits->Stress);

   //if (!pDX->m_bSaveAndValidate)
   //   m_etloc *= 1.0e3;
   DDX_Text(pDX, IDC_ET_LOC, m_etloc);

   DDX_Text(pDX, IDC_ALPHA_U, m_alpha_u);
   DDX_Check_Bool(pDX, IDC_ECU_CHECK, m_bExperimental_ecu);

   // Internally, e_cu is a negative value since it is compression. In the UI we want it to be
   // a positive value. Change the sign going into and out of the UI.
   if (!pDX->m_bSaveAndValidate)
      m_ecu *= -1;

   if(m_wnd_ecu.IsWindowEnabled()) DDX_Text(pDX, IDC_ECU, m_ecu);

   if (pDX->m_bSaveAndValidate)
      m_ecu *= -1;

   DDX_UnitValueAndTag(pDX, IDC_FIBER, IDC_FIBER_UNIT, m_FiberLength, pDisplayUnits->ComponentDim);

   if (bValidate)
   {
      // only validate if controls are enabled
      DDV_UnitValueGreaterThanZero(pDX, IDC_FT_CRI, m_ftcri, pDisplayUnits->Stress);
      DDV_UnitValueGreaterThanZero(pDX, IDC_FT_CR, m_ftcr, pDisplayUnits->Stress);
      DDV_UnitValueGreaterThanZero(pDX, IDC_FT_LOC, m_ftloc, pDisplayUnits->Stress);
      DDV_GreaterThanZero(pDX, IDC_ET_LOC, m_etloc);

      if (m_bExperimental_ecu) DDV_GreaterThanZero(pDX, IDC_ECU, m_ecu);

      DDV_UnitValueGreaterThanZero(pDX, IDC_FIBER, m_FiberLength, pDisplayUnits->ComponentDim);
   }

   //if (pDX->m_bSaveAndValidate)
   //   m_etloc /= 1.0e3;
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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_FHWAUHPC);
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

BOOL CFHWAUHPCConcretePage::OnSetActive()
{
   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   BOOL bEnable = pParent->m_General.GetConcreteType() == pgsTypes::FHWA_UHPC ? TRUE : FALSE;
   EnumChildWindows(GetSafeHwnd(), EnableChildWindow, bEnable);
   
   On_ecu();

   return __super::OnSetActive();
}

BOOL CFHWAUHPCConcretePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
