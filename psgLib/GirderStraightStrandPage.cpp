///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// GirderStraightStrandPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderStraightStrandPage.h"
#include "GirderMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderTemporaryStrandPage property page

IMPLEMENT_DYNCREATE(CGirderTemporaryStrandPage, CPropertyPage)

CGirderTemporaryStrandPage::CGirderTemporaryStrandPage() : 
CPropertyPage(CGirderTemporaryStrandPage::IDD),
m_TemporaryGrid(this)
{
	//{{AFX_DATA_INIT(CGirderTemporaryStrandPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderTemporaryStrandPage::~CGirderTemporaryStrandPage()
{
}

void CGirderTemporaryStrandPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderTemporaryStrandPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	DDV_GXGridWnd(pDX, &m_TemporaryGrid);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   if (!pDad->ExchangeTemporaryStrandData(pDX))
      pDX->Fail();
}


BEGIN_MESSAGE_MAP(CGirderTemporaryStrandPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderTemporaryStrandPage)
	ON_BN_CLICKED(IDC_ADD_TEMPORARY_STRAND, OnAddTemporaryStrand)
	ON_BN_CLICKED(IDC_DEL_TEMPORARY_STRAND, OnDelTemporaryStrand)
	ON_BN_CLICKED(IDC_APPEND_TEMPORARY_STRAND, OnAppendTemporaryStrand)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderTemporaryStrandPage message handlers

void CGirderTemporaryStrandPage::OnAddTemporaryStrand() 
{
	m_TemporaryGrid.Insertrow();
}

void CGirderTemporaryStrandPage::OnDelTemporaryStrand() 
{
	m_TemporaryGrid.Removerows();
}

void CGirderTemporaryStrandPage::OnEnableDelete(const CGirderStrandGrid* pgrid, bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_DEL_TEMPORARY_STRAND);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderTemporaryStrandPage::OnNcActivate(BOOL bActive)
{
	if (GXDiscardNcActivate())
		return TRUE;

	return CPropertyPage::OnNcActivate(bActive);
}

BOOL CGirderTemporaryStrandPage::OnInitDialog() 
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   ASSERT(pDad);

	CPropertyPage::OnInitDialog();
	

	m_TemporaryGrid.SubclassDlgItem(IDC_TEMPORARY_STRAND_GRID, this);
   m_TemporaryGrid.CustomInit();

   // set data in grids - would be nice to be able to do this in DoDataExchange,
   // but mfc sucks.
   pDad->UploadTemporaryStrandData();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderTemporaryStrandPage::OnAppendTemporaryStrand() 
{
	m_TemporaryGrid.Appendrow();
}

LRESULT CGirderTemporaryStrandPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_TEMPORARY_STRANDS_TAB );
   return TRUE;
}
