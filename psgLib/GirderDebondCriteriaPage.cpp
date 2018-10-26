///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "..\htmlhelp\HelpTopics.hh"

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
   pDad->ExchangeDebondCriteriaData(pDX);

   if ( !pDX->m_bSaveAndValidate )
   {
      UpdateCheckBoxes();
   }
}


BEGIN_MESSAGE_MAP(CGirderDebondCriteriaPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDebondCriteriaPage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_CHECK_MAX_LENGTH_FRACTION, OnCheckMaxLengthFraction)
	ON_BN_CLICKED(IDC_CHECK_MAX_LENGTH, OnCheckMaxLength)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDebondCriteriaPage message handlers
LRESULT CGirderDebondCriteriaPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_DEBOND_CRITERIA );
   return TRUE;
}

void CGirderDebondCriteriaPage::UpdateCheckBoxes()
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

void CGirderDebondCriteriaPage::OnCheckMaxLengthFraction() 
{
	UpdateCheckBoxes();
}

void CGirderDebondCriteriaPage::OnCheckMaxLength() 
{
	UpdateCheckBoxes();
}
