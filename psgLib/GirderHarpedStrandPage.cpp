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

// Names are slightly different in library than rest of program
inline LPCTSTR LOCAL_LABEL_HARP_TYPE(pgsTypes::AdjustableStrandType type)
{
   if (pgsTypes::asHarped == type)
   {
      return _T("Harped");
   }
   else  if (pgsTypes::asStraight == type)
   {
      return _T("Adj. Straight");
   }
   else
   {
      return _T("Adjustable");
   }
}


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
   ON_BN_CLICKED(IDC_ALLOW_STR_ADJUST, &CGirderHarpedStrandPage::OnClickStraightAdjust)
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
   UpdateStraightAdjust();

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
   if (pgsTypes::asStraight        == GetAdjustableStrandType() ||
      pgsTypes::asStraightOrHarped == GetAdjustableStrandType())
   {
      // never need harped grid if web strands can be straight
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

void CGirderHarpedStrandPage::OnClickStraightAdjust()
{
   UpdateStraightAdjust();
}

void CGirderHarpedStrandPage::UpdateHpAdjust()
{
   BOOL enable = pgsTypes::asStraight!=GetAdjustableStrandType() ? TRUE : FALSE;

   int ctrls[] = {IDC_HP_INCREMENT, IDC_HP_INCREMENT_T, 
                  IDC_HP_LSL, IDC_HP_LSL_T, IDC_HP_LSL_S, IDC_COMBO_HP_LSL,
                  IDC_HP_USL, IDC_HP_USL_T, IDC_HP_USL_S, IDC_COMBO_HP_USL,
                  -1};

   UpdateAdjustCtls(enable, IDC_ALLOW_HP_ADJUST, ctrls);
}

void CGirderHarpedStrandPage::UpdateEndAdjust()
{
   BOOL enable = pgsTypes::asStraight!=GetAdjustableStrandType() ? TRUE : FALSE;

   int ctrls[] = {IDC_END_INCREMENT, IDC_END_INCREMENT_T, 
                  IDC_END_LSL, IDC_END_LSL_T, IDC_END_LSL_S, IDC_COMBO_END_LSL,
                  IDC_END_USL, IDC_END_USL_T, IDC_END_USL_S, IDC_COMBO_END_USL,
                  -1};

   UpdateAdjustCtls(enable, IDC_ALLOW_END_ADJUST, ctrls);
}

void CGirderHarpedStrandPage::UpdateStraightAdjust()
{
   BOOL enable = pgsTypes::asHarped!=GetAdjustableStrandType() ? TRUE : FALSE;

   int ctrls[] = {IDC_STR_INCREMENT, IDC_STR_INCREMENT_T, 
                  IDC_STR_LSL, IDC_STR_LSL_T, IDC_STR_LSL_S, IDC_COMBO_STR_LSL,
                  IDC_STR_USL, IDC_STR_USL_T, IDC_STR_USL_S, IDC_COMBO_STR_USL,
                  -1};

   UpdateAdjustCtls(enable, IDC_ALLOW_STR_ADJUST, ctrls);
}

void CGirderHarpedStrandPage::UpdateAdjustCtls(BOOL enableGroup, int checkCtrl, int ctrls[])
{
   CButton* pbut = (CButton*)GetDlgItem(checkCtrl);
   ASSERT(pbut);

   pbut->EnableWindow(enableGroup);

   BOOL enable = enableGroup & (pbut->GetCheck()==0 ? FALSE : TRUE);

   int idx=0;
   while (ctrls[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ctrls[idx]);
      ASSERT(pdel);
      pdel->EnableWindow(enable);

      idx++;
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

pgsTypes::AdjustableStrandType CGirderHarpedStrandPage::GetAdjustableStrandType()
{
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_WEB_STRAND_TYPE_COMBO);
   return (pgsTypes::AdjustableStrandType)pcb->GetCurSel();
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

   str.Format(_T("# %s Strands = %d"), LOCAL_LABEL_HARP_TYPE(GetAdjustableStrandType()), nh);

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
   pgsTypes::AdjustableStrandType adj_type = GetAdjustableStrandType();
   BOOL use_diffgrid = pgsTypes::asHarped == adj_type;

   CWnd* pw = (CWnd*)GetDlgItem(IDC_USE_DIFF_GRID);
   pw->EnableWindow(use_diffgrid);

   CString msg;
   msg.Format(_T("Coerce Odd Number of %s Strands"),LOCAL_LABEL_HARP_TYPE(adj_type));
   pw = (CWnd*)GetDlgItem(IDC_ODD_STRANDS);
   pw->SetWindowText(msg);

   BOOL use_harped = pgsTypes::asHarped == adj_type;

   pw = (CWnd*)GetDlgItem(IDC_STATIC_HP);
   if (use_harped)
   {
      pw->SetWindowText(_T("Strand Locations at Harping Points Measured from Bottom C.L. of Girder"));
   }
   else
   {
      pw->SetWindowText(_T("Strand Locations Measured from Bottom C.L. of Girder"));
   }
   
   pw = (CWnd*)GetDlgItem(IDC_STATIC_END);
   pw->ShowWindow(use_harped ? SW_SHOW : SW_HIDE);

   pw = (CWnd*)GetDlgItem(IDC_ADJUSTABLE_NOTE);
   pw->ShowWindow(pgsTypes::asStraightOrHarped == adj_type ? SW_SHOW : SW_HIDE);

   pw = (CWnd*)GetDlgItem(IDC_REVERSE_HARPED_STRAND_ORDER);
   pw->EnableWindow(pgsTypes::asStraight != adj_type);

   if (use_harped)
   {
      msg = _T("Vertical Adjustment of Harped Strands");
   }
   else
   {
      msg = _T("Vertical Adjustment of Adjustable Strands");
   }

   pw = (CWnd*)GetDlgItem(IDC_VERT_ADJUST_GROUP);
   pw->SetWindowText(msg);

   UpdateHpAdjust();
   UpdateEndAdjust();
   UpdateStraightAdjust();


   m_MainGrid.OnChangeWebStrandType();
   
}


