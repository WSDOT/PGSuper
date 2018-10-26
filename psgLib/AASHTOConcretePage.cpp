///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// ConcreteEntryGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include "ConcreteEntryGeneralPage.h"
#include <MfcTools\CustomDDX.h>

#include <EAF\EAFApp.h>

#include "..\htmlhelp\helptopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAASHTOConcretePage dialog


CAASHTOConcretePage::CAASHTOConcretePage(): CPropertyPage(IDD_AASHTO_CONCRETE)
{
	//{{AFX_DATA_INIT(CAASHTOConcretePage)
	//}}AFX_DATA_INIT
}


void CAASHTOConcretePage::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CPropertyPage::DoDataExchange(pDX);

   //{{AFX_DATA_MAP(CAASHTOConcretePage)
   //}}AFX_DATA_MAP
   DDX_Check_Bool(pDX, IDC_HAS_AGG_STRENGTH, m_bHasFct );
   DDX_UnitValueAndTag(pDX, IDC_AGG_STRENGTH, IDC_AGG_STRENGTH_T, m_Fct, pDisplayUnits->Stress );
   if ( m_bHasFct || !pDX->m_bSaveAndValidate )
   {
      if ( !pDX->m_bSaveAndValidate )
      {
         CWnd* pWnd = GetDlgItem(IDC_AGG_STRENGTH);
         pWnd->GetWindowText(m_strFct);
      }
   }

   DDX_Text(pDX, IDC_EC_K1, m_EccK1 );
   DDV_GreaterThanZero(pDX,IDC_EC_K1,m_EccK1);

   DDX_Text(pDX, IDC_EC_K2, m_EccK2 );
   DDV_GreaterThanZero(pDX,IDC_EC_K2,m_EccK2);

   DDX_Text(pDX, IDC_CREEP_K1, m_CreepK1 );
   DDV_GreaterThanZero(pDX,IDC_CREEP_K1,m_CreepK1);

   DDX_Text(pDX, IDC_CREEP_K2, m_CreepK2 );
   DDV_GreaterThanZero(pDX,IDC_CREEP_K2,m_CreepK2);

   DDX_Text(pDX, IDC_SHRINKAGE_K1, m_ShrinkageK1 );
   DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K1,m_ShrinkageK1);

   DDX_Text(pDX, IDC_SHRINKAGE_K2, m_ShrinkageK2 );
   DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K2,m_ShrinkageK2);
}


BEGIN_MESSAGE_MAP(CAASHTOConcretePage, CPropertyPage)
	//{{AFX_MSG_MAP(CAASHTOConcretePage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
   ON_BN_CLICKED(IDC_HAS_AGG_STRENGTH,OnAggSplittingStrengthClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAASHTOConcretePage message handlers
LRESULT CAASHTOConcretePage::OnCommandHelp(WPARAM, LPARAM lParam)
{
#pragma Reminder("UPDATE: Update help file reference for this topic")
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONCRETE_ENTRY_DIALOG );
   return TRUE;
}


BOOL CAASHTOConcretePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAASHTOConcretePage::OnSetActive()
{
   CPropertyPage::OnSetActive();

   CConcreteEntryDlg* pParent = (CConcreteEntryDlg*)GetParent();
   BOOL bEnable = (pParent->m_General.m_Type == pgsTypes::Normal ? FALSE : TRUE);
   GetDlgItem(IDC_HAS_AGG_STRENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_AGG_STRENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_AGG_STRENGTH_T)->EnableWindow(bEnable);

   if ( bEnable )
      OnAggSplittingStrengthClicked();

   return TRUE;
}

void CAASHTOConcretePage::OnAggSplittingStrengthClicked()
{
   CButton* pButton = (CButton*)GetDlgItem(IDC_HAS_AGG_STRENGTH);
   ASSERT(pButton);
   BOOL bIsChecked = pButton->GetCheck();
   GetDlgItem(IDC_AGG_STRENGTH)->EnableWindow(bIsChecked);
   GetDlgItem(IDC_AGG_STRENGTH_T)->EnableWindow(bIsChecked);
}
