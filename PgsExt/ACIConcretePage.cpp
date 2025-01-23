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

// ACIConcretePage.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\ACIConcretePage.h>
#include <PgsExt\ConcreteDetailsDlg.h>
#include "ACIParametersDlg.h"
#include "..\Documentation\PGSuper.hh"

#include <Materials/ACI209Concrete.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CACIConcretePage dialog

IMPLEMENT_DYNAMIC(CACIConcretePage, CPropertyPage)

CACIConcretePage::CACIConcretePage()
	: CPropertyPage()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   Construct(IDD_ACI_CONCRETE);
}

CACIConcretePage::~CACIConcretePage()
{
}

void CACIConcretePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX,IDC_USER,m_bUseACIParameters);
   DDX_CBItemData(pDX,IDC_CURE_METHOD,m_CureMethod);
   DDX_CBItemData(pDX,IDC_CEMENT_TYPE,m_CementType);
   DDX_UnitValueAndTag(pDX,IDC_ALPHA,IDC_ALPHA_UNIT,m_A,pDisplayUnits->Time3);
   DDX_Text(pDX,IDC_BETA,m_B);

   if ( pDX->m_bSaveAndValidate )
   {
      m_bUserParameters = !m_bUseACIParameters;
   }
}


BEGIN_MESSAGE_MAP(CACIConcretePage, CPropertyPage)
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_USER, &CACIConcretePage::OnUserParameters)
   ON_CBN_SELCHANGE(IDC_CURE_METHOD, &CACIConcretePage::OnCureMethod)
   ON_CBN_SELCHANGE(IDC_CEMENT_TYPE, &CACIConcretePage::OnCementType)
   ON_BN_CLICKED(IDC_COMPUTE, &CACIConcretePage::OnCompute)
END_MESSAGE_MAP()


// CACIConcretePage message handlers

BOOL CACIConcretePage::OnInitDialog() 
{
   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   if ( !pParent->m_bEnableComputeTimeParamters )
   {
      GetDlgItem(IDC_COMPUTE)->ShowWindow(SW_HIDE);
   }

   CComboBox* pcbCureMethod = (CComboBox*)GetDlgItem(IDC_CURE_METHOD);
   pcbCureMethod->SetItemData(pcbCureMethod->AddString(_T("Moist")),pgsTypes::Moist);
   pcbCureMethod->SetItemData(pcbCureMethod->AddString(_T("Steam")),pgsTypes::Steam);

   CComboBox* pcbCementType = (CComboBox*)GetDlgItem(IDC_CEMENT_TYPE);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Type I")),pgsTypes::TypeI);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Type III")),pgsTypes::TypeIII);

   m_bUseACIParameters = !m_bUserParameters;

	CPropertyPage::OnInitDialog();

   OnUserParameters();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CACIConcretePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_ACI );
}

void CACIConcretePage::OnUserParameters()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_USER);
   //GetDlgItem(IDC_CURE_METHOD)->EnableWindow(bEnable); // always want this to be enabled
   GetDlgItem(IDC_CEMENT_TYPE)->EnableWindow(bEnable);

   GetDlgItem(IDC_ALPHA_LABEL)->EnableWindow(!bEnable);
   GetDlgItem(IDC_ALPHA)->EnableWindow(!bEnable);
   GetDlgItem(IDC_ALPHA_UNIT)->EnableWindow(!bEnable);
   GetDlgItem(IDC_BETA_LABEL)->EnableWindow(!bEnable);
   GetDlgItem(IDC_BETA)->EnableWindow(!bEnable);
   GetDlgItem(IDC_COMPUTE)->EnableWindow(!bEnable);

   if ( bEnable )
   {
      UpdateParameters();
   }
}

void CACIConcretePage::OnCureMethod()
{
   BOOL bUseACI = IsDlgButtonChecked(IDC_USER);
   if ( bUseACI )
   {
      UpdateParameters();
   }
}

void CACIConcretePage::OnCementType()
{
   BOOL bUseACI = IsDlgButtonChecked(IDC_USER);
   if ( bUseACI )
   {
      UpdateParameters();
   }
}

void CACIConcretePage::OnCompute()
{
   UpdateData(TRUE);

   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();

   INT_PTR result;
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      CACIParametersDlg dlg;
      dlg.m_t1  = pParent->m_TimeAtInitialStrength;
      dlg.m_fc1 = pParent->m_fci;
      dlg.m_fc2 = pParent->m_fc28;
      result = dlg.DoModal();

      if ( result == IDOK )
      {
         m_A = dlg.m_A;
         m_B = dlg.m_B;
         pParent->m_fci = dlg.m_fc1;
         pParent->m_fc28 = dlg.m_fc2;
      }
   }

   if ( result == IDOK )
   {
      UpdateData(FALSE);
   }
}

void CACIConcretePage::UpdateParameters()
{
   UpdateData(TRUE);

   WBFL::Materials::ACI209Concrete::GetModelParameters((WBFL::Materials::CuringType)m_CureMethod,
                                         (WBFL::Materials::CementType)m_CementType,
                                         &m_A,&m_B);

   UpdateData(FALSE);
}
