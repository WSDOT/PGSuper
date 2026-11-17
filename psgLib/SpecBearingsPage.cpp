///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// SpecBearingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PsgLib\PsgLib.h"
#include "SpecBearingsPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>


/////////////////////////////////////////////////////////////////////////////
// CSpecBearingsPage property page

IMPLEMENT_DYNCREATE(CSpecBearingsPage, CPropertyPage)

CSpecBearingsPage::CSpecBearingsPage() : CPropertyPage(CSpecBearingsPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecBearingsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecBearingsPage::~CSpecBearingsPage()
{
}

void CSpecBearingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecBridgeSitePage)
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeBearingsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecBearingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecBearingsPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_BEARING_METHOD_A, OnBearingMethodChanged)
   ON_BN_CLICKED(IDC_BEARING_METHOD_B, OnBearingMethodChanged)
   ON_BN_CLICKED(IDC_ABSOLUTE_STRAIN_SUM, OnAbsoluteStrainSumChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecBearingsPage message handlers

void CSpecBearingsPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_BEARINGS );
}

BOOL CSpecBearingsPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();
   OnBearingMethodChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecBearingsPage::OnBearingMethodChanged()
{
   GetDlgItem(IDC_ABSOLUTE_STRAIN_SUM)->EnableWindow(IsDlgButtonChecked(IDC_BEARING_METHOD_B) == BST_CHECKED);
}

void CSpecBearingsPage::OnAbsoluteStrainSumChanged()
{
   if (IsDlgButtonChecked(IDC_ABSOLUTE_STRAIN_SUM) == BST_UNCHECKED)
   {
      ::AfxMessageBox(_T("Using signed strain contributions can produce artificially low combined strains when opposite-sign dead- and live-load rotations cancel after applying the 1.75 cyclic amplification factor. ")
         _T("AASHTO LRFD Commentary C14.7.5.3.3 advises using the sum of absolute values when the directions of the loading components in the critical combination are unclear. ")
         _T("Verify that the signed combination represents the governing case."), MB_OK | MB_ICONWARNING);
   }
}

BOOL CSpecBearingsPage::OnSetActive()
{
   return CPropertyPage::OnSetActive();
}
