///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// BridgeDescShearPage.cpp : implementation file
//
#include "stdafx.h"
#include <PsgLib\ShearData.h>
#include <PsgLib\ShearSteelPage.h>
#include <PsgLib\RebarUIUtils.h>
#include <psglib\LibraryEditorDoc.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <MfcTools\CustomDDX.h>

#include <Lrfd\RebarPool.h>
#include "ShearSteelGrid.h"
#include "HorizShearGrid.h"

#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const DWORD CShearSteelPage::IDD = IDD_EDIT_SHEAR_STEEL;

/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage property page

IMPLEMENT_DYNCREATE(CShearSteelPage, CPropertyPage)

CShearSteelPage::CShearSteelPage():
m_AllowRestoreDefaults(false)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::Construct(CShearSteelPage::IDD,IDS_GIRDER_SHEAR);

   m_pGrid = std::auto_ptr<CShearSteelGrid>(new CShearSteelGrid());
   m_pHorizGrid = std::auto_ptr<CHorizShearGrid>(new CHorizShearGrid());

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
	//}}AFX_DATA_MAP

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX,IDC_SYMMETRICAL,m_ShearData.bAreZonesSymmetrical);
   DDX_Check_Bool(pDX,IDC_ROUGHENED,           m_ShearData.bIsRoughenedSurface);
   DDX_Check_Bool(pDX,IDC_CHECK_SPLITTING,     m_ShearData.bUsePrimaryForSplitting);

   // Splitting
   DDX_UnitValueAndTag(pDX, IDC_SPLITTING_ZL, IDC_SPLITTING_ZL_UNIT, m_ShearData.SplittingZoneLength, pDisplayUnits->XSectionDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_SPLITTING_ZL, m_ShearData.SplittingZoneLength, pDisplayUnits->XSectionDim );

   DDX_UnitValueAndTag(pDX, IDC_SPLITTING_SPACING, IDC_SPLITTING_SPACING_UNIT, m_ShearData.SplittingBarSpacing, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_SPLITTING_SPACING, m_ShearData.SplittingBarSpacing, pDisplayUnits->ComponentDim );

   DDX_CBItemData(pDX,IDC_SPLITTING_BAR_SIZE,m_ShearData.SplittingBarSize);

   DDX_Text(pDX,IDC_SPLITTING_NLEGS,m_ShearData.nSplittingBars);
   DDV_NonNegativeDouble(pDX,IDC_SPLITTING_NLEGS,m_ShearData.nSplittingBars);

   if (pDX->m_bSaveAndValidate)
   {
      if (matRebar::bsNone != m_ShearData.SplittingBarSize)
      {
         if (m_ShearData.SplittingZoneLength<=0.00001)
         {
            AfxMessageBox(_T("Splitting Zone length must be greater than zero if bars are specified"));
            pDX->PrepareCtrl(IDC_SPLITTING_ZL);
            pDX->Fail();
         }

         if (m_ShearData.SplittingZoneLength<m_ShearData.SplittingBarSpacing)
         {
            AfxMessageBox(_T("Splitting Zone bar spacing must be less than zone length."));
            pDX->PrepareCtrl(IDC_SPLITTING_SPACING);
            pDX->Fail();
         }
      }
   }

   // Confinement
   DDX_UnitValueAndTag(pDX, IDC_CONFINE_ZL, IDC_CONFINE_ZL_UNIT, m_ShearData.ConfinementZoneLength, pDisplayUnits->XSectionDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_CONFINE_ZL, m_ShearData.ConfinementZoneLength, pDisplayUnits->XSectionDim );

   DDX_UnitValueAndTag(pDX, IDC_CONFINE_SPACING, IDC_CONFINE_SPACING_UNIT, m_ShearData.ConfinementBarSpacing, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_CONFINE_SPACING, m_ShearData.ConfinementBarSpacing, pDisplayUnits->ComponentDim );

   DDX_CBItemData(pDX,IDC_CONFINE_BAR_SIZE,m_ShearData.ConfinementBarSize);

   if (pDX->m_bSaveAndValidate)
   {
      if (matRebar::bsNone != m_ShearData.ConfinementBarSize)
      {
         if (m_ShearData.ConfinementZoneLength<=0.00001)
         {
            AfxMessageBox(_T("Confinement Zone length must be greater than zero if bars are specified"));
            pDX->PrepareCtrl(IDC_CONFINE_ZL);
            pDX->Fail();
         }

         if (m_ShearData.ConfinementZoneLength<m_ShearData.ConfinementBarSpacing)
         {
            AfxMessageBox(_T("Confinement Zone bar spacing must be less than zone length."));
            pDX->PrepareCtrl(IDC_CONFINE_SPACING);
            pDX->Fail();
         }
      }
   }

   // Primary Grid
	DDV_GXGridWnd(pDX, m_pGrid.get());

   if (pDX->m_bSaveAndValidate)
   {
      int idx;
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);
      if ( idx == CB_ERR )
      {
         m_ShearData.ShearBarType = matRebar::A615;
         m_ShearData.ShearBarGrade = matRebar::Grade60;
      }
      else
      {
         GetStirrupMaterial(idx,m_ShearData.ShearBarType,m_ShearData.ShearBarGrade);
      }

      // zone info from grid
      m_ShearData.ShearZones.clear();

      CShearZoneData lsi;
      ROWCOL nrows = m_pGrid->GetRowCount();
      Uint32 zn = 0;
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (m_pGrid->GetRowData(i,nrows,&lsi))
         {
            lsi.ZoneNum = zn;
            if ( !IsEqual(lsi.ZoneLength,Float64_Max) )
            {
               lsi.ZoneLength = ::ConvertToSysUnits(lsi.ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
            }
            lsi.BarSpacing = ::ConvertToSysUnits(lsi.BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);

            // make sure stirrup spacing is greater than zone length
            if (zn+1 < nrows)
            {
               if (lsi.ZoneLength<=0.00001)
               {
                  CString msg;
                  msg.Format(_T("Zone length must be greater than zero in Shear Zone %d"),zn+1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            if ((matRebar::bsNone != lsi.VertBarSize || matRebar::bsNone != lsi.ConfinementBarSize) && zn < nrows)
            {
               if (lsi.ZoneLength<lsi.BarSpacing)
               {
                  CString msg;
                  msg.Format(_T("Bar spacing must be less than zone length in Shear Zone %d"),zn+1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            // make sure stirrup spacing is >0 if stirrups or confinement bars exist
            if ( (matRebar::bsNone != lsi.VertBarSize || matRebar::bsNone != lsi.ConfinementBarSize) && lsi.BarSpacing<=0.0)
            {
               CString msg;
               msg.Format(_T("Bar spacing must be greater than zero if stirrups exist in Shear Zone %d"),zn+1);
               AfxMessageBox(msg);
               pDX->Fail();
            }
            
            m_ShearData.ShearZones.push_back(lsi);
            zn++;
         }
      }
  }
   else
   {
      // fill er up
      int idx = GetStirrupMaterialIndex(m_ShearData.ShearBarType,m_ShearData.ShearBarGrade);
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);

      // grid
      CShearData::ShearZoneVec vec;
      for (CShearData::ShearZoneConstIterator it = m_ShearData.ShearZones.begin(); it!=m_ShearData.ShearZones.end(); it++)
      {
         // Copy all data then convert length values
         CShearZoneData inf(*it);
         if ( !IsEqual(inf.ZoneLength,Float64_Max) )
         {
            inf.ZoneLength = ::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
         }
         inf.BarSpacing     = ::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
         vec.push_back(inf);
      }
      m_pGrid->FillGrid(vec, m_ShearData.bAreZonesSymmetrical);

      // can't delete strands at start
      CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
      ASSERT(pdel);
      pdel->EnableWindow(FALSE);

   }

   // Horiz shear Grid
	DDV_GXGridWnd(pDX, m_pHorizGrid.get());

   if (pDX->m_bSaveAndValidate)
   {
      // zone info from grid
      m_ShearData.HorizontalInterfaceZones.clear();

      CHorizontalInterfaceZoneData lsi;
      ROWCOL nrows = m_pHorizGrid->GetRowCount();
      Uint32 zn = 0;
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (m_pHorizGrid->GetRowData(i,nrows,&lsi))
         {
            lsi.ZoneNum = zn;
            if ( !IsEqual(lsi.ZoneLength,Float64_Max) )
            {
               lsi.ZoneLength = ::ConvertToSysUnits(lsi.ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
            }
            lsi.BarSpacing = ::ConvertToSysUnits(lsi.BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);

            // make sure stirrup spacing is greater than zone length
            if (zn+1 < nrows)
            {
               if (lsi.ZoneLength<=0.00001)
               {
                  CString msg;
                  msg.Format(_T("Zone length must be greater than zero in Horiz Interface Shear Zone %d"),zn+1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            if (matRebar::bsNone != lsi.BarSize && zn < nrows)
            {
               if (lsi.ZoneLength<lsi.BarSpacing)
               {
                  CString msg;
                  msg.Format(_T("Bar spacing must be less than zone length in Horiz Interface Shear Zone %d"),zn+1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            // make sure stirrup spacing is >0 if stirrups or confinement bars exist
            if ( (matRebar::bsNone != lsi.BarSize) && lsi.BarSpacing<=0.0)
            {
               CString msg;
               msg.Format(_T("Bar spacing must be greater than zero if stirrups exist in Horiz Interface Shear Zone %d"),zn+1);
               AfxMessageBox(msg);
               pDX->Fail();
            }
            
            m_ShearData.HorizontalInterfaceZones.push_back(lsi);
            zn++;
         }
      }
  }
   else
   {
      CShearData::HorizontalInterfaceZoneVec vec;
      for (CShearData::HorizontalInterfaceZoneConstIterator it = m_ShearData.HorizontalInterfaceZones.begin(); it!=m_ShearData.HorizontalInterfaceZones.end(); it++)
      {
         // Copy all data then convert length values
         CHorizontalInterfaceZoneData inf(*it);
         if ( !IsEqual(inf.ZoneLength,Float64_Max) )
         {
            inf.ZoneLength     = ::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
         }

         inf.BarSpacing     = ::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
         vec.push_back(inf);
      }
      m_pHorizGrid->FillGrid(vec, m_ShearData.bAreZonesSymmetrical);

      // can't delete strands at start
      CWnd* pdel = GetDlgItem(IDC_REMOVEHORIZROWS);
      ASSERT(pdel);
      pdel->EnableWindow(FALSE);
   }
}


BEGIN_MESSAGE_MAP(CShearSteelPage, CPropertyPage)
	//{{AFX_MSG_MAP(CShearSteelPage)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertRow)
	ON_BN_CLICKED(IDC_REMOVEHORIZROWS, OnRemoveHorizRows)
	ON_BN_CLICKED(IDC_INSERTHORIZROW, OnInsertHorizRow)
	ON_BN_CLICKED(IDC_SYMMETRICAL, OnClickedSymmetrical)
   ON_BN_CLICKED(IDC_RESTORE_DEFAULTS, OnRestoreDefaults)
	ON_COMMAND(ID_HELP, OnHelp)
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

void CShearSteelPage::OnEnableHorizDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEHORIZROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CShearSteelPage::OnInitDialog() 
{
	m_pGrid->SubclassDlgItem(IDC_SHEAR_GRID, this);
   m_pGrid->CustomInit();

	m_pHorizGrid->SubclassDlgItem(IDC_HORIZ_GRID, this);
   m_pHorizGrid->CustomInit();

   CEAFDocument* pEAFDoc = EAFGetDocument();
   bool bFilterBySpec = true;
   if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CLibraryEditorDoc)) )
      bFilterBySpec = false;

   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   FillRebarMaterialComboBox(pc,bFilterBySpec);

   FillBarComboBox((CComboBox*)GetDlgItem(IDC_SPLITTING_BAR_SIZE));
   FillBarComboBox((CComboBox*)GetDlgItem(IDC_CONFINE_BAR_SIZE));

   // 
   CWnd* pw = (CWnd*)GetDlgItem(IDC_RESTORE_DEFAULTS);
   pw->ShowWindow(m_AllowRestoreDefaults ? SW_SHOW:SW_HIDE);

	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CShearSteelPage::OnRemoveRows() 
{
   DoRemoveRows();
}

void CShearSteelPage::DoRemoveRows()
{
	m_pGrid->DoRemoveRows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CShearSteelPage::OnInsertRow() 
{
   DoInsertRow();
}

void CShearSteelPage::DoInsertRow() 
{
	m_pGrid->InsertRow(false); // insert at top if no selection
}

void CShearSteelPage::OnRemoveHorizRows() 
{
   DoRemoveHorizRows();
}

void CShearSteelPage::DoRemoveHorizRows()
{
	m_pHorizGrid->DoRemoveRows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEHORIZROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CShearSteelPage::OnInsertHorizRow() 
{
   DoInsertHorizRow();
}

void CShearSteelPage::DoInsertHorizRow() 
{
	m_pHorizGrid->InsertRow(false); // insert at top if no selection
}


void CShearSteelPage::OnClickedSymmetrical()
{
   CButton* pdel = (CButton*)GetDlgItem(IDC_SYMMETRICAL);
   ASSERT(pdel);
   bool is_sym = pdel->GetCheck()!=BST_UNCHECKED;

   m_pGrid->SetSymmetry(is_sym);
   m_pHorizGrid->SetSymmetry(is_sym);
}


BOOL CShearSteelPage::OnSetActive() 
{
	BOOL val = CPropertyPage::OnSetActive();

   m_pGrid->SelectRange(CGXRange().SetTable(), FALSE);
   m_pHorizGrid->SelectRange(CGXRange().SetTable(), FALSE);

   return val;
}

void CShearSteelPage::OnHelp() 
{
   UINT helpID = IDH_TRANSVERSE_REINFORCEMENT_TAB;

   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, helpID );
}

void CShearSteelPage::FillBarComboBox(CComboBox* pCB)
{
   int idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bsNone).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bsNone);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs3).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs3);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs4).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs4);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs5).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs5);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs6).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs6);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs7).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs7);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs8).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs8);

   idx = pCB->AddString(lrfdRebarPool::GetBarSize(matRebar::bs9).c_str());
   pCB->SetItemData(idx,(DWORD_PTR)matRebar::bs9);
}


void CShearSteelPage::OnRestoreDefaults()
{
    DoRestoreDefaults();

   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CShearSteelPage::DoRestoreDefaults()
{
    ATLASSERT(0); // library should never want to do this
}