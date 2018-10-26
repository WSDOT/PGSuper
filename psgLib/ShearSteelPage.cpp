///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// ShearSteelPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "ShearSteelPage.h"
#include "GirderMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"
#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage property page

IMPLEMENT_DYNCREATE(CShearSteelPage, CPropertyPage)

CShearSteelPage::CShearSteelPage() : CPropertyPage(CShearSteelPage::IDD,IDS_GIRDER_SHEAR)
{
	//{{AFX_DATA_INIT(CShearSteelPage)
	//}}AFX_DATA_INIT
}

CShearSteelPage::~CShearSteelPage()
{
}

void CShearSteelPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShearSteelPage)
	DDX_Control(pDX, IDC_TF_BAR_SIZE, m_TfBarSize);
	DDX_Control(pDX, IDC_BAR_SIZE, m_BarSize);
	DDX_Control(pDX, IDC_LAST_ZONE, m_LastZone);
   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_MaterialName);
	//}}AFX_DATA_MAP

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

	DDV_GXGridWnd(pDX, &m_Grid);

   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeTransverseData(pDX);
}


BEGIN_MESSAGE_MAP(CShearSteelPage, CPropertyPage)
	//{{AFX_MSG_MAP(CShearSteelPage)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertRow)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage message handlers

void CShearSteelPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CShearSteelPage::OnInitDialog() 
{

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   ASSERT(pDad);

	CPropertyPage::OnInitDialog();
	
	m_Grid.SubclassDlgItem(IDC_SHEAR_GRID, this);
   m_Grid.CustomInit();

   // can't delete strands at start
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);

   // select the one and only material
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   ASSERT(pc);
   pc->SetCurSel(0);

   // set data in grids - would be nice to be able to do this in DoDataExchange,
   // but mfc sucks.
   pDad->UploadTransverseData();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CShearSteelPage::OnRemoveRows() 
{
   DoRemoveRows();
}

void CShearSteelPage::DoRemoveRows() 
{
	m_Grid.DoRemoveRows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);

   // update list control
   int lz = m_LastZone.GetCurSel();
   ROWCOL nrows = m_Grid.GetRowCount();
   FillLastZone(nrows);
   if (lz==CB_ERR)
      m_LastZone.SetCurSel(0);
   else if ((ROWCOL)lz>nrows)
      m_LastZone.SetCurSel(nrows);
   else
      m_LastZone.SetCurSel(lz);
}

void CShearSteelPage::OnInsertRow() 
{
   DoInsertRow();
}

void CShearSteelPage::DoInsertRow() 
{
	m_Grid.DoInsertRow();

   // update list control
   int lz = m_LastZone.GetCurSel();
   ROWCOL nrows = m_Grid.GetRowCount();
   FillLastZone(nrows);
   if (lz==CB_ERR)
      m_LastZone.SetCurSel(0);
   else
      m_LastZone.SetCurSel(lz);

}

void CShearSteelPage::FillLastZone(int siz)
{
   CString tmp;
   m_LastZone.ResetContent();
   m_LastZone.AddString("none");
   for (int i=1; i<=siz; i++)
   {
      tmp.Format("Zone %d",i);
      m_LastZone.AddString(tmp);
   }
}

LRESULT CShearSteelPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_TRANSVERSE_REINFORCEMENT_TAB );
   return TRUE;
}

