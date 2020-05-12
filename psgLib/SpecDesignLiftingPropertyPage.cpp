// SpecDesignLiftingPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignLiftingPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignLiftingPropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignLiftingPropertyPage, CMFCPropertyPage)

CSpecDesignLiftingPropertyPage::CSpecDesignLiftingPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_LIFTING)
{
}

CSpecDesignLiftingPropertyPage::~CSpecDesignLiftingPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignLiftingPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_LIFTING, OnCheckLifting)
   ON_BN_CLICKED(IDC_CHECK_HANDLING_WEIGHT, OnCheckHandlingWeight)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignLiftingPropertyPage message handlers

void CSpecDesignLiftingPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignLiftingData(pDX);
}


BOOL CSpecDesignLiftingPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();
   
   OnCheckLifting();
   OnCheckHandlingWeight();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDesignLiftingPropertyPage::OnCheckLifting()
{
   int list[] = { IDC_MIN_LIFTING_POINT_LABEL,IDC_MIN_LIFTING_POINT,IDC_MIN_LIFTING_POINT_UNIT,IDC_LIFTING_POINT_LOCATION_ACCURACY_LABEL,IDC_STATIC_L,IDC_LIFTING_POINT_LOCATION_ACCURACY,IDC_LIFTING_POINT_LOCATION_ACCURACY_UNIT,-1 };

   CheckDesignCtrl(IDC_CHECK_LIFTING, IDC_DESIGN_LIFTING, list, this);
}

void CSpecDesignLiftingPropertyPage::OnCheckHandlingWeight()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_HANDLING_WEIGHT);
   GetDlgItem(IDC_HANDLING_WEIGHT)->EnableWindow(bEnable);
   GetDlgItem(IDC_HANDLING_WEIGHT_UNIT)->EnableWindow(bEnable);
}

void CSpecDesignLiftingPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
