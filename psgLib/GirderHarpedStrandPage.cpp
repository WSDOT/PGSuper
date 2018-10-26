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

// GirderStrandPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderHarpedStrandPage.h"
#include "GirderMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderHarpedStrandPage property page

IMPLEMENT_DYNCREATE(CGirderHarpedStrandPage, CPropertyPage)

CGirderHarpedStrandPage::CGirderHarpedStrandPage() : 
CPropertyPage(CGirderHarpedStrandPage::IDD,IDS_GIRDER_STRAND),
m_MainGrid(this)
{
	//{{AFX_DATA_INIT(CGirderHarpedStrandPage)
	//}}AFX_DATA_INIT
}

CGirderHarpedStrandPage::~CGirderHarpedStrandPage()
{
}

void CGirderHarpedStrandPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderHarpedStrandPage)
	//}}AFX_DATA_MAP

	DDV_GXGridWnd(pDX, &m_MainGrid);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   if (!pDad->ExchangeStrandData(pDX))
      pDX->Fail();
}

BEGIN_MESSAGE_MAP(CGirderHarpedStrandPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderHarpedStrandPage)
	ON_BN_CLICKED(IDC_DEL_HARPED_STRAND, OnDelGlobalStrand)
	ON_BN_CLICKED(IDC_ADD_HARPED_STRAND, OnAddGlobalStrand)
	ON_BN_CLICKED(IDC_APPEND_HARPED_STRAND, OnAppendGlobalStrand)
	ON_BN_CLICKED(IDC_EDIT_STRAND, OnEditGlobalStrand)
	ON_BN_CLICKED(IDC_USE_DIFF_GRID, OnClickHarpedBox)
	ON_BN_CLICKED(IDC_ALLOW_HP_ADJUST, OnClickHpAdjust)
	ON_BN_CLICKED(IDC_ALLOW_END_ADJUST, OnClickEndAdjust)
	ON_BN_CLICKED(IDC_MOVEUP_HARPED_STRAND, OnMoveUpGlobalStrand)
	ON_BN_CLICKED(IDC_MOVEDOWN_HARPED_STRAND, OnMoveDownGlobalStrand)
	ON_BN_CLICKED(IDC_ENDVIEW, OnEndview)
	ON_WM_NCACTIVATE()
	ON_BN_CLICKED(IDC_MIDVIEW, OnMidview)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)

   ON_BN_CLICKED(IDC_REVERSE_HARPED_STRAND_ORDER,OnReverseHarpedStrandOrder)
   ON_BN_CLICKED(IDC_GENERATE,OnGenerateStrandPositions)
   ON_CBN_SELCHANGE(IDC_WEB_STRAND_TYPE_COMBO, &CGirderHarpedStrandPage::OnCbnSelchangeWebStrandTypeCombo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderHarpedStrandPage message handlers

BOOL CGirderHarpedStrandPage::OnInitDialog() 
{
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   ASSERT(pDad);

	CPropertyPage::OnInitDialog();
	
	m_MainGrid.SubclassDlgItem(IDC_HARP_STRAND_GRID, this);
   m_MainGrid.CustomInit();

   // can't delete strands at start
   OnEnableDelete(false);

   // update labels depending on straight or harped web strands
   OnCbnSelchangeWebStrandTypeCombo();

   // vertical adjustment controls
   UpdateHpAdjust();
   UpdateEndAdjust();

   // set data in grids - would be nice to be able to do this in DoDataExchange,
   // but mfc sucks.
   pDad->UploadStrandData();

   CWnd* pw = (CWnd*)GetDlgItem(IDC_STATIC_END);
   ASSERT(pw);
   pw->EnableWindow( DoUseHarpedGrid() ? TRUE : FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderHarpedStrandPage::OnEnableDelete(bool canDelete)
{
   int ctrls[] = {IDC_DEL_HARPED_STRAND, IDC_ADD_HARPED_STRAND, IDC_EDIT_STRAND, IDC_MOVEUP_HARPED_STRAND, IDC_MOVEDOWN_HARPED_STRAND, -1};

   int idx=0;
   while (ctrls[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ctrls[idx]);
      ASSERT(pdel);
      pdel->EnableWindow(canDelete ? TRUE : FALSE);

      idx++;
   }
}


BOOL CGirderHarpedStrandPage::OnNcActivate(BOOL bActive)
{
	if (GXDiscardNcActivate())
		return TRUE;

	return CPropertyPage::OnNcActivate(bActive);
}

void CGirderHarpedStrandPage::OnAddGlobalStrand() 
{
	m_MainGrid.InsertSelectedRow();
}

void CGirderHarpedStrandPage::OnDelGlobalStrand() 
{
	m_MainGrid.RemoveSelectedRow();
}

void CGirderHarpedStrandPage::OnClickHarpedBox() 
{
   m_MainGrid.OnChangeUseHarpedGrid();

   CWnd* pw = (CWnd*)GetDlgItem(IDC_STATIC_END);
   ASSERT(pw);
   pw->EnableWindow( DoUseHarpedGrid() ? TRUE : FALSE);
}

bool CGirderHarpedStrandPage::DoUseHarpedGrid()
{
   if (!DoUseHarpedWebStrands())
   {
      // never need harped grid if web strands are straight
      return false;
   }
   else
   {
      CButton* pbut = (CButton*)GetDlgItem(IDC_USE_DIFF_GRID);
      ASSERT(pbut);
      return pbut->GetCheck()!=0;
   }
}

LRESULT CGirderHarpedStrandPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_HARPED_STRANDS_TAB );

   return TRUE;
}

void CGirderHarpedStrandPage::OnAppendGlobalStrand() 
{
	m_MainGrid.AppendSelectedRow();
}

void CGirderHarpedStrandPage::OnEditGlobalStrand() 
{
	m_MainGrid.EditSelectedRow();
}

void CGirderHarpedStrandPage::OnMoveUpGlobalStrand()
{
   m_MainGrid.MoveUpSelectedRow();
}

void CGirderHarpedStrandPage::OnMoveDownGlobalStrand()
{
   m_MainGrid.MoveDownSelectedRow();
}

void CGirderHarpedStrandPage::OnClickHpAdjust()
{
   UpdateHpAdjust();
}

void CGirderHarpedStrandPage::OnClickEndAdjust()
{
   UpdateEndAdjust();
}

void CGirderHarpedStrandPage::UpdateHpAdjust()
{
   CButton* pbut = (CButton*)GetDlgItem(IDC_ALLOW_HP_ADJUST);
   ASSERT(pbut);

   if(DoUseHarpedWebStrands())
   {
      pbut->SetWindowTextW(_T("At Harping Points"));
   }
   else
   {
      pbut->SetWindowTextW(_T("Along Girder"));
   }

   BOOL enable = pbut->GetCheck()==0 ? FALSE : TRUE;

   int ctrls[] = {IDC_HP_INCREMENT, IDC_HP_INCREMENT_T, 
                  IDC_HP_LSL, IDC_HP_LSL_T, IDC_HP_LSL_S, IDC_COMBO_HP_LSL,
                  IDC_HP_USL, IDC_HP_USL_T, IDC_HP_USL_S, IDC_COMBO_HP_USL,
                  -1};

   int idx=0;
   while (ctrls[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ctrls[idx]);
      ASSERT(pdel);
      pdel->EnableWindow(enable);

      idx++;
   }
}

void CGirderHarpedStrandPage::UpdateEndAdjust()
{
   CButton* pbut = (CButton*)GetDlgItem(IDC_ALLOW_END_ADJUST);
   ASSERT(pbut);
   BOOL enable = pbut->GetCheck()==0 ? FALSE : TRUE;

   int ctrls[] = {IDC_ALLOW_END_ADJUST, IDC_END_INCREMENT, IDC_END_INCREMENT_T, 
                  IDC_END_LSL, IDC_END_LSL_T, IDC_END_LSL_S, IDC_COMBO_END_LSL,
                  IDC_END_USL, IDC_END_USL_T, IDC_END_USL_S, IDC_COMBO_END_USL,
                  -1};

   int show = DoUseHarpedWebStrands() ? SW_SHOW : SW_HIDE;

   int idx=0;
   while (ctrls[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ctrls[idx]);
      ASSERT(pdel);
      pdel->ShowWindow(show);

      idx++;
   }

   if (show==SW_SHOW)
   {
      idx=1;
      while (ctrls[idx] != -1)
      {
         CWnd* pdel = GetDlgItem(ctrls[idx]);
         ASSERT(pdel);
         pdel->EnableWindow(enable);

         idx++;
      }
   }
}

void CGirderHarpedStrandPage::OnEndview() 
{
   if ( !UpdateData(TRUE) )
      return;

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   pDad->m_GirderDimensionsPage.ViewSection(true);
}

void CGirderHarpedStrandPage::OnMidview() 
{
   if ( !UpdateData(TRUE) )
      return;

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   pDad->m_GirderDimensionsPage.ViewSection(false);
}

bool CGirderHarpedStrandPage::DoUseHarpedWebStrands()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_WEB_STRAND_TYPE_COMBO);
   return pcb->GetCurSel()==0 ? TRUE : FALSE;
}

