///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// SpecDeflectionsPage.cpp : implementation file
//

#include "stdafx.h"
#include "psglib\psglib.h"
#include "SpecDeflectionsPage.h"
#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDeflectionsPage property page

IMPLEMENT_DYNCREATE(CSpecDeflectionsPage, CPropertyPage)

CSpecDeflectionsPage::CSpecDeflectionsPage() : CPropertyPage(CSpecDeflectionsPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDeflectionsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecDeflectionsPage::~CSpecDeflectionsPage()
{
}

void CSpecDeflectionsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecBridgeSitePage)
	//}}AFX_DATA_MAP

   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDeflectionsData(pDX);
}


BEGIN_MESSAGE_MAP(CSpecDeflectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDeflectionsPage)
	ON_BN_CLICKED(IDC_LL_DEFLECTION, OnCheckLlDeflection)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDeflectionsPage message handlers

void CSpecDeflectionsPage::OnCheckLlDeflection() 
{
   DoCheckLlDeflection();
}

void CSpecDeflectionsPage::DoCheckLlDeflection()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_LL_DEFLECTION);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   CWnd* pwnd = GetDlgItem(IDC_LL_DEF_STATIC);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_DEFLECTION_LIMIT);
   ASSERT(pchk);
   pwnd->EnableWindow(ischk);
}

BOOL CSpecDeflectionsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
   DoCheckLlDeflection();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSpecDeflectionsPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DEFLECTIONS_TAB );
   return TRUE;
}
