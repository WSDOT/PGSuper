///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PsgLib\PsgLib.h>
#include "GirderDiaphragmPage.h"
#include "GirderMainSheet.h"

#include "DiaphragmDefinitionDlg.h"

#include <IFace\BeamFactory.h>
#include <EAF\EAFDocument.h>



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
   {
      return FALSE;
   }

   CGirderMainSheet* pParent = (CGirderMainSheet*)GetParent();
   auto beamFactory = pParent->m_Entry.GetBeamFactory();

   pgsTypes::SupportedDiaphragmTypes diaphragmTypes = beamFactory->GetSupportedDiaphragms();
   if ( diaphragmTypes.size() == 0 )
   {
      // diaphragm rules are not supported by this girder
      GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT1)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_DELETE)->ShowWindow(SW_HIDE);
      m_Grid.ShowWindow(SW_HIDE);
      GetDlgItem(IDC_NO_DIAPHRAGMS)->ShowWindow(SW_SHOW);
   }

   return TRUE;
}



BEGIN_MESSAGE_MAP(CGirderDiaphragmPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDiaphragmPage)
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_BN_CLICKED(IDC_ADD,OnAdd)
   ON_BN_CLICKED(IDC_EDIT1,OnEdit)
   ON_BN_CLICKED(IDC_DELETE,OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDiaphragmPage message handlers
void CGirderDiaphragmPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_DIAPHRAGMS );
}

void CGirderDiaphragmPage::OnAdd()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   GirderLibraryEntry::DiaphragmLayoutRule rule; // new rule

   CGirderMainSheet* pParent = (CGirderMainSheet*)GetParent();
   CDiaphragmDefinitionDlg dlg(pParent->m_Entry,rule);

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

   ROWCOL row = m_Grid.GetRowCount();
   if (row == 0)
   {
      OnAdd();
      return;
   }

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   CRowColArray rows;
   ROWCOL nSelected = m_Grid.GetSelectedRows(rows);

   if ( nSelected == 0 )
   {
      m_Grid.SelectRange(CGXRange(1,0));
   }

   nSelected = m_Grid.GetSelectedRows(rows);
   ASSERT(nSelected != 0);

   CGirderMainSheet* pParent = (CGirderMainSheet*)GetParent();
   GirderLibraryEntry::DiaphragmLayoutRule* pRule = m_Grid.GetRule(rows[0]);
   CDiaphragmDefinitionDlg dlg(pParent->m_Entry,*pRule);

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
