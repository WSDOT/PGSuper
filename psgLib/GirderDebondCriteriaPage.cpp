///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// GirderDebondCriteriaPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "GirderDebondCriteriaPage.h"
#include "GirderMainSheet.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDebondCriteriaPage property page

IMPLEMENT_DYNCREATE(CGirderDebondCriteriaPage, CPropertyPage)

CGirderDebondCriteriaPage::CGirderDebondCriteriaPage() : CPropertyPage(CGirderDebondCriteriaPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDebondCriteriaPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDebondCriteriaPage::~CGirderDebondCriteriaPage()
{
}

void CGirderDebondCriteriaPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDebondCriteriaPage)
	//}}AFX_DATA_MAP

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeFlexuralCriteriaData(pDX);

   if ( !pDX->m_bSaveAndValidate )
   {
      UpdateCheckBoxes();
   }
}


BEGIN_MESSAGE_MAP(CGirderDebondCriteriaPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDebondCriteriaPage)
	ON_BN_CLICKED(ID_HELP,OnHelp)
	ON_BN_CLICKED(IDC_CHECK_MAX_LENGTH_FRACTION, OnCheckMaxLengthFraction)
	ON_BN_CLICKED(IDC_CHECK_MAX_LENGTH, OnCheckMaxLength)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_STRAIGHT_DESIGN_CHECK, &CGirderDebondCriteriaPage::OnBnClickedStraightDesignCheck)
   ON_BN_CLICKED(IDC_DEBOND_DESIGN_CHECK, &CGirderDebondCriteriaPage::OnBnClickedDebondDesignCheck)
   ON_BN_CLICKED(IDC_HARPED_DESIGN_CHECK, &CGirderDebondCriteriaPage::OnBnClickedHarpedDesignCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDebondCriteriaPage message handlers

BOOL CGirderDebondCriteriaPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();

   CGirderMainSheet* pParent = (CGirderMainSheet*)GetParent();
   if ( pParent->IsSplicedGirder() )
   {
      // Design strategies don't apply to spliced girders.. Hide the UI elements
      GetDlgItem(IDC_DEFAULT_DISTANCE_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEFAULT_DISTANCE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEFAULT_DISTANCE_UNIT)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_DESIGN_STRATEGY_GROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DESIGN_STRATEGY_NOTE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FCI_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FC_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_STRAIGHT_DESIGN_CHECK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRAIGHT_FCI)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRAIGHT_FC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRAIGHT_UNITS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRAIGHT_RAISE_CHECK)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_DEBOND_DESIGN_CHECK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEBOND_FCI)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEBOND_FC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEBOND_UNITS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DEBOND_RAISE_CHECK)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_HARPED_DESIGN_CHECK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HARPED_FCI)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HARPED_FC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_HARPED_UNITS)->ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDebondCriteriaPage::OnHelp()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_FLEXURAL_DESIGN );
}

void CGirderDebondCriteriaPage::UpdateCheckBoxes()
{
   UpdateDebondCheckBoxes();
   UpdateDesignCheckBoxes();
}

void CGirderDebondCriteriaPage::UpdateDebondCheckBoxes()
{
   CButton* pbox = (CButton*)GetDlgItem(IDC_CHECK_MAX_LENGTH_FRACTION);
   ASSERT(pbox);
   bool is_check = pbox->GetCheck()==0 ? FALSE:TRUE;

   CWnd* pedit = GetDlgItem(IDC_MAX_LENGTH_FRACTION);
   ASSERT(pedit);
   pedit->EnableWindow(is_check);

   pbox = (CButton*)GetDlgItem(IDC_CHECK_MAX_LENGTH);
   ASSERT(pbox);
   is_check = pbox->GetCheck()==0 ? FALSE:TRUE;

   pedit = GetDlgItem(IDC_MAX_LENGTH);
   ASSERT(pedit);
   pedit->EnableWindow(is_check);
}

void CGirderDebondCriteriaPage::UpdateDesignCheckBoxes()
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   BOOL can_straight = pDad->CanDoAllStraightDesign() ? TRUE:FALSE;
   BOOL can_harp     = pDad->CanHarpStrands() ? TRUE:FALSE;
   BOOL can_debond   = pDad->CanDebondStrands() ? TRUE:FALSE;

   int straightCtrls[] = {IDC_STRAIGHT_DESIGN_CHECK,IDC_STRAIGHT_FCI,IDC_STRAIGHT_FC,IDC_STRAIGHT_UNITS,IDC_STRAIGHT_RAISE_CHECK, NULL};
   int debondCtrls[]   = {IDC_DEBOND_DESIGN_CHECK,IDC_DEBOND_FCI,IDC_DEBOND_FC,IDC_DEBOND_UNITS,IDC_DEBOND_RAISE_CHECK, NULL};
   int harpedCtrls[]   = {IDC_HARPED_DESIGN_CHECK,IDC_HARPED_FCI,IDC_HARPED_FC,IDC_HARPED_UNITS, NULL};

   // All straight design if possible
   EnableCtrls(straightCtrls, can_straight);
   if (can_straight)
   {
      CButton* pbox = (CButton*)GetDlgItem(IDC_STRAIGHT_DESIGN_CHECK);
      bool is_check = pbox->GetCheck()!=0 ? TRUE:FALSE;
      EnableCtrls(straightCtrls+1, is_check);
   }

   // enable debond and harp controls only if strands are available
   EnableCtrls(debondCtrls, can_debond);
   if (can_debond)
   {
      CButton* pbox = (CButton*)GetDlgItem(IDC_DEBOND_DESIGN_CHECK);
      bool is_check = pbox->GetCheck()!=0 ? TRUE:FALSE;

      EnableCtrls(debondCtrls+1, is_check);
   }

   EnableCtrls(harpedCtrls, can_harp);
   if (can_harp)
   {
      CButton* pbox = (CButton*)GetDlgItem(IDC_HARPED_DESIGN_CHECK);
      bool is_check = pbox->GetCheck()!=0 ? TRUE:FALSE;

      EnableCtrls(harpedCtrls+1, is_check);
   }
}

void CGirderDebondCriteriaPage::EnableCtrls(int* ctrlIDs, BOOL enable)
{
   while(*ctrlIDs!=NULL)
   {
      CWnd* pedit = GetDlgItem(*ctrlIDs);
      pedit->EnableWindow(enable);

      ctrlIDs++;
   }
}

void CGirderDebondCriteriaPage::OnCheckMaxLengthFraction() 
{
	UpdateCheckBoxes();
}

void CGirderDebondCriteriaPage::OnCheckMaxLength() 
{
	UpdateCheckBoxes();
}

void CGirderDebondCriteriaPage::OnBnClickedStraightDesignCheck()
{
   UpdateDesignCheckBoxes();
}

void CGirderDebondCriteriaPage::OnBnClickedDebondDesignCheck()
{
   UpdateDesignCheckBoxes();
}

void CGirderDebondCriteriaPage::OnBnClickedHarpedDesignCheck()
{
   UpdateDesignCheckBoxes();
}
