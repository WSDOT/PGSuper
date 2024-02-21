///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// LongSteelPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "LongSteelPage.h"
#include "GirderMainSheet.h"
#include <psglib\LibraryEditorDoc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLongSteelPage property page

IMPLEMENT_DYNCREATE(CLongSteelPage, CPropertyPage)

CLongSteelPage::CLongSteelPage() : CPropertyPage(CLongSteelPage::IDD,IDS_GIRDER_LONG)
{
	//{{AFX_DATA_INIT(CLongSteelPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

}

CLongSteelPage::~CLongSteelPage()
{
}

void CLongSteelPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLongSteelPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	DDV_GXGridWnd(pDX, &m_Grid);

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   if ( pDX->m_bSaveAndValidate )
   {
      WBFL::Materials::Rebar::Type type;
      WBFL::Materials::Rebar::Grade grade;
      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,type,grade);
      pDad->m_Entry.SetLongSteelMaterial(type,grade);
   }
   else
   {
      WBFL::Materials::Rebar::Type type;
      WBFL::Materials::Rebar::Grade grade;
      pDad->m_Entry.GetLongSteelMaterial(type,grade);
      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,type,grade);
   }

   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeLongitudinalData(pDX);
}

BEGIN_MESSAGE_MAP(CLongSteelPage, CPropertyPage)
	//{{AFX_MSG_MAP(CLongSteelPage)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoverows)
	ON_WM_NCACTIVATE()
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLongSteelPage message handlers

void CLongSteelPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CLongSteelPage::OnInitDialog() 
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   ASSERT(pDad);

	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();

   CEAFDocument* pEAFDoc = EAFGetDocument();
   bool bFilterBySpec = true;
   if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CLibraryEditorDoc)) )
      bFilterBySpec = false;

   int curSel = m_cbRebar.GetCurSel();
   m_cbRebar.Initialize(bFilterBySpec);
   m_cbRebar.SetCurSel(curSel);
	

   // can't delete strands at start
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);

   // set data in grids - would be nice to be able to do this in DoDataExchange,
   // but mfc sucks.
   pDad->UploadLongitudinalData();
   
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLongSteelPage::OnInsertrow() 
{
	m_Grid.Insertrow();
}

void CLongSteelPage::OnRemoverows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

BOOL CLongSteelPage::OnNcActivate(BOOL bActive)
{
	if (GXDiscardNcActivate())
		return TRUE;

	return CPropertyPage::OnNcActivate(bActive);
}

void CLongSteelPage::OnHelp()
{
   EAFHelp(  EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_LONGITUDINAL_REINFORCEMENT );
}

void CLongSteelPage::OnAppendRow() 
{
	m_Grid.Appendrow();
}

void CLongSteelPage::GetRebarMaterial(WBFL::Materials::Rebar::Type* pType,WBFL::Materials::Rebar::Grade* pGrade)
{
   m_cbRebar.GetMaterial(pType,pGrade);
}