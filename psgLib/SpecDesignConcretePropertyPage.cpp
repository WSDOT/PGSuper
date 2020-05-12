// SpecDesignConcretePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignConcretePropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignConcretePropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignConcretePropertyPage, CMFCPropertyPage)

CSpecDesignConcretePropertyPage::CSpecDesignConcretePropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_CONCRETE)
{
}

CSpecDesignConcretePropertyPage::~CSpecDesignConcretePropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignConcretePropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_FC1, OnFcTypeChanged)
   ON_BN_CLICKED(IDC_FC2, OnFcTypeChanged)
   ON_BN_CLICKED(IDC_USE_90_DAY_STRENGTH, OnBnClicked90DayStrength)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignConcretePropertyPage message handlers

void CSpecDesignConcretePropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignConcreteData(pDX);
}


BOOL CSpecDesignConcretePropertyPage::OnInitDialog()
{
   __super::OnInitDialog();
   OnBnClicked90DayStrength();
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDesignConcretePropertyPage::OnFcTypeChanged()
{
   OnBnClicked90DayStrength();
}

void CSpecDesignConcretePropertyPage::OnBnClicked90DayStrength()
{
   BOOL bCorrectFcSetting = IsDlgButtonChecked(IDC_FC2);
   BOOL bIsUsed = IsDlgButtonChecked(IDC_USE_90_DAY_STRENGTH);
   BOOL bEnable = (bIsUsed && bCorrectFcSetting);

   GetDlgItem(IDC_USE_90_DAY_STRENGTH)->EnableWindow(bCorrectFcSetting);
   GetDlgItem(IDC_90_DAY_STRENGTH_FACTOR)->EnableWindow(bEnable);
   GetDlgItem(IDC_90_DAY_STRENGTH_LABEL)->EnableWindow(bEnable);
}

void CSpecDesignConcretePropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