void CGirderHarpedStrandPage::UpdateStrandStatus(Uint16 ns, Uint16 ndb, Uint16 nh)
{
   CString str;
   str.Format(_T("# Straight Strands = %d"), ns);

   CWnd* pw = (CWnd*)GetDlgItem(IDC_SS_STATUS);
   ASSERT(pw);
   pw->SetWindowText(str);

   str.Format(_T("# Debondable Strands = %d"), ndb);

   pw = (CWnd*)GetDlgItem(IDC_DB_STATUS);
   ASSERT(pw);
   pw->SetWindowText(str);

   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_WEB_STRAND_TYPE_COMBO);
   BOOL use_harped = pcb->GetCurSel()==0 ? TRUE : FALSE;

   str.Format(_T("# %s Strands = %d"), LABEL_HARP_TYPE(!use_harped), nh);

   pw = (CWnd*)GetDlgItem(IDC_HS_STATUS);
   ASSERT(pw);
   pw->SetWindowText(str);

   str.Format(_T("Total Perm. Strands = %d"), nh+ns);

   pw = (CWnd*)GetDlgItem(IDC_TT_STATUS);
   ASSERT(pw);
   pw->SetWindowText(str);
}

void CGirderHarpedStrandPage::OnReverseHarpedStrandOrder()
{
   m_MainGrid.ReverseHarpedStrandOrder();
}

