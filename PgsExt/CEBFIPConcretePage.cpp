///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// CEBFIPConcretePage.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\CEBFIPConcretePage.h>
#include <PgsExt\ConcreteDetailsDlg.h>
#include "CEBFIPParametersDlg.h"
#include "..\Documentation\PGSuper.hh"

#include <Materials/CEBFIPConcrete.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CCEBFIPConcretePage dialog

IMPLEMENT_DYNAMIC(CCEBFIPConcretePage, CPropertyPage)

CCEBFIPConcretePage::CCEBFIPConcretePage()
	: CPropertyPage()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   Construct(IDD_CEBFIP_CONCRETE);
}

CCEBFIPConcretePage::~CCEBFIPConcretePage()
{
}

void CCEBFIPConcretePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   DDX_CBItemData(pDX,IDC_CEMENT_TYPE,m_CementType);

   DDX_Check_Bool(pDX,IDC_USER,m_bUseCEBFIPParameters);
   DDX_CBItemData(pDX,IDC_CEMENT_TYPE,m_CementType);
   DDX_Text(pDX,IDC_S,m_S);
   DDX_Text(pDX,IDC_BETA_SC,m_BetaSc);

   if ( pDX->m_bSaveAndValidate )
   {
      m_bUserParameters = !m_bUseCEBFIPParameters;
   }
}


BEGIN_MESSAGE_MAP(CCEBFIPConcretePage, CPropertyPage)
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_USER, &CCEBFIPConcretePage::OnUserParameters)
   ON_CBN_SELCHANGE(IDC_CEMENT_TYPE, &CCEBFIPConcretePage::OnCementType)
   ON_BN_CLICKED(IDC_COMPUTE, &CCEBFIPConcretePage::OnCompute)
END_MESSAGE_MAP()


// CCEBFIPConcretePage message handlers

BOOL CCEBFIPConcretePage::OnInitDialog() 
{
   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   if ( !pParent->m_bEnableComputeTimeParamters )
   {
      GetDlgItem(IDC_COMPUTE)->ShowWindow(SW_HIDE);
   }

   CComboBox* pcbCementType = (CComboBox*)GetDlgItem(IDC_CEMENT_TYPE);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Rapid Hardening, High Strength Cemetn (RS)")),pgsTypes::RS);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Normal Hardening Cement (N)")),pgsTypes::N);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Rapid Hardening Cement (R)")),pgsTypes::R);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Slowly Hardening Cement (SL)")),pgsTypes::SL);

   m_bUseCEBFIPParameters = !m_bUserParameters;

	CPropertyPage::OnInitDialog();

   OnUserParameters();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCEBFIPConcretePage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_CEBFIP );
}

void CCEBFIPConcretePage::OnUserParameters()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_USER);
   GetDlgItem(IDC_CEMENT_TYPE)->EnableWindow(bEnable);

   GetDlgItem(IDC_S_LABEL)->EnableWindow(!bEnable);
   GetDlgItem(IDC_S)->EnableWindow(!bEnable);
   GetDlgItem(IDC_BETA_SC_LABEL)->EnableWindow(!bEnable);
   GetDlgItem(IDC_BETA_SC)->EnableWindow(!bEnable);
   GetDlgItem(IDC_COMPUTE)->EnableWindow(!bEnable);

   if ( bEnable )
   {
      UpdateParameters();
   }
}

void CCEBFIPConcretePage::OnCementType()
{
   BOOL bUseCEBFIP = IsDlgButtonChecked(IDC_USER);
   if ( bUseCEBFIP )
   {
      UpdateParameters();
   }
}

void CCEBFIPConcretePage::OnCompute()
{
   UpdateData(TRUE);

   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();

   INT_PTR result;
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      CCEBFIPParametersDlg dlg;
      dlg.m_t1  = pParent->m_TimeAtInitialStrength;
      dlg.m_fc1 = pParent->m_fci;
      dlg.m_fc2 = pParent->m_fc28;
      result = dlg.DoModal();

      if ( result == IDOK )
      {
         m_S = dlg.m_S;
         pParent->m_fci = dlg.m_fc1;
         pParent->m_fc28 = dlg.m_fc2;
      }
   }

   if ( result == IDOK )
   {
      UpdateData(FALSE);
   }
}

void CCEBFIPConcretePage::UpdateParameters()
{
   UpdateData(TRUE);

   WBFL::Materials::CEBFIPConcrete::GetModelParameters((WBFL::Materials::CEBFIPConcrete::CementType)m_CementType,&m_S,&m_BetaSc);

   UpdateData(FALSE);
}
