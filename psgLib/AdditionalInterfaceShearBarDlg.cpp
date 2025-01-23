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
// AdditionalInterfaceShearBarDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AdditionalInterfaceShearBarDlg.h"
#include <psglib\ShearSteelPage.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>


// CAdditionalInterfaceShearBarDlg dialog

IMPLEMENT_DYNAMIC(CAdditionalInterfaceShearBarDlg, CDialog)

CAdditionalInterfaceShearBarDlg::CAdditionalInterfaceShearBarDlg(CShearSteelPage* pParent /*=NULL*/)
	: CDialog(IDD_EDIT_ADDITIONAL_INTERFACE_STEEL, pParent)
{
   m_pParent = pParent;
   m_pHorizGrid = std::make_unique<CHorizShearGrid>();
}

CAdditionalInterfaceShearBarDlg::~CAdditionalInterfaceShearBarDlg()
{
}

void CAdditionalInterfaceShearBarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   // Horiz shear Grid
   DDV_GXGridWnd(pDX, m_pHorizGrid.get());


   if (pDX->m_bSaveAndValidate)
   {
      // zone info from grid
      m_HorizontalInterfaceZones.clear();

      CHorizontalInterfaceZoneData lsi;
      ROWCOL nrows = m_pHorizGrid->GetRowCount();
      Uint32 zn = 0;
      for (ROWCOL i = 1; i <= nrows; i++)
      {
         if (m_pHorizGrid->GetRowData(i, nrows, &lsi))
         {
            lsi.ZoneNum = zn;
            if (!IsEqual(lsi.ZoneLength, Float64_Max))
            {
               lsi.ZoneLength = WBFL::Units::ConvertToSysUnits(lsi.ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
            }
            lsi.BarSpacing = WBFL::Units::ConvertToSysUnits(lsi.BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);

            // make sure stirrup spacing is greater than zone length
            if (zn + 1 < nrows)
            {
               if (lsi.ZoneLength <= 0.00001)
               {
                  CString msg;
                  msg.Format(_T("Zone length must be greater than zero in Horiz Interface Shear Zone %d"), zn + 1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            if (WBFL::Materials::Rebar::Size::bsNone != lsi.BarSize && zn < nrows)
            {
               if (lsi.ZoneLength<lsi.BarSpacing)
               {
                  CString msg;
                  msg.Format(_T("Bar spacing must be less than zone length in Horiz Interface Shear Zone %d"), zn + 1);
                  AfxMessageBox(msg);
                  pDX->Fail();
               }
            }

            // make sure stirrup spacing is >0 if stirrups or confinement bars exist
            if ((WBFL::Materials::Rebar::Size::bsNone != lsi.BarSize) && lsi.BarSpacing <= 0.0)
            {
               CString msg;
               msg.Format(_T("Bar spacing must be greater than zero if stirrups exist in Horiz Interface Shear Zone %d"), zn + 1);
               AfxMessageBox(msg);
               pDX->Fail();
            }

            m_HorizontalInterfaceZones.push_back(lsi);
            zn++;
         }
      }
   }
   else
   {
      CShearData2::HorizontalInterfaceZoneVec vec;
      for (CShearData2::HorizontalInterfaceZoneConstIterator it = m_HorizontalInterfaceZones.begin(); it != m_HorizontalInterfaceZones.end(); it++)
      {
         // Copy all data then convert length values
         CHorizontalInterfaceZoneData inf(*it);
         if (!IsEqual(inf.ZoneLength, Float64_Max))
         {
            inf.ZoneLength = WBFL::Units::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->XSectionDim.UnitOfMeasure);
         }

         inf.BarSpacing = WBFL::Units::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
         vec.push_back(inf);
      }
      m_pHorizGrid->FillGrid(vec, m_bAreZonesSymmetrical);

   }
}


BEGIN_MESSAGE_MAP(CAdditionalInterfaceShearBarDlg, CDialog)
   ON_BN_CLICKED(IDC_REMOVEHORIZROWS, OnRemoveHorizRows)
   ON_BN_CLICKED(IDC_INSERTHORIZROW, OnInsertHorizRow)
   ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CAdditionalInterfaceShearBarDlg message handlers

BOOL CAdditionalInterfaceShearBarDlg::OnInitDialog()
{
   m_pHorizGrid->SubclassDlgItem(IDC_HORIZ_GRID, this);
   m_pHorizGrid->CustomInit();

   CDialog::OnInitDialog();

   CShearSteelPageParent* pParent = dynamic_cast<CShearSteelPageParent*>(m_pParent);
   if (pParent && !pParent->HasDeck())
   {
      m_pHorizGrid->EnableWindow(FALSE);
      GetDlgItem(IDC_INSERTHORIZROW)->EnableWindow(FALSE);
      GetDlgItem(IDC_REMOVEHORIZROWS)->EnableWindow(FALSE);
   }

   if (m_HorizontalInterfaceZones.size() == 0)
   {
      // if there isn't any data, disable the delete button
      OnEnableHorizDelete(FALSE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CAdditionalInterfaceShearBarDlg::GetLastZoneName(CString& strSymmetric, CString& strEnd)
{
   m_pParent->GetLastZoneName(strSymmetric, strEnd);
}

void CAdditionalInterfaceShearBarDlg::OnEnableHorizDelete(BOOL bEnable)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEHORIZROWS);
   ASSERT(pdel);
   pdel->EnableWindow(bEnable);
}

void CAdditionalInterfaceShearBarDlg::OnRemoveHorizRows()
{
   DoRemoveHorizRows();
}

void CAdditionalInterfaceShearBarDlg::DoRemoveHorizRows()
{
   m_pHorizGrid->DoRemoveRows();
}

void CAdditionalInterfaceShearBarDlg::OnInsertHorizRow()
{
   DoInsertHorizRow();
}

void CAdditionalInterfaceShearBarDlg::DoInsertHorizRow()
{
   m_pHorizGrid->InsertRow(false); // insert at top if no selection
}

void CAdditionalInterfaceShearBarDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_ADDITIONAL_INTERFACE_SHEAR_REBAR);
}
