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

// GirderSegmentLongitudinalRebarPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "GirderSegmentLongitudinalRebarPage.h"
#include "GirderSegmentDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include "HtmlHelp\HelpTopics.hh"
#include <PsgLib\RebarUIUtils.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage property page

IMPLEMENT_DYNCREATE(CGirderSegmentLongitudinalRebarPage, CPropertyPage)

CGirderSegmentLongitudinalRebarPage::CGirderSegmentLongitudinalRebarPage() : CPropertyPage(CGirderSegmentLongitudinalRebarPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderSegmentLongitudinalRebarPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderSegmentLongitudinalRebarPage::~CGirderSegmentLongitudinalRebarPage()
{
}

void CGirderSegmentLongitudinalRebarPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderSegmentLongitudinalRebarPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDV_GXGridWnd(pDX, &m_Grid);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   // longitudinal steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      int idx;
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);
      GetStirrupMaterial(idx,pSegment->LongitudinalRebarData.BarType,pSegment->LongitudinalRebarData.BarGrade);
      m_Grid.GetRebarData(pSegment->LongitudinalRebarData.RebarRows);
   }
   else
   {
      int idx = GetStirrupMaterialIndex(pSegment->LongitudinalRebarData.BarType,pSegment->LongitudinalRebarData.BarGrade);
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);
      m_Grid.FillGrid(pSegment->LongitudinalRebarData);
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentLongitudinalRebarPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentLongitudinalRebarPage)
	ON_BN_CLICKED(IDC_RESTORE_DEFAULTS1, OnRestoreDefaults)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage message handlers
void CGirderSegmentLongitudinalRebarPage::RestoreToLibraryDefaults()
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   const GirderLibraryEntry* pGird = pParent->m_Girder.GetGirderLibraryEntry();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   // update data member
   pSegment->LongitudinalRebarData.CopyGirderEntryData(*pGird);
}

void CGirderSegmentLongitudinalRebarPage::OnRestoreDefaults() 
{
	RestoreToLibraryDefaults();
   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CGirderSegmentLongitudinalRebarPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderSegmentLongitudinalRebarPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   FillRebarMaterialComboBox(pc);

   CPropertyPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderSegmentLongitudinalRebarPage::OnInsertrow() 
{
   m_Grid.Insertrow();
}

void CGirderSegmentLongitudinalRebarPage::OnAppendRow() 
{
   m_Grid.Appendrow();
}

void CGirderSegmentLongitudinalRebarPage::OnRemoveRows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CGirderSegmentLongitudinalRebarPage::OnHelp() 
{
	::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_REBAR );
	
}
