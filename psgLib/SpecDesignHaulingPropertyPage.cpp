// SpecDesignHaulingPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignHaulingPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignHaulingPropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignHaulingPropertyPage, CMFCPropertyPage)

CSpecDesignHaulingPropertyPage::CSpecDesignHaulingPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_HAULING)
{
}

CSpecDesignHaulingPropertyPage::~CSpecDesignHaulingPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignHaulingPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_HAULING, OnCheckHauling)
   ON_BN_CLICKED(IDC_IS_SUPPORT_LESS_THAN, OnBnClickedIsSupportLessThan)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignHaulingPropertyPage message handlers

void CSpecDesignHaulingPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignHaulingData(pDX);
}


BOOL CSpecDesignHaulingPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   OnCheckHauling();
   OnBnClickedIsSupportLessThan();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDesignHaulingPropertyPage::OnCheckHauling()
{
   int list[] = { IDC_MIN_TRUCK_SUPPORT_LABEL,IDC_MIN_TRUCK_SUPPORT,IDC_MIN_TRUCK_SUPPORT_UNIT,IDC_STATIC_H,
      IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_LABEL,IDC_TRUCK_SUPPORT_LOCATION_ACCURACY,
      IDC_TRUCK_SUPPORT_LOCATION_ACCURACY_UNIT,IDC_IS_SUPPORT_LESS_THAN,IDC_SUPPORT_LESS_THAN,
      IDC_SUPPORT_LESS_THAN_UNIT,IDC_STATIC_K,-1 };

   CheckDesignCtrl(IDC_CHECK_HAULING, IDC_DESIGN_HAULING, list, this);
}


void CSpecDesignHaulingPropertyPage::OnBnClickedIsSupportLessThan()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_IS_SUPPORT_LESS_THAN);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);

   pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN_UNIT);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}

void CSpecDesignHaulingPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
