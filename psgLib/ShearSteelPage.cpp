///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <psglib\LibraryEditorDoc.h>
#include "AdditionalInterfaceShearBarDlg.h"

#include <IFace\Project.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Tools.h>
#include <MfcTools\CustomDDX.h>


#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const DWORD CShearSteelPage::IDD = IDD_EDIT_SHEAR_STEEL;

static Float64 zone_bar_spacing_tolerance = ::ConvertToSysUnits(0.0001, unitMeasure::Feet);

/////////////////////////////////////////////////////////////////////////////
// CShearSteelPage property page

IMPLEMENT_DYNCREATE(CShearSteelPage, CPropertyPage)

CShearSteelPage::CShearSteelPage():
m_bAllowRestoreDefaults(false)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::Construct(CShearSteelPage::IDD,IDS_GIRDER_SHEAR);

   m_pGrid = std::make_unique<CShearSteelGrid>();

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

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);

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

   DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,m_ShearData.ShearBarType,m_ShearData.ShearBarGrade);

   if (pDX->m_bSaveAndValidate)
   {
      // zone info from grid
      m_ShearData.ShearZones.clear();

      CShearZoneData2 lsi;
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

            if (IsEqual(lsi.ZoneLength, lsi.BarSpacing, zone_bar_spacing_tolerance))
            {
               // sometimes zone length and bar spacing are the same, but come out a little differently numerically
               // becase zone length is in feet and bar spacing in inches. For example, zone of 0.20833 ft with spacing of 2.5" are
               // the same but numerically the bar spacing is greater than the zone length. When this situation occurs,
               // force the bar spacing to match the zone length
               lsi.BarSpacing = lsi.ZoneLength;
            }

            // make sure stirrup spacing is greater than zone length
            if (zn+1 < nrows)
            {
               if (::IsLE(lsi.ZoneLength,0.0))
               {
                  CString msg;
                  msg.Format(_T("Zone length must be greater than zero in Shear Zone %d"),zn+1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            if ((matRebar::bsNone != lsi.VertBarSize || matRebar::bsNone != lsi.ConfinementBarSize) && zn < nrows)
            {
               if (::IsLT(lsi.ZoneLength,lsi.BarSpacing))
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
      // grid
      CShearData2::ShearZoneVec vec;
      for (CShearData2::ShearZoneConstIterator it = m_ShearData.ShearZones.begin(); it!=m_ShearData.ShearZones.end(); it++)
      {
         // Copy all data then convert length values
         CShearZoneData2 inf(*it);
         if ( !IsEqual(inf.ZoneLength,Float64_Max) )
         {
            inf.ZoneLength = ::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
         }

         inf.BarSpacing     = ::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
         vec.push_back(inf);
      }
      m_pGrid->FillGrid(vec, m_ShearData.bAreZonesSymmetrical);

      //// can't delete strands at start
      //CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
      //ASSERT(pdel);
      //pdel->EnableWindow(FALSE);

   }
}


BEGIN_MESSAGE_MAP(CShearSteelPage, CPropertyPage)
	//{{AFX_MSG_MAP(CShearSteelPage)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertRow)
	ON_BN_CLICKED(IDC_SYMMETRICAL, OnClickedSymmetrical)
   ON_BN_CLICKED(IDC_RESTORE_DEFAULTS, OnRestoreDefaults)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ADDITIONAL_INTERFACE_SHEAR_BARS, &CShearSteelPage::OnBnClickedAdditionalInterfaceShearBars)
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
   m_pGrid->SubclassDlgItem(IDC_SHEAR_GRID, this);
   m_pGrid->CustomInit();

   CEAFDocument* pEAFDoc = EAFGetDocument();
   bool bFilterBySpec = true;
   if (pEAFDoc->IsKindOf(RUNTIME_CLASS(CLibraryEditorDoc)))
   {
      bFilterBySpec = false;
   }

   FillBarComboBox((CComboBox*)GetDlgItem(IDC_SPLITTING_BAR_SIZE));
   FillBarComboBox((CComboBox*)GetDlgItem(IDC_CONFINE_BAR_SIZE));

   GetDlgItem(IDC_RESTORE_DEFAULTS)->ShowWindow(m_bAllowRestoreDefaults ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_SEED_VALUE_NOTE)->ShowWindow(m_bAllowRestoreDefaults ? SW_HIDE : SW_SHOW);

   CString strSymmetric, strEnd;
   GetLastZoneName(strSymmetric, strEnd);
   CString strLabel;
   strLabel.Format(_T("Make zones symmetric about %s"), strSymmetric);
   GetDlgItem(IDC_SYMMETRICAL)->SetWindowText(strLabel);


   CPropertyPage::OnInitDialog();

   CWnd* pWnd = GetParent();
   CShearSteelPageParent* pParent = dynamic_cast<CShearSteelPageParent*>(pWnd);
   if (pParent && !pParent->HasDeck())
   {
      m_pGrid->HideCols(5, 5);
      GetDlgItem(IDC_ADDITIONAL_INTERFACE_SHEAR_BARS)->ShowWindow(SW_HIDE);
   }

   if (pParent)
   {
      GetDlgItem(IDC_ROUGHENED)->SetWindowText(pParent->GetIntentionalRougheningPrompt());
   }


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


void CShearSteelPage::OnClickedSymmetrical()
{
   m_pGrid->SetSymmetry(IsDlgButtonChecked(IDC_SYMMETRICAL) == BST_CHECKED ? true : false);
}

void CShearSteelPage::GetRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade)
{
   m_cbRebar.GetMaterial(pType,pGrade);
}

BOOL CShearSteelPage::OnSetActive() 
{
	BOOL val = CPropertyPage::OnSetActive();

   m_pGrid->SelectRange(CGXRange().SetTable(), FALSE);

   return val;
}

UINT CShearSteelPage::GetHelpID()
{
   return IDH_GIRDER_TRANSVERSE_REINFORCEMENT;
}

void CShearSteelPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), GetHelpID() );
}

