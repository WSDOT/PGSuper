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

// SpecDescrPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecDescrPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/ShearCapacityCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDescrPage property page

IMPLEMENT_DYNCREATE(CSpecDescrPage, CPropertyPage)

CSpecDescrPage::CSpecDescrPage() : CPropertyPage(CSpecDescrPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDescrPage)
	//}}AFX_DATA_INIT
}

CSpecDescrPage::~CSpecDescrPage()
{
}

void CSpecDescrPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecDescrPage)
	//}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_SPECIFICATION, m_cbSpecification);

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDescriptionData(pDX);

}


BEGIN_MESSAGE_MAP(CSpecDescrPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDescrPage)
	ON_WM_CANCELMODE()
    ON_CBN_SELCHANGE(IDC_SPECIFICATION,OnSpecificationChanged)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_USE_CURRENT_VERSION, &CSpecDescrPage::OnBnClickedUseCurrentVersion)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDescrPage message handlers
void CSpecDescrPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL );
}

BOOL CSpecDescrPage::OnInitDialog() 
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   for(auto e : pgsTypes::enum_range<WBFL::LRFD::BDSManager::Edition>(WBFL::LRFD::BDSManager::Edition::FirstEdition1994,WBFL::LRFD::BDSManager::GetLatestEdition()))
   {
      int idx = pSpec->AddString(WBFL::LRFD::BDSManager::GetEditionAsString(e));
      pSpec->SetItemData(idx,(DWORD)(e));
   }

   CSpecMainSheet* pParent = (CSpecMainSheet*)GetParent();
   WBFL::LRFD::BDSManager::Edition version = pParent->GetSpecVersion();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EFF_FLANGE_WIDTH);
   pCB->AddString(_T("in accordance with LRFD 4.6.2.6"));

   if ( version < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims )
   {
      pCB->AddString(_T("using the tributary width"));
   }


   CPropertyPage::OnInitDialog();

   // enable/disable si setting for before 2007
   OnBnClickedUseCurrentVersion();
   OnSpecificationChanged();

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDescrPage::OnCancelMode() 
{
	CPropertyPage::OnCancelMode();
}

WBFL::LRFD::BDSManager::Edition CSpecDescrPage::GetSpecVersion()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   return (WBFL::LRFD::BDSManager::Edition)(pSpec->GetItemData(idx));
}

void CSpecDescrPage::OnSpecificationChanged()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   DWORD_PTR id = pSpec->GetItemData(idx);

   BOOL enable_si = TRUE;
   if ((DWORD)WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims < id)
   {
      CheckRadioButton(IDC_SPEC_UNITS_SI,IDC_SPEC_UNITS_US,IDC_SPEC_UNITS_US);
      enable_si = FALSE;
   }

   CButton* pSi = (CButton*)GetDlgItem(IDC_SPEC_UNITS_SI);
   pSi->EnableWindow(enable_si);

   // Vci/Vcw method was removed from spec in 2017
   CSpecMainSheet* pParent = (CSpecMainSheet*)GetParent();
   WBFL::LRFD::BDSManager::Edition version = pParent->GetSpecVersion();

   if (WBFL::LRFD::BDSManager::Edition::EighthEdition2017 <= version)
   {
      auto shear_capacity_criteria = pParent->m_Entry.GetShearCapacityCriteria();
      if (shear_capacity_criteria.CapacityMethod == pgsTypes::scmVciVcw)
      {
         ::AfxMessageBox(_T("The Vci/Vcw method is currently selected for computing shear capacity, and this method was removed from the LRFD Bridge Design Specifications in the 8th Edition, 2017. The shear capacity method will be changed to compute in accordance with the General Method per LRFD 5.7.3.5.\nVisit the Shear Capacity tab for more options."), MB_OK|MB_ICONWARNING);
         shear_capacity_criteria.CapacityMethod = pgsTypes::scmBTEquations;
         pParent->m_Entry.SetShearCapacityCriteria(shear_capacity_criteria);
      }
   }
}

void CSpecDescrPage::OnBnClickedUseCurrentVersion()
{
   if (IsDlgButtonChecked(IDC_USE_CURRENT_VERSION) == BST_CHECKED)
   {
      // disable the dropdown list
      m_cbSpecification.EnableWindow(FALSE);

      // box is checked so change to the most current specification
      // it will be last one in the dropdown list
      m_cbSpecification.SetCurSel(m_cbSpecification.GetCount() - 1);
   }
   else
   {
      m_cbSpecification.EnableWindow(TRUE);
   }
   OnSpecificationChanged();
}
