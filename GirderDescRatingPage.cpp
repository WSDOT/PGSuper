// GirderDescRatingPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"
#include "GirderDescRatingPage.h"
#include "GirderDescDlg.h"
#include <MFCTools\CustomDDX.h>
#include "HtmlHelp\HelpTopics.hh"

// CGirderDescRatingPage dialog

IMPLEMENT_DYNAMIC(CGirderDescRatingPage, CPropertyPage)

CGirderDescRatingPage::CGirderDescRatingPage()
	: CPropertyPage(CGirderDescRatingPage::IDD)
{
}

CGirderDescRatingPage::~CGirderDescRatingPage()
{
}

void CGirderDescRatingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, pParent->m_GirderData.Condition);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,      pParent->m_GirderData.ConditionFactor);
}


BEGIN_MESSAGE_MAP(CGirderDescRatingPage, CPropertyPage)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CGirderDescRatingPage::OnConditionFactorTypeChanged)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CGirderDescRatingPage message handlers

BOOL CGirderDescRatingPage::OnInitDialog()
{
   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString("Good or Satisfactory (Structure condition rating 6 or higher)");
   pcbConditionFactor->AddString("Fair (Structure condition rating of 5)");
   pcbConditionFactor->AddString("Poor (Structure condition rating 4 or lower)");
   pcbConditionFactor->AddString("Other");
   pcbConditionFactor->SetCurSel(0);

   CPropertyPage::OnInitDialog();

   OnConditionFactorTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescRatingPage::OnConditionFactorTypeChanged()
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_CONDITION_FACTOR);
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);

   int idx = pcbConditionFactor->GetCurSel();
   switch(idx)
   {
   case 0:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("1.00"));
      break;
   case 1:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.95"));
      break;
   case 2:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.85"));
      break;
   case 3:
      pEdit->EnableWindow(TRUE);
      break;
   }
}

void CGirderDescRatingPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_CONDITION );
}
