// SpecDesignFinalPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignFinalPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignFinalPropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignFinalPropertyPage, CMFCPropertyPage)

CSpecDesignFinalPropertyPage::CSpecDesignFinalPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_FINAL)
{
}

CSpecDesignFinalPropertyPage::~CSpecDesignFinalPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignFinalPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_A, OnCheckA)
   ON_BN_CLICKED(IDC_DESIGN_A, OnDesignA)
   ON_BN_CLICKED(IDC_LL_DEFLECTION, OnBnClickedLlDeflection)
   ON_BN_CLICKED(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE, OnBnClickedCheckBottomFlangeClearance)
   ON_BN_CLICKED(IDC_CHECK_INCLINDED_GIRDER, OnBnClickedCheckInclindedGirder)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignFinalPropertyPage message handlers

void CSpecDesignFinalPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignFinalData(pDX);
}


BOOL CSpecDesignFinalPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnCheckA();
   OnBnClickedLlDeflection();
   OnBnClickedCheckBottomFlangeClearance();
   OnBnClickedCheckInclindedGirder();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDesignFinalPropertyPage::OnCheckA()
{
   int list[] = { -1 };

   CheckDesignCtrl(IDC_CHECK_A, IDC_DESIGN_A, list, this);

   OnDesignA();
}

void CSpecDesignFinalPropertyPage::OnDesignA()
{
   int list[] = { IDC_A_ROUNDING_EDIT, IDC_A_ROUNDING_UNIT, -1 };
   CheckDesignCtrl(IDC_DESIGN_A, IDC_A_ROUNDING_CB, list, this);
}

void CSpecDesignFinalPropertyPage::OnBnClickedCheckBottomFlangeClearance()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_BOTTOM_FLANGE_CLEARANCE);
   GetDlgItem(IDC_CLEARANCE_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE)->EnableWindow(bEnable);
   GetDlgItem(IDC_CLEARANCE_UNIT)->EnableWindow(bEnable);
}

void CSpecDesignFinalPropertyPage::OnBnClickedLlDeflection()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_LL_DEFLECTION);
   GetDlgItem(IDC_LL_DEF_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_DEFLECTION_LIMIT)->EnableWindow(bEnable);
}

void CSpecDesignFinalPropertyPage::OnBnClickedCheckInclindedGirder()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_INCLINDED_GIRDER);
   GetDlgItem(IDC_INCLINDED_GIRDER_FS_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_INCLINDED_GIRDER_FS)->EnableWindow(bEnable);
}

void CSpecDesignFinalPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
