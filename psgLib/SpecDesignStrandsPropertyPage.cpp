// SpecDesignStrandsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDesignStrandsPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDesignStrandsPropertyPage

IMPLEMENT_DYNAMIC(CSpecDesignStrandsPropertyPage, CMFCPropertyPage)

CSpecDesignStrandsPropertyPage::CSpecDesignStrandsPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DESIGN_STRANDS)
{
}

CSpecDesignStrandsPropertyPage::~CSpecDesignStrandsPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDesignStrandsPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDesignStrandsPropertyPage message handlers

void CSpecDesignStrandsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDesignStrandsData(pDX);
}


BOOL CSpecDesignStrandsPropertyPage::OnInitDialog()
{
   __super::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDesignStrandsPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
