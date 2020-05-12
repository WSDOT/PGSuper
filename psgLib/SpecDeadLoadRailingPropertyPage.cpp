// SpecDeadLoadRailingPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpecDeadLoadRailingPropertyPage.h"
#include "SpecPropertySheet.h"
#include <EAF\EAFDocument.h>

// CSpecDeadLoadRailingPropertyPage

IMPLEMENT_DYNAMIC(CSpecDeadLoadRailingPropertyPage, CMFCPropertyPage)

CSpecDeadLoadRailingPropertyPage::CSpecDeadLoadRailingPropertyPage() :
   CMFCPropertyPage(IDD_SPEC_DEAD_LOAD_RAILING)
{
}

CSpecDeadLoadRailingPropertyPage::~CSpecDeadLoadRailingPropertyPage()
{
}


BEGIN_MESSAGE_MAP(CSpecDeadLoadRailingPropertyPage, CMFCPropertyPage)
   ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



// CSpecDeadLoadRailingPropertyPage message handlers

void CSpecDeadLoadRailingPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSpecDescrPage)
   //}}AFX_DATA_MAP
   DDX_Control(pDX, IDC_DIST_TRAFFIC_BARRIER_SPIN, m_TrafficSpin);

   CSpecPropertySheet* pParent = (CSpecPropertySheet*)GetParent();
   pParent->ExchangeDeadLoadRailingData(pDX);
}


BOOL CSpecDeadLoadRailingPropertyPage::OnInitDialog()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DIST_TRAFFIC_BARRIER_BASIS);
   int index = pCB->AddString(_T("nearest girders"));
   pCB->SetItemData(index, pgsTypes::tbdGirder);
   index = pCB->AddString(_T("nearest mating surfaces"));
   pCB->SetItemData(index, pgsTypes::tbdMatingSurface);
   index = pCB->AddString(_T("nearest webs"));
   pCB->SetItemData(index, pgsTypes::tbdWebs);

   __super::OnInitDialog();

   m_TrafficSpin.SetRange(0, 100);

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDeadLoadRailingPropertyPage::OnHelp()
{
#pragma Reminder("WORKING HERE - Dialogs - Need to update help")
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_GENERAL);
}
