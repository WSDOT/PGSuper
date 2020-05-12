// SpecDesignReleasePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignReleasePropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignReleasePropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignReleasePropertyPage, CMFCPropertyPage)

CSpecDesignReleasePropertyPage::CSpecDesignReleasePropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_RELEASE)
{
}

CSpecDesignReleasePropertyPage::~CSpecDesignReleasePropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignReleasePropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_HD, OnCheckHoldDownForce)
   ON_BN_CLICKED(IDC_CHECK_SLOPE, OnCheckSlope)
   ON_BN_CLICKED(IDC_CHECK_SPLITTING, OnCheckSplitting)
   ON_BN_CLICKED(IDC_CHECK_CONFINEMENT, OnCheckConfinement)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignReleasePropertyPage message handlers

void CSpecDesignReleasePropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignReleaseData(pDX);
}


BOOL CSpecDesignReleasePropertyPage::OnInitDialog()
{
   CEAFApp* pApp = EAFGetApp();
   const auto* pDisplayUnits = pApp->GetDisplayUnits();

   // set statics for strand slope
   CString sl05, sl06, sl07;
   if (pApp->GetUnitsMode() == eafTypes::umSI)
   {
      VERIFY(sl05.LoadString(IDS_SLOPE_O5_SI));
      VERIFY(sl06.LoadString(IDS_SLOPE_O6_SI));
      VERIFY(sl07.LoadString(IDS_SLOPE_O7_SI));
   }
   else
   {
      VERIFY(sl05.LoadString(IDS_SLOPE_O5_US));
      VERIFY(sl06.LoadString(IDS_SLOPE_O6_US));
      VERIFY(sl07.LoadString(IDS_SLOPE_O7_US));
   }

   CComboBox* pcbHDFT = (CComboBox*)GetDlgItem(IDC_HOLD_DOWN_FORCE_TYPE);
   pcbHDFT->AddString(_T("Total Hold Down Force"));
   pcbHDFT->AddString(_T("Hold Down Force per Strand"));

   __super::OnInitDialog();
   
   OnCheckHoldDownForce();
   OnCheckSlope();
   OnCheckSplitting();
   OnCheckConfinement();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSpecDesignReleasePropertyPage::OnSetActive()
{
   CSpecPropertySheet* pDad = (CSpecPropertySheet*)GetParent();

   // deal with 2017 crosswalk
   CWnd* pWnd = GetDlgItem(IDC_SSPLITTING);
   pWnd->SetWindowText(CString(_T("Splitting Resistance (")) + pDad->LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) + _T(")"));

   pWnd = GetDlgItem(IDC_SCONFINEMENT);
   pWnd->SetWindowText(CString(_T("Confinement Reinforcement (")) + pDad->LrfdCw8th(_T("5.10.10.2"), _T("5.9.4.4.2")) + _T(")"));

   return CMFCPropertyPage::OnSetActive();
}

void CSpecDesignReleasePropertyPage::OnCheckHoldDownForce()
{
   int list[] = { IDC_HOLD_DOWN_FORCE_TYPE,IDC_HOLD_DOWN_FORCE,IDC_HOLD_DOWN_FORCE_UNITS,IDC_FRICTION_LABEL,IDC_FRICTION,IDC_FRICTION_UNIT,-1 };

   CheckDesignCtrl(IDC_CHECK_HD, IDC_DESIGN_HD, list, this);
}

void CSpecDesignReleasePropertyPage::OnCheckSlope()
{
   int list[] = { IDC_STATIC_SLOPE_05,IDC_STRAND_SLOPE_05,IDC_STATIC_SLOPE_06,IDC_STRAND_SLOPE_06,IDC_STATIC_SLOPE_07,
      IDC_STRAND_SLOPE_07,-1 };
   CheckDesignCtrl(IDC_CHECK_SLOPE, IDC_DESIGN_SLOPE, list, this);
}

void CSpecDesignReleasePropertyPage::OnCheckSplitting()
{
   int list[] = { IDC_STATIC_SPL,IDC_STATIC_SH,IDC_N,-1 };
   CheckDesignCtrl(IDC_CHECK_SPLITTING, IDC_DESIGN_SPLITTING, list, this);
}

void CSpecDesignReleasePropertyPage::OnCheckConfinement()
{
   int list[] = { -1 };

   CheckDesignCtrl(IDC_CHECK_CONFINEMENT, IDC_DESIGN_CONFINEMENT, list, this);
}

void CSpecDesignReleasePropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}

