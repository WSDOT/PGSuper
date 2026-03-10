///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// PCIUNPCConcretePage.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include "PGSuperUnits.h"
#include <PgsExt\ConcreteDetailsDlg.h>
#include <PgsExt\PCIUHPCConcretePage.h>
#include <System\Tokenizer.h>
#include "CopyConcreteEntry.h"
#include <LRFD\Lrfd.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\Bridge.h>

#include <PGSuperColors.h>
#include "..\Documentation\PGSuper.hh"


/////////////////////////////////////////////////////////////////////////////
// CPCIUHPCConcretePage dialog


CPCIUHPCConcretePage::CPCIUHPCConcretePage() : CPropertyPage()
{
	//{{AFX_DATA_INIT(CPCIUHPCConcretePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   Construct(IDD_PCIUHPC_CONCRETE);
}


void CPCIUHPCConcretePage::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CPropertyPage::DoDataExchange(pDX);

   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   bool bValidate = pParent->m_General.GetConcreteType() == pgsTypes::PCI_UHPC ? true : false;

   //{{AFX_DATA_MAP(CPCIUHPCConcretePage)
   //}}AFX_DATA_MAP
   DDX_UnitValueAndTag(pDX, IDC_FFC, IDC_FFC_UNIT, m_ffc, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FRR, IDC_FRR_UNIT, m_frr, pDisplayUnits->Stress);
   DDX_UnitValueAndTag(pDX, IDC_FIBER, IDC_FIBER_UNIT, m_FiberLength, pDisplayUnits->ComponentDim);

   if (!pDX->m_bSaveAndValidate)
      m_AutogenousShrinkage *= 1.0e3;

   DDX_Text(pDX, IDC_AUTOGENOUS_SHRINKAGE, m_AutogenousShrinkage);

   if (pDX->m_bSaveAndValidate)
      m_AutogenousShrinkage /= 1.0e3;

   DDX_Check_Bool(pDX, IDC_PCTT, m_bPCTT);

   if (bValidate)
   {
      // only validate if controls are enabled
      DDV_UnitValueGreaterThanZero(pDX, IDC_FFC, m_ffc, pDisplayUnits->Stress);
      DDV_UnitValueGreaterThanZero(pDX, IDC_FRR, m_frr, pDisplayUnits->Stress);
      DDV_UnitValueGreaterThanZero(pDX, IDC_FIBER, m_FiberLength, pDisplayUnits->ComponentDim);
      DDV_LimitOrMore(pDX, IDC_AUTOGENOUS_SHRINKAGE, m_AutogenousShrinkage,0.0);
   }
}


BEGIN_MESSAGE_MAP(CPCIUHPCConcretePage, CPropertyPage)
	//{{AFX_MSG_MAP(CPCIUHPCConcretePage)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCIUHPCConcretePage message handlers
void CPCIUHPCConcretePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_PCIUHPC);
}

BOOL CPCIUHPCConcretePage::OnSetActive()
{
   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   BOOL bEnable = pParent->m_General.GetConcreteType() == pgsTypes::PCI_UHPC ? TRUE : FALSE;
   GetDlgItem(IDC_FFC)->EnableWindow(bEnable);
   GetDlgItem(IDC_FRR)->EnableWindow(bEnable);
   GetDlgItem(IDC_FIBER)->EnableWindow(bEnable);
   GetDlgItem(IDC_AUTOGENOUS_SHRINKAGE)->EnableWindow(bEnable);
   GetDlgItem(IDC_PCTT)->EnableWindow(bEnable);

   return __super::OnSetActive();
}

BOOL CPCIUHPCConcretePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