void CShearSteelPage::FillBarComboBox(CComboBox* pcbRebar)
{
   int idx = pcbRebar->AddString(_T("None"));
   pcbRebar->SetItemData(idx,(DWORD_PTR)matRebar::bsNone);
   lrfdRebarIter rebarIter(m_ShearData.ShearBarType,m_ShearData.ShearBarGrade,true);
   for ( rebarIter.Begin(); rebarIter; rebarIter.Next() )
   {
      const matRebar* pRebar = rebarIter.GetCurrentRebar();
      idx = pcbRebar->AddString(pRebar->GetName().c_str());
      pcbRebar->SetItemData(idx,(DWORD_PTR)pRebar->GetSize());
   }
}


void CShearSteelPage::OnRestoreDefaults()
{
    DoRestoreDefaults();

   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CShearSteelPage::DoRestoreDefaults()
{
    ATLASSERT(false); // library should never want to do this
}

void CShearSteelPage::GetLastZoneName(CString& strSymmetric, CString& strEnd)
{
   strSymmetric = _T("mid-span");
   strEnd = _T("girder end");
}

void CShearSteelPage::OnBnClickedAdditionalInterfaceShearBars()
{
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      CAdditionalInterfaceShearBarDlg dlg(this);

      GetRebarMaterial(&dlg.m_RebarType, &dlg.m_RebarGrade);

      dlg.m_bAreZonesSymmetrical = IsDlgButtonChecked(IDC_SYMMETRICAL) == BST_CHECKED ? true : false;
      dlg.m_HorizontalInterfaceZones = m_ShearData.HorizontalInterfaceZones;
      if (dlg.DoModal() == IDOK)
      {
         m_ShearData.HorizontalInterfaceZones = dlg.m_HorizontalInterfaceZones;
      }
   }
}
