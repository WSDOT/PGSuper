///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// SpecLoadFactors.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecLoadFactors.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecLoadFactors property page

IMPLEMENT_DYNCREATE(CSpecLoadFactors, CPropertyPage)

CSpecLoadFactors::CSpecLoadFactors() : CPropertyPage(CSpecLoadFactors::IDD)
{
	//{{AFX_DATA_INIT(CSpecLoadFactors)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecLoadFactors::~CSpecLoadFactors()
{
}

void CSpecLoadFactors::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecLoadFactors)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLoadFactorData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecLoadFactors, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecLoadFactors)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecLoadFactors message handlers
LRESULT CSpecLoadFactors::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_LOADFACTORS );
   return TRUE;
}

BOOL CSpecLoadFactors::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   if ( pDad->GetSpecVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      // disable Fatigue I if before 2009
      GetDlgItem(IDC_FATIGUE_I_DC)->EnableWindow(FALSE);
      GetDlgItem(IDC_FATIGUE_I_DW)->EnableWindow(FALSE);
      GetDlgItem(IDC_FATIGUE_I_LLIM)->EnableWindow(FALSE);
   }
   else
   {
      // disable Service IA if 2009 or later
      GetDlgItem(IDC_SERVICE_IA_DC)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_IA_DW)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_IA_LLIM)->EnableWindow(FALSE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
