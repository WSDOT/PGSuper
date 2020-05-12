// SpecShearCapacityPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecShearCapacityPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/SpecificationCriteria.h>

// CSpecShearCapacityPropertyPage

IMPLEMENT_DYNAMIC(CSpecShearCapacityPropertyPage, CMFCPropertyPage)

CSpecShearCapacityPropertyPage::CSpecShearCapacityPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_SHEAR_CAPACITY)
{
}

CSpecShearCapacityPropertyPage::~CSpecShearCapacityPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecShearCapacityPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecShearCapacityPropertyPage message handlers

void CSpecShearCapacityPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeShearCapacityData(pDX);
}


BOOL CSpecShearCapacityPropertyPage::OnInitDialog()
{
   FillShearMethodList();

   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecShearCapacityPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();
   
   FillShearMethodList();

   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= pDad->m_Entry.GetSpecificationCriteria().GetEdition())
   {
      GetDlgItem(IDC_SLWC_FR_LABEL)->SetWindowText(_T("Lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_HIDE);
   }
   else
   {
      GetDlgItem(IDC_SLWC_FR_LABEL)->SetWindowText(_T("Sand lightweight concrete"));
      GetDlgItem(IDC_ALWC_FR_LABEL)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_ALWC_FR_UNIT)->ShowWindow(SW_SHOW);
   }

   // 2017 crosswalk chapter 5 reorg
   GetDlgItem(IDC_LIMIT_STRAIN)->SetWindowText(CString(_T("Limit net longitudinal strain in the section at the centroid of the tension reinforcement (es) to positive values (LRFD ")) + pDad->LrfdCw8th(_T("5.8.3.4.2"), _T("5.7.3.4.2")) + _T(")"));

   return __super::OnSetActive();
}

void CSpecShearCapacityPropertyPage::FillShearMethodList()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SHEAR_CAPACITY);
   int cur_sel = pCB->GetCurSel();
   pgsTypes::ShearCapacityMethod shear_capacity_method = pgsTypes::scmBTEquations;

   if (cur_sel != CB_ERR)
      shear_capacity_method = (pgsTypes::ShearCapacityMethod)pCB->GetItemData(cur_sel);

   pCB->ResetContent();

   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();


   WBFL::LRFD::BDSManager::Edition version = pDad->GetSpecVersion();

   int idxGeneral, idxVciVcw, idxAppendixB, idxWSDOT2001, idxWSDOT2007;

   idxGeneral = pCB->AddString(CString(_T("Compute in accordance with LRFD ")) + CString(WBFL::LRFD::LrfdCw8th(_T("5.8.3.4.2"), _T("5.7.3.4.2"))) + CString(_T(" (General method)")));
   if (version <= WBFL::LRFD::BDSManager::Edition::FourthEdition2007)
      pCB->SetItemData(idxGeneral, (DWORD)pgsTypes::scmBTTables); // 4th Edition and earlier, general method is Beta-Theta tables
   else
      pCB->SetItemData(idxGeneral, (DWORD)pgsTypes::scmBTEquations); // After 4th Edition, general method is Beta-Theta equations

   if (WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= version && WBFL::LRFD::BDSManager::Edition::EighthEdition2017 > version)
   {
      idxVciVcw = pCB->AddString(_T("Compute in accordance with LRFD 5.8.3.4.3 (Vci and Vcw method)"));
      pCB->SetItemData(idxVciVcw, (DWORD)pgsTypes::scmVciVcw);
   }

   if (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= version)
   {
      idxAppendixB = pCB->AddString(_T("Compute in accordance with LRFD B5.1 (Beta-Theta Tables)"));
      pCB->SetItemData(idxAppendixB, (DWORD)pgsTypes::scmBTTables);
   }

   if (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= version)
   {
      idxWSDOT2001 = pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual (June 2001)"));
      pCB->SetItemData(idxWSDOT2001, (DWORD)pgsTypes::scmWSDOT2001);
   }

   if (WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= version && version < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims)
   {
      idxWSDOT2007 = pCB->AddString(_T("Compute in accordance with WSDOT Bridge Design Manual (August 2007)"));
      pCB->SetItemData(idxWSDOT2007, (DWORD)pgsTypes::scmWSDOT2007);
   }

   // select the right item
   switch (shear_capacity_method)
   {
   case pgsTypes::scmBTEquations:
      pCB->SetCurSel(idxGeneral); // general method
      break;

   case pgsTypes::scmVciVcw:
      if (WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pDad->GetSpecVersion())
         pCB->SetCurSel(idxVciVcw); // 4th and later, select Vci Vcw
      else
         pCB->SetCurSel(idxGeneral); // otherwise, select general
      break;

   case pgsTypes::scmBTTables:
      if (pDad->GetSpecVersion() <= WBFL::LRFD::BDSManager::Edition::FourthEdition2007)
         pCB->SetCurSel(idxGeneral); // 4th and earlier, tables is the general method
      else
         pCB->SetCurSel(idxAppendixB); // after 4th, this is Appendix B5.1
      break;

   case pgsTypes::scmWSDOT2001:
      if (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= pDad->GetSpecVersion())
      {
         pCB->SetCurSel(idxWSDOT2001); // 2nd + 2000 and later, use 2001 BDM
      }
      else
      {
         pCB->SetCurSel(idxGeneral); // before 2nd+2000, use general method
      }
      break;

   case pgsTypes::scmWSDOT2007:
      if (WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= pDad->GetSpecVersion())
      {
         pCB->SetCurSel(idxWSDOT2007); // 4th and later, 2007 method ok
      }
      else if (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= pDad->GetSpecVersion())
      {
         pCB->SetCurSel(idxWSDOT2001); // 2nd + 2000 and later (but not 4th and later), use 2001 BDM
      }
      else
      {
         pCB->SetCurSel(idxGeneral); // before 2nd+2000, use general method
      }
      break;
   }
}

void CSpecShearCapacityPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
