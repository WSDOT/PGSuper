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

// ShearDesignPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ShearDesignPage.h"

#include "ShearBarsLegsGrid.h"
#include "GirderMainSheet.h"

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#include <system\tokenizer.h>
#include <MfcTools\CustomDDX.h>
#include <boost/algorithm/string.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CShearDesignPage dialog
static const Float64 SPACING_TOL=1.0e-6;

IMPLEMENT_DYNAMIC(CShearDesignPage, CPropertyPage)

CShearDesignPage::CShearDesignPage()
	: CPropertyPage(CShearDesignPage::IDD)
   , m_bExtendDeckBars(FALSE)
   , m_bBarsProvideConfinement(FALSE)
   , m_LongReinfShearMethod(0)
{
   m_pGrid = std::make_unique<CShearBarsLegsGrid>();
}

CShearDesignPage::~CShearDesignPage()
{
}

void CShearDesignPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // dad is a friend of the entry. use him to transfer data.
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   if (!pDX->m_bSaveAndValidate)
   {
      // load data from parent to this
      pDad->UploadShearDesignData(pDX);

      // Fill bars/legs grid
      m_pGrid->FillGrid(m_StirrupSizeBarComboColl);

      // Fill bar spacings
      sysNumericFormatTool ftool(sysNumericFormatTool::Automatic,pDisplayUnits->ComponentDim.Width,pDisplayUnits->ComponentDim.Precision);
      std::_tostringstream  os_spacings;
      std::vector<Float64>::size_type size = m_BarSpacings.size();
      for (std::vector<Float64>::size_type sf=0; sf<size; sf++)
      {
         Float32 conval = (Float32)::ConvertFromSysUnits(m_BarSpacings[sf], pDisplayUnits->ComponentDim.UnitOfMeasure);

         std::_tstring str(ftool.AsString(conval));
         boost::trim(str);

         std::_tstring::size_type tlc = str.find('.'); // Add ".0" if no decimal
         if (tlc==std::_tstring::npos)
         {
            str = str + std::_tstring(_T(".0"));
         }

         os_spacings << str;

         if (sf<size-1)
            os_spacings <<", ";
      }

      CString strSpacings(os_spacings.str().c_str());

      GetDlgItem(IDC_EDIT_BAR_SPACINGS)->SetWindowText(strSpacings);
      GetDlgItem(IDC_EDIT_BAR_SPACINGS_UNIT)->SetWindowText(pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
   }

   // DDX's
   DDX_UnitValueAndTag(pDX, IDC_MAX_SPACING_CHG, IDC_MAX_SPACING_CHG_UNIT, m_MaxStirrupSpacingChange, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MAX_SPACING_CHG, m_MaxStirrupSpacingChange, pDisplayUnits->ComponentDim);

   DDX_Percentage(pDX, IDC_MAX_SHEARCAP_CHG, m_MaxShearCapChange);
   DDV_Range( pDX, mfcDDV::LE,mfcDDV::GE,m_MaxShearCapChange,0.0,100.0);

   DDX_Text(pDX, IDC_MIN_ZONELEN_BARS, m_MinZoneLengthBars);
   DDV_GreaterThanZero(pDX, IDC_MIN_ZONELEN_BARS, m_MinZoneLengthBars);

   DDX_UnitValueAndTag(pDX, IDC_MIN_ZONELEN_DIST, IDC_MIN_ZONELEN_DIST_UNIT, m_MinZoneLengthDist, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_MIN_ZONELEN_DIST, m_MinZoneLengthDist, pDisplayUnits->ComponentDim);

   DDX_Check(pDX, IDC_EXTEND_DECK_BARS, m_bExtendDeckBars);
   DDX_Check(pDX, IDC_BARS_PROVIDE_CONFINEMENT, m_bBarsProvideConfinement);

   DDX_CBIndex(pDX, IDC_LONG_REINF_SHEAR_METHOD, m_LongReinfShearMethod);

   if (pDX->m_bSaveAndValidate)
   {
      // Get data from grid - remove any duplicates
      m_StirrupSizeBarComboColl = m_pGrid->GetGridData();

      if (m_StirrupSizeBarComboColl.empty())
      {
         ::AfxMessageBox(_T("Error - There must be at least one Bar Sizes/Legs row in grid"),MB_OK | MB_ICONWARNING);
         pDX->PrepareCtrl(IDC_BARS_LEGS_GRID);
         pDX->Fail();
      }

      // Check for duplicates using brute force
      StirrupSizeBarComboConstIter its=m_StirrupSizeBarComboColl.begin();
      while(its!=m_StirrupSizeBarComboColl.end()) 
      {
         int row=1;
         bool first = true;
         StirrupSizeBarComboConstIter itcb=m_StirrupSizeBarComboColl.begin();
         while(itcb!=m_StirrupSizeBarComboColl.end())
         {
            if (*itcb == *its)
            {
               if (first)
               {
                  first = false;
               }
               else
               {
                  // Could remove second and any other dup's quietly, but a better approach is to warn user
                  // about bad input
                  // itcb = m_StirrupSizeBarComboColl.erase(itcb);
                  CString msg;
                  msg.Format(_T("Row %d in grid contains a duplicate Bar Sizes/Legs entry. All entries must be unique"),row);
                  ::AfxMessageBox(msg, MB_OK | MB_ICONWARNING);
                  pDX->PrepareCtrl(IDC_BARS_LEGS_GRID);
                  pDX->Fail();
               }
            }

            itcb++;
            row++;
         }

         its++;
      }

      // Spacings
      CString str_spacings;
      GetDlgItem(IDC_EDIT_BAR_SPACINGS)->GetWindowText(str_spacings);

      LPCTSTR delims[] = {_T(","),_T(" "), 0};
      sysTokenizer tokizerd(delims);
      tokizerd.push_back(str_spacings);

      sysTokenizer::size_type nps = tokizerd.size();

      if (nps==0)
      {
         ::AfxMessageBox(_T("Error - There must be at least one Bar Spacing value for design algorithm. Please enter a comma-delimited list of spacings."),MB_OK | MB_ICONWARNING);
         pDX->PrepareCtrl(IDC_EDIT_BAR_SPACINGS);
         pDX->Fail();
      }
      else
      {
         m_BarSpacings.clear();

         for (sysTokenizer::size_type ips=0; ips<nps; ips++)
         {
            Float64 db;
            if (!sysTokenizer::ParseDouble(tokizerd[ips].c_str(), &db))
            {
               CString msg;
               msg.Format(_T("Error - Item %d in the list of Bar Spacing values is not a number. Please enter a comma-delimited list of spacings."), ips+1);
               ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
               pDX->PrepareCtrl(IDC_EDIT_BAR_SPACINGS);
               pDX->Fail();
            }
            else
            {
               db = ::ConvertToSysUnits(db, pDisplayUnits->ComponentDim.UnitOfMeasure);

               if(db<=0.0)
               {
                  ::AfxMessageBox(_T("Error - Bar spacings must be greater than zero. Please enter a comma-delimited list of spacings."),MB_OK | MB_ICONWARNING);
                  pDX->PrepareCtrl(IDC_EDIT_BAR_SPACINGS);
                  pDX->Fail();
               }

               m_BarSpacings.push_back(db);
            }
         }

         // Sort spacing list and remove duplicates
         std::sort(m_BarSpacings.begin(), m_BarSpacings.end());
         std::unique(m_BarSpacings.begin(), m_BarSpacings.end());

         // Make sure design algorithm can step from one spacing to the next
         std::vector<Float64>::iterator ittwo=m_BarSpacings.begin();
         std::vector<Float64>::iterator itone(ittwo++);
         while(ittwo!=m_BarSpacings.end())
         {
            Float64 val_one = *itone++;
            Float64 val_two = *ittwo++;
            if (val_one*(1+m_MaxShearCapChange) < val_two-SPACING_TOL || val_one+m_MaxStirrupSpacingChange < val_two-SPACING_TOL)
            {
               CString msg(_T("Error - Allowable changes in stirrup bar spacings are such that the design algoritm cannot select a next spacing. You must tighten spacings, or loosen spacing change requirements"));
               ::AfxMessageBox(msg,MB_OK | MB_ICONWARNING);
               pDX->PrepareCtrl(IDC_EDIT_BAR_SPACINGS);
               pDX->Fail();
            }
         }
      }


      // Send data to parent
      pDad->DownloadShearDesignData(pDX);
   }
}


BEGIN_MESSAGE_MAP(CShearDesignPage, CPropertyPage)
   ON_BN_CLICKED(IDC_REMOVEROWS, &CShearDesignPage::OnRemoveRows)
   ON_BN_CLICKED(IDC_INSERTROW, &CShearDesignPage::OnInsertRow)
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()


// CShearDesignPage message handlers
void CShearDesignPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_SHEAR_DESIGN );
}

BOOL CShearDesignPage::OnInitDialog()
{
	m_pGrid->SubclassDlgItem(IDC_BARS_LEGS_GRID, this);
   m_pGrid->CustomInit();

   CPropertyPage::OnInitDialog();

   // disable delete button on start
   OnEnableDelete(false);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void CShearDesignPage::OnRemoveRows() 
{
   DoRemoveRows();
}

void CShearDesignPage::DoRemoveRows()
{
	m_pGrid->DoRemoveRows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CShearDesignPage::OnInsertRow() 
{
   DoInsertRow();
}

void CShearDesignPage::DoInsertRow() 
{
	m_pGrid->InsertRow(false); // insert at top if no selection
}

void CShearDesignPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}