void CGirderHarpedStrandPage::OnGenerateStrandPositions()
{
   m_MainGrid.GenerateStrandPositions();
}

void CGirderHarpedStrandPage::OnCbnSelchangeWebStrandTypeCombo()
{
   BOOL use_harped = DoUseHarpedWebStrands();

   CWnd* pw = (CWnd*)GetDlgItem(IDC_USE_DIFF_GRID);
   pw->EnableWindow(use_harped);

   CString msg;
   msg.Format(_T("Coerce Odd Number of %s Strands"),LABEL_HARP_TYPE(!use_harped));
   pw = (CWnd*)GetDlgItem(IDC_ODD_STRANDS);
   pw->SetWindowTextW(msg);

   pw = (CWnd*)GetDlgItem(IDC_STATIC_HP);
   if (use_harped)
   {
      pw->SetWindowTextW(_T("Strand Locations at Harping Points Measured from Bottom C.L. of Girder"));
   }
   else
   {
      pw->SetWindowTextW(_T("Strand Locations Along Girder Measured from Bottom C.L. of Girder"));
   }
   
   pw = (CWnd*)GetDlgItem(IDC_STATIC_END);
   pw->ShowWindow(use_harped ? SW_SHOW : SW_HIDE);

   pw = (CWnd*)GetDlgItem(IDC_REVERSE_HARPED_STRAND_ORDER);
   pw->EnableWindow(use_harped);

   msg.Format(_T("Vertical Adjustment of %s Strands"),LABEL_HARP_TYPE(!use_harped));
   pw = (CWnd*)GetDlgItem(IDC_VERT_ADJUST_GROUP);
   pw->SetWindowTextW(msg);

   UpdateHpAdjust();
   UpdateEndAdjust();


   m_MainGrid.OnChangeWebStrandType();
   
}
