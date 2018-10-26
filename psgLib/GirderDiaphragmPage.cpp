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

// GirderDiaphragmPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderDiaphragmPage.h"
#include "GirderMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"

#include "DiaphragmDefinitionDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDiaphragmPage property page

IMPLEMENT_DYNCREATE(CGirderDiaphragmPage, CPropertyPage)

CGirderDiaphragmPage::CGirderDiaphragmPage() : CPropertyPage(CGirderDiaphragmPage::IDD,IDS_GIRDER_DIAPHRAGM)
{
	//{{AFX_DATA_INIT(CGirderDiaphragmPage)
	//}}AFX_DATA_INIT
}

CGirderDiaphragmPage::~CGirderDiaphragmPage()
{
}

void CGirderDiaphragmPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDiaphragmPage)
	//}}AFX_DATA_MAP

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDiaphragmData(pDX);
}

BOOL CGirderDiaphragmPage::OnInitDialog()
{
	m_Grid.SubclassDlgItem(IDC_DIAPHRAGM_GRID, this);
   m_Grid.CustomInit();

   if ( !CPropertyPage::OnInitDialog() )
      return FALSE;

   return TRUE;
}



BEGIN_MESSAGE_MAP(CGirderDiaphragmPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDiaphragmPage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
   ON_BN_CLICKED(IDC_ADD,OnAdd)
   ON_BN_CLICKED(IDC_EDIT1,OnEdit)
   ON_BN_CLICKED(IDC_DELETE,OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDiaphragmPage message handlers
LRESULT CGirderDiaphragmPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIAPHRAGM_LAYOUT_DIALOG );
   return TRUE;
}

void CGirderDiaphragmPage::OnAdd()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   GirderLibraryEntry::DiaphragmLayoutRule rule; // new rule

   CDiaphragmDefinitionDlg dlg;

   dlg.m_Rule = rule;
   if ( dlg.DoModal() == IDOK )
   {
      m_Grid.AddRow(); // only add if editing is OK
      ROWCOL row = m_Grid.GetRowCount();
      m_Grid.SetRule(row,dlg.m_Rule);
   }
}

void CGirderDiaphragmPage::OnEdit()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   CRowColArray rows;
   ROWCOL nSelected = m_Grid.GetSelectedRows(rows);

   if ( nSelected == 0 )
      m_Grid.SelectRange(CGXRange(1,0));

   nSelected = m_Grid.GetSelectedRows(rows);
   ASSERT(nSelected != 0);

   CDiaphragmDefinitionDlg dlg;

   GirderLibraryEntry::DiaphragmLayoutRule* pRule = m_Grid.GetRule(rows[0]);

   dlg.m_Rule = *pRule;
   if ( dlg.DoModal() == IDOK )
   {
      m_Grid.SetRule(rows[0],dlg.m_Rule);
   }
}

void CGirderDiaphragmPage::OnDelete()
{
   m_Grid.RemoveRows();
}

void CGirderDiaphragmPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_DELETE);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}
