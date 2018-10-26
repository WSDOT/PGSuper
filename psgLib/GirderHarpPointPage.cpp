///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// GirderHarpPointPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderHarpPointPage.h"
#include "GirderMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"


#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderHarpPointPage property page

IMPLEMENT_DYNCREATE(CGirderHarpPointPage, CPropertyPage)

CGirderHarpPointPage::CGirderHarpPointPage() : CPropertyPage(CGirderHarpPointPage::IDD,IDS_GIRDER_HARPPOINT)
{
	//{{AFX_DATA_INIT(CGirderHarpPointPage)
	//}}AFX_DATA_INIT
}

CGirderHarpPointPage::~CGirderHarpPointPage()
{
}

void CGirderHarpPointPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderHarpPointPage)
	//}}AFX_DATA_MAP

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeHarpPointData(pDX);
}

BOOL CGirderHarpPointPage::OnInitDialog()
{
   if ( !CPropertyPage::OnInitDialog() )
      return FALSE;

   OnMinimum();
   return TRUE;
}



BEGIN_MESSAGE_MAP(CGirderHarpPointPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderHarpPointPage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
   ON_BN_CLICKED(IDC_MINIMUM,OnMinimum)
   ON_CBN_SELCHANGE(IDC_L,OnSelectionChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderHarpPointPage message handlers
LRESULT CGirderHarpPointPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_HARPING_POINTS_TAB );
   return TRUE;
}


void CGirderHarpPointPage::OnFractional()
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   pDad->MiscOnFractional();
}

void CGirderHarpPointPage::OnAbsolute()
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   pDad->MiscOnAbsolute();
}

void CGirderHarpPointPage::OnMinimum()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_MINIMUM);
   GetDlgItem(IDC_MIN_HP)->EnableWindow(bEnable);
   GetDlgItem(IDC_MIN_HP_UNIT)->EnableWindow(bEnable);
}

void CGirderHarpPointPage::OnSelectionChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_L);

   switch( pCB->GetCurSel() )
   {
      case 0:      OnFractional();      break;
      case 1:      OnFractional();      break;
      case 2:      OnAbsolute();        break;
      default:     ATLASSERT(false); // should never get here
   }
}
