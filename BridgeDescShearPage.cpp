///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescShearPage.h"
#include "GirderDescDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <MfcTools\CustomDDX.h>

#include "HtmlHelp\HelpTopics.hh"

#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescShearPage property page

IMPLEMENT_DYNCREATE(CGirderDescShearPage, CPropertyPage)

CGirderDescShearPage::CGirderDescShearPage() : CPropertyPage(CGirderDescShearPage::IDD,IDS_GIRDER_SHEAR)
{
	//{{AFX_DATA_INIT(CGirderDescShearPage)
	//}}AFX_DATA_INIT
}

CGirderDescShearPage::~CGirderDescShearPage()
{
}

void CGirderDescShearPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescShearPage)
	DDX_Control(pDX, IDC_TF_BAR_SIZE, m_TfBarSize);
	DDX_Control(pDX, IDC_BAR_SIZE, m_BarSize);
	DDX_Control(pDX, IDC_LAST_ZONE, m_LastZone);
	//}}AFX_DATA_MAP

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   DDX_UnitValueAndTag(pDX, IDC_TF_SPACING, IDC_TF_SPACING_UNITS, m_ShearData.TopFlangeBarSpacing, pDisplayUnits->GetComponentDimUnit());
   DDV_UnitValueZeroOrMore(pDX, IDC_TF_SPACING, m_ShearData.TopFlangeBarSpacing, pDisplayUnits->GetComponentDimUnit() );

	DDV_GXGridWnd(pDX, &m_Grid);

   DDX_CBItemData(pDX,IDC_TF_BAR_SIZE,m_ShearData.TopFlangeBarSize);
   DDX_CBItemData(pDX,IDC_BAR_SIZE,m_ShearData.ConfinementBarSize);

   if (pDX->m_bSaveAndValidate)
   {
      int idx;
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);
      pParent->GetStirrupMaterial(idx,m_ShearData.ShearBarType,m_ShearData.ShearBarGrade);


      // last confinement zone
      int iz = m_LastZone.GetCurSel();
      if (iz==CB_ERR)
         m_ShearData.NumConfinementZones = 0;
      else
         m_ShearData.NumConfinementZones = iz;


      // check spacing
      if (m_ShearData.TopFlangeBarSize != matRebar::bsNone && m_ShearData.TopFlangeBarSpacing<=0.0)
      {
         CString msg("Top Flange Bar Spacing must be greater than zero");
         AfxMessageBox(msg);
         pDX->Fail();
      }

      // zone info from grid
      m_ShearData.ShearZones.clear();

      GirderLibraryEntry::ShearZoneInfo lsi;
      ROWCOL nrows = m_Grid.GetRowCount();
      Uint32 zn = 1;
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (m_Grid.GetRowData(i,&lsi))
         {
            lsi.ZoneLength = ::ConvertToSysUnits(lsi.ZoneLength, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
            lsi.StirrupSpacing = ::ConvertToSysUnits(lsi.StirrupSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

            // make sure stirrup spacing is greater than zone length
            if ((matRebar::bsNone != lsi.VertBarSize || matRebar::bsNone != lsi.HorzBarSize) && zn < nrows)
            {
               if (lsi.ZoneLength<lsi.StirrupSpacing)
               {
                  CString msg;
                  msg.Format(_T("Bar spacing must be less than zone length in Shear Zone %d"),zn);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            // make sure stirrup spacing is >0 if stirrups or confinement bars exist
            if ( (matRebar::bsNone != lsi.VertBarSize || matRebar::bsNone != lsi.HorzBarSize) && lsi.StirrupSpacing<=0.0)
            {
               CString msg;
               msg.Format(_T("Bar spacing must be greater than zero if stirrups exist in Shear Zone %d"),zn);
               AfxMessageBox(msg);
               pDX->Fail();
            }
            
            // must have stirrup spacing >0 in confinement zone
            if (zn-1<m_ShearData.NumConfinementZones && lsi.StirrupSpacing<=0.0)
            {
               CString msg;
               msg.Format(_T("Bar spacing must be >0.0 in Confinment Zone %d"),zn);
               AfxMessageBox(msg);
               pDX->Fail();
            }

            CShearZoneData sz;
            sz.ZoneNum    = zn;
            sz.ZoneLength = lsi.ZoneLength;
            sz.BarSpacing = lsi.StirrupSpacing;
            sz.VertBarSize    = lsi.VertBarSize;
            sz.nVertBars      = lsi.nVertBars;
            sz.HorzBarSize    = lsi.HorzBarSize;
            sz.nHorzBars      = lsi.nHorzBars;

            m_ShearData.ShearZones.push_back(sz);
            zn++;
         }
      }
      
      // check that last confiment zone is within bounds
      ZoneIndexType siz = m_ShearData.ShearZones.size();
      ASSERT(m_ShearData.NumConfinementZones<=siz);
   }
   else
   {
      // fill er up
      int idx = pParent->GetStirrupMaterialIndex(m_ShearData.ShearBarType,m_ShearData.ShearBarGrade);
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);

      // grid
      GirderLibraryEntry::ShearZoneInfoVec vec;
      for (CShearData::ShearZoneConstIterator it = m_ShearData.ShearZones.begin(); it!=m_ShearData.ShearZones.end(); it++)
      {
         GirderLibraryEntry::ShearZoneInfo inf;
         inf.ZoneLength     = ::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
         inf.StirrupSpacing = ::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
         inf.VertBarSize    = (*it).VertBarSize;
         inf.nVertBars      = (*it).nVertBars;
         inf.HorzBarSize    = (*it).HorzBarSize;
         inf.nHorzBars      = (*it).nHorzBars;
         vec.push_back(inf);
      }
      m_Grid.FillGrid(vec);

      // last confinement zone
      FillLastZone(vec.size());
      ZoneIndexType sel = m_ShearData.NumConfinementZones;
      if (sel <= vec.size())
      {
         m_LastZone.SetCurSel((int)sel);
      }
      else
      {
         ASSERT(0); // Shear zone from data file out of range
         m_LastZone.SetCurSel(0);
      }

      // can't delete strands at start
      CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
      ASSERT(pdel);
      pdel->EnableWindow(FALSE);

   }

   DDX_Check_Bool(pDX,IDC_STIRRUPS_ENGAGE_DECK,m_ShearData.bDoStirrupsEngageDeck);
   DDX_Check_Bool(pDX,IDC_ROUGHENED,           m_ShearData.bIsRoughenedSurface);
}


