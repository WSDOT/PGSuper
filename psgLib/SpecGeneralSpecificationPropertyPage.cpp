// SpecGeneralSpecificationPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecGeneralSpecificationPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/ShearCapacityCriteria.h>

// CSpecGeneralSpecificationPropertyPage

IMPLEMENT_DYNAMIC(CSpecGeneralSpecificationPropertyPage, CMFCPropertyPage)

CSpecGeneralSpecificationPropertyPage::CSpecGeneralSpecificationPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_GENERAL_SPECIFICATION)
{
}

CSpecGeneralSpecificationPropertyPage::~CSpecGeneralSpecificationPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecGeneralSpecificationPropertyPage, CMFCPropertyPage)
   ON_CBN_SELCHANGE(IDC_SPECIFICATION, OnSpecificationChanged)
   ON_BN_CLICKED(IDC_USE_CURRENT_VERSION, OnBnClickedUseCurrentVersion)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecGeneralSpecificationPropertyPage message handlers

void CSpecGeneralSpecificationPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   DDX_Control(pDX, IDC_SPECIFICATION, m_cbSpecification);

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeGeneralSpecificationData(pDX);
}


BOOL CSpecGeneralSpecificationPropertyPage::OnInitDialog()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx;
   for (int i = 1; i < (int)WBFL::LRFD::BDSManager::Edition::LastEdition; i++)
   {
      idx = pSpec->AddString(WBFL::LRFD::BDSManager::GetEditionAsString((WBFL::LRFD::BDSManager::Edition)(i)));
      pSpec->SetItemData(idx, (DWORD)(i));
   }

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   WBFL::LRFD::BDSManager::Edition version = pParent->GetSpecVersion();

   __super::OnInitDialog();

   // enable/disable si setting for before 2007
   OnSpecificationChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

WBFL::LRFD::BDSManager::Edition CSpecGeneralSpecificationPropertyPage::GetSpecVersion()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   return (WBFL::LRFD::BDSManager::Edition)(pSpec->GetItemData(idx));
}

void CSpecGeneralSpecificationPropertyPage::OnSpecificationChanged()
{
   CComboBox* pSpec = (CComboBox*)GetDlgItem(IDC_SPECIFICATION);
   int idx = pSpec->GetCurSel();
   DWORD_PTR id = pSpec->GetItemData(idx);

   BOOL enable_si = TRUE;
   if (id > (DWORD)WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims)
   {
      CheckRadioButton(IDC_SPEC_UNITS_SI, IDC_SPEC_UNITS_US, IDC_SPEC_UNITS_US);
      enable_si = FALSE;
   }

   CButton* pSi = (CButton*)GetDlgItem(IDC_SPEC_UNITS_SI);
   pSi->EnableWindow(enable_si);

   // Vci/Vcw method was removed from spec in 2017
   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   WBFL::LRFD::BDSManager::Edition version = pParent->GetSpecVersion();

   if (WBFL::LRFD::BDSManager::Edition::EighthEdition2017 <= version)
   {
      auto shear_capacity_criteria = pParent->m_Entry.GetShearCapacityCriteria();
      if (shear_capacity_criteria.CapacityMethod == pgsTypes::scmVciVcw)
      {
         shear_capacity_criteria.CapacityMethod = pgsTypes::scmBTEquations;
         pParent->m_Entry.SetShearCapacityCriteria(shear_capacity_criteria);
      }
   }
}

void CSpecGeneralSpecificationPropertyPage::OnBnClickedUseCurrentVersion()
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

void CSpecGeneralSpecificationPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
