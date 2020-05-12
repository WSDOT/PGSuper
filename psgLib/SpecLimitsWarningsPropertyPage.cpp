// SpecLimitsWarningsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecLimitsWarningsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/PrestressLossCriteria.h>

// CSpecLimitsWarningsPropertyPage

IMPLEMENT_DYNAMIC(CSpecLimitsWarningsPropertyPage, CMFCPropertyPage)

CSpecLimitsWarningsPropertyPage::CSpecLimitsWarningsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_LIMITS_WARNINGS)
{
}

CSpecLimitsWarningsPropertyPage::~CSpecLimitsWarningsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecLimitsWarningsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_GIRDER_SAG, OnBnClickedCheckGirderSag)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecLimitsWarningsPropertyPage message handlers

void CSpecLimitsWarningsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   CMFCPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeLimitsWarningsData(pDX);
}


BOOL CSpecLimitsWarningsPropertyPage::OnInitDialog()
{
   CComboBox* pcbSagOptions = (CComboBox*)GetDlgItem(IDC_SAG_OPTIONS);
   int idx = pcbSagOptions->AddString(_T("Upper bound camber"));
   pcbSagOptions->SetItemData(idx, (DWORD_PTR)pgsTypes::SagCamber::UpperBoundCamber);

   idx = pcbSagOptions->AddString(_T("Average camber"));
   pcbSagOptions->SetItemData(idx, (DWORD_PTR)pgsTypes::SagCamber::AverageCamber);

   idx = pcbSagOptions->AddString(_T("Lower bound camber"));
   pcbSagOptions->SetItemData(idx, (DWORD_PTR)pgsTypes::SagCamber::LowerBoundCamber);

   __super::OnInitDialog();

   OnBnClickedCheckGirderSag();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecLimitsWarningsPropertyPage::OnSetActive()
{
   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   // if this is third edition or earlier, enable the shipping loss controls
   if (pParent->m_Entry.GetPrestressLossCriteria().LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP)
   {
      GetDlgItem(IDC_SAG_OPTIONS_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SAG_OPTIONS)->ShowWindow(SW_HIDE);
   }

   return __super::OnSetActive();
}

void CSpecLimitsWarningsPropertyPage::OnBnClickedCheckGirderSag()
{
   // TODO: Add your control notification handler code here
   BOOL bEnable = (IsDlgButtonChecked(IDC_CHECK_GIRDER_SAG) == BST_CHECKED) ? TRUE : FALSE;
   GetDlgItem(IDC_SAG_OPTIONS)->EnableWindow(bEnable);
}

void CSpecLimitsWarningsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