BEGIN_MESSAGE_MAP(CGirderDescShearPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescShearPage)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertRow)
	ON_BN_CLICKED(IDC_RESTORE_DEFAULTS, OnRestoreDefaults)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescShearPage message handlers

void CGirderDescShearPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderDescShearPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_SHEAR_GRID, this);
   m_Grid.CustomInit();

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   pParent->FillMaterialComboBox(pc);

   FillBarComboBox((CComboBox*)GetDlgItem(IDC_TF_BAR_SIZE));
   FillBarComboBox((CComboBox*)GetDlgItem(IDC_BAR_SIZE));

	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescShearPage::OnRemoveRows() 
{
   DoRemoveRows();
}

void CGirderDescShearPage::DoRemoveRows()
{
	m_Grid.DoRemoveRows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);

   // update list control
   Uint32 lz = m_LastZone.GetCurSel();
   ROWCOL nrows = m_Grid.GetRowCount();
   FillLastZone(nrows);
   if (lz==CB_ERR)
      m_LastZone.SetCurSel(0);
   else if (lz>nrows)
      m_LastZone.SetCurSel(nrows);
   else
      m_LastZone.SetCurSel(lz);
}

void CGirderDescShearPage::OnInsertRow() 
{
   DoInsertRow();
}

void CGirderDescShearPage::DoInsertRow() 
{
	m_Grid.InsertRow(false); // insert at top if no selection

   // update list control
   int lz = m_LastZone.GetCurSel();
   ROWCOL nrows = m_Grid.GetRowCount();
   FillLastZone(nrows);
   if (lz==CB_ERR)
      m_LastZone.SetCurSel(0);
   else
      m_LastZone.SetCurSel(lz);
}

void CGirderDescShearPage::FillLastZone(ZoneIndexType siz)
{
   CString tmp;
   m_LastZone.ResetContent();
   m_LastZone.AddString(_T("None"));
   for (ZoneIndexType i=1; i<=siz; i++)
   {
      tmp.Format(_T("Zone %d"),i);
      m_LastZone.AddString(tmp);
   }
}

void CGirderDescShearPage::RestoreToLibraryDefaults()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // get shear information from library
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGird = pLib->GetGirderEntry( m_CurGrdName.c_str());
   ASSERT(pGird!=0);

   // update data member
   m_ShearData.CopyGirderEntryData(*pGird);
}

void CGirderDescShearPage::OnRestoreDefaults() 
{
	RestoreToLibraryDefaults();
   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

BOOL CGirderDescShearPage::OnSetActive() 
{

	BOOL val = CPropertyPage::OnSetActive();

   m_Grid.SelectRange(CGXRange().SetTable(), FALSE);

   return val;
}

void CGirderDescShearPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_SHEARDESC );
}

void CGirderDescShearPage::FillBarComboBox(CComboBox* pCB)
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
}
