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

// BridgeDescLongitudinalRebar.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescLongitudinalRebar.h"
#include "GirderDescDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongitudinalRebar property page

IMPLEMENT_DYNCREATE(CGirderDescLongitudinalRebar, CPropertyPage)

CGirderDescLongitudinalRebar::CGirderDescLongitudinalRebar() : CPropertyPage(CGirderDescLongitudinalRebar::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescLongitudinalRebar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDescLongitudinalRebar::~CGirderDescLongitudinalRebar()
{
}

void CGirderDescLongitudinalRebar::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescLongitudinalRebar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDV_GXGridWnd(pDX, &m_Grid);

   DDX_CBStringExactCase(pDX,IDC_MILD_STEEL_SELECTOR,m_RebarData.strRebarMaterial);

   // longitudinal steel information from grid and store it
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if (pDX->m_bSaveAndValidate)
   {
      CLongitudinalRebarData rebarData;

      ROWCOL nrows = m_Grid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         CLongitudinalRebarData::RebarRow row;
         if (m_Grid.GetRowData(i,&row))
         {
            // values are in display units - must convert to system
            row.Cover      = ::ConvertToSysUnits(row.Cover,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
            row.BarSpacing = ::ConvertToSysUnits(row.BarSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
            rebarData.RebarRows.push_back(row);
         }
      }
      m_RebarData = rebarData;
   }
   else
   {
      CLongitudinalRebarData rebardata;
      std::vector<CLongitudinalRebarData::RebarRow>::iterator iter;
      for ( iter = m_RebarData.RebarRows.begin(); iter != m_RebarData.RebarRows.end(); iter++ )
      {
         CLongitudinalRebarData::RebarRow row = *iter;
         row.BarSpacing = ::ConvertFromSysUnits(row.BarSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
         row.Cover      = ::ConvertFromSysUnits(row.Cover,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

         rebardata.RebarRows.push_back(row);
      }
      m_Grid.FillGrid(rebardata);
   }
}


BEGIN_MESSAGE_MAP(CGirderDescLongitudinalRebar, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescLongitudinalRebar)
	ON_BN_CLICKED(IDC_RESTORE_DEFAULTS1, OnRestoreDefaults)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongitudinalRebar message handlers

void CGirderDescLongitudinalRebar::RestoreToLibraryDefaults()
{
   // get shear information from library
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGird = pLib->GetGirderEntry( m_CurGrdName.c_str());
   ASSERT(pGird!=0);

   // update data member
   m_RebarData.CopyGirderEntryData(*pGird);
}

void CGirderDescLongitudinalRebar::OnRestoreDefaults() 
{
	RestoreToLibraryDefaults();
   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CGirderDescLongitudinalRebar::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderDescLongitudinalRebar::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();
	
   // select the one and only material
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   ASSERT(pc);
   pc->SetCurSel(0);

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescLongitudinalRebar::OnInsertrow() 
{
   m_Grid.Insertrow();
}

void CGirderDescLongitudinalRebar::OnAppendRow() 
{
   m_Grid.Appendrow();
}

void CGirderDescLongitudinalRebar::OnRemoveRows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CGirderDescLongitudinalRebar::OnHelp() 
{
	::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_REBAR );
	
}
