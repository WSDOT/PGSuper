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

// GirderMainSheet.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderMainSheet.h"
#include <MfcTools\CustomDDX.h>

#include <Units\sysUnits.h>
#include "..\htmlhelp\HelpTopics.hh"

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderMainSheet

IMPLEMENT_DYNAMIC(CGirderMainSheet, CPropertySheet)

CGirderMainSheet::CGirderMainSheet( GirderLibraryEntry& rentry, UINT nIDCaption, 
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_AllowEditing(allowEditing)
{
   Init();
}

CGirderMainSheet::CGirderMainSheet( GirderLibraryEntry& rentry, LPCTSTR pszCaption,
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_AllowEditing(allowEditing)
{
   Init();
}

CGirderMainSheet::~CGirderMainSheet()
{
}


BEGIN_MESSAGE_MAP(CGirderMainSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderMainSheet)
   ON_NOTIFY_REFLECT(PSN_APPLY, OnApply)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderMainSheet message handlers
void CGirderMainSheet::Init()
{
   // Turn on help for the property sheet
   m_psh.dwFlags                            |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_GirderDimensionsPage.m_psp.dwFlags     |= PSP_HASHELP;
   m_GirderHarpedStrandPage.m_psp.dwFlags   |= PSP_HASHELP;
   m_GirderStraightStrandPage.m_psp.dwFlags |= PSP_HASHELP;
   m_LongSteelPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_ShearSteelPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_HarpPointPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_DiaphragmPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_GirderDebondCriteriaPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_GirderDimensionsPage);
   AddPage(&m_GirderHarpedStrandPage);   // straight and harped strands
   AddPage(&m_GirderStraightStrandPage); // temporary strands
   AddPage(&m_GirderDebondCriteriaPage);
   AddPage(&m_HarpPointPage);
   AddPage(&m_LongSteelPage);
   AddPage(&m_ShearSteelPage);
   AddPage(&m_DiaphragmPage);
}

void CGirderMainSheet::ExchangeDimensionData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX,IDC_PT,m_Entry.m_bCanPostTension);

   if (pDX->m_bSaveAndValidate)
   {
	   DDX_Text(pDX, IDC_ENAME, m_Name);
      if (m_Name.IsEmpty())
      {
         AfxMessageBox("Girder Name cannot be blank");
         pDX->Fail();
      }
      m_Entry.SetName(m_Name);
   }
   else
   {
      m_Name = m_Entry.GetName().c_str();
	   DDX_Text(pDX, IDC_ENAME, m_Name);
   }

   bool bUnitsSI = (pApp->GetUnitsMode() == eafTypes::umSI);

   if ( pDX->m_bSaveAndValidate )
   {

      // Pull the values from the grid... Convert back to system units
      std::vector<std::string> names = m_Entry.m_pBeamFactory->GetDimensionNames();
      std::vector<const unitLength*> units = m_Entry.m_pBeamFactory->GetDimensionUnits(bUnitsSI);

      ATLASSERT(names.size() == units.size());

      std::vector<std::string>::iterator name_iter;
      std::vector<const unitLength*>::iterator unit_iter;
      long nRow = 1;
      for ( name_iter = names.begin(), unit_iter = units.begin(); 
            name_iter != names.end() || unit_iter != units.end();
            name_iter++, unit_iter++ )
      {
         CString strValue = m_GirderDimensionsPage.m_Grid.GetCellValue(nRow,1);
         Float64 value = atof(strValue);

         const unitLength* pUnit = *unit_iter;
         if ( pUnit != NULL )
            value = ::ConvertToSysUnits(value,*pUnit);

         m_Entry.SetDimension(*name_iter,value,true);
         nRow++;
      }

      std::string strErrMsg;
      bool bValid = m_Entry.m_pBeamFactory->ValidateDimensions(m_Entry.m_Dimensions,bUnitsSI,&strErrMsg);
      if ( !bValid )
      {
         AfxMessageBox( strErrMsg.c_str(), MB_ICONEXCLAMATION );
         pDX->Fail();
      }
   }
   else
   {
      // Fill up the grid
      std::vector<std::string> names = m_Entry.m_pBeamFactory->GetDimensionNames();
      std::vector<const unitLength*> units = m_Entry.m_pBeamFactory->GetDimensionUnits(bUnitsSI);

      ATLASSERT(names.size() == units.size());

      std::vector<std::string>::iterator name_iter;
      std::vector<const unitLength*>::iterator unit_iter;
      long nRow = 1;
      for ( name_iter = names.begin(), unit_iter = units.begin(); 
            name_iter != names.end() || unit_iter != units.end();
            name_iter++, unit_iter++ )
      {
         std::string name = *name_iter;

         Float64 value = m_Entry.GetDimension(name);

         const unitLength* pUnit = *unit_iter;
         if ( pUnit != NULL )
         {
            value = ::ConvertFromSysUnits(value,*pUnit);

            name += " (";
            name += pUnit->UnitTag();
            name += ")";
         }

         VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 0), name.c_str() ));
         VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 1), value ));
         nRow++;
      }

      m_GirderDimensionsPage.m_Grid.ResizeColWidthsToFit(CGXRange(0,0,m_GirderDimensionsPage.m_Grid.GetRowCount(),1));
   }
}

bool CGirderMainSheet::ExchangeTemporaryStrandData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // controls data
   // get strand locations from grids
   if (pDX->m_bSaveAndValidate)
   {
      // Temporary strands
      m_Entry.m_TemporaryStrands.clear();

      Float64 heightStart = m_Entry.GetBeamHeight(pgsTypes::metStart);
      Float64 heightEnd   = m_Entry.GetBeamHeight(pgsTypes::metEnd);

      ROWCOL nrows = m_GirderStraightStrandPage.m_TemporaryGrid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         Float64 x,y;
         if (m_GirderStraightStrandPage.m_TemporaryGrid.GetRowData(i,&x,&y))
         {
            // values are in display units - must convert to system
            x = ::ConvertToSysUnits(x, pDisplayUnits->ComponentDim.UnitOfMeasure);
            y = ::ConvertToSysUnits(y, pDisplayUnits->ComponentDim.UnitOfMeasure);

            GirderLibraryEntry::StraightStrandLocation strandLocation;
            strandLocation.m_Xstart = x;
            strandLocation.m_Ystart = heightStart - y;
            strandLocation.m_Xend   = x;
            strandLocation.m_Yend   = heightEnd   - y;

            m_Entry.m_TemporaryStrands.push_back(strandLocation);
         }
         else
         {
            CString msg;
            msg.Format("Error - Incomplete data in row %d",i);
            AfxMessageBox(msg);
		      pDX->PrepareCtrl(m_GirderStraightStrandPage.m_TemporaryGrid.GridWnd()->GetDlgCtrlID());
            return false;
         }
      }
   }
   return true;
}

void CGirderMainSheet::UploadTemporaryStrandData()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // upload Temporary strand data into grid - convert units first
   Float64 heightStart = m_Entry.GetBeamHeight(pgsTypes::metStart);

   CComPtr<IPoint2dCollection> new_temp_strands;
   new_temp_strands.CoCreateInstance(CLSID_Point2dCollection);

   GirderLibraryEntry::StraightStrandIterator iter;
   for ( iter = m_Entry.m_TemporaryStrands.begin(); iter != m_Entry.m_TemporaryStrands.end(); iter++ )
   {
      GirderLibraryEntry::StraightStrandLocation& strandLocation = *iter;
      Float64 x = ::ConvertFromSysUnits(strandLocation.m_Xstart, pDisplayUnits->ComponentDim.UnitOfMeasure);
      Float64 y = ::ConvertFromSysUnits(heightStart - strandLocation.m_Ystart, pDisplayUnits->ComponentDim.UnitOfMeasure);

      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(x,y);
      new_temp_strands->Add(p);
   }

   m_GirderStraightStrandPage.m_TemporaryGrid.FillGrid(new_temp_strands);
}

bool CGirderMainSheet::ExchangeStrandData(CDataExchange* pDX)
{
   // move data from grid back to library entry
   // controls data
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX,IDC_ALLOW_HP_ADJUST, m_Entry.m_HPAdjustment.m_AllowVertAdjustment );
   DDX_UnitValueAndTag(pDX, IDC_HP_INCREMENT, IDC_HP_INCREMENT_T, m_Entry.m_HPAdjustment.m_StrandIncrement, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_HP_INCREMENT,m_Entry.m_HPAdjustment.m_StrandIncrement, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_HP_LSL, IDC_HP_LSL_T, m_Entry.m_HPAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_HP_LSL,m_Entry.m_HPAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_HP_LSL, (int&)m_Entry.m_HPAdjustment.m_BottomFace);
   DDX_UnitValueAndTag(pDX, IDC_HP_USL, IDC_HP_USL_T, m_Entry.m_HPAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_HP_USL,m_Entry.m_HPAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_HP_USL, (int&)m_Entry.m_HPAdjustment.m_TopFace);
   
   DDX_Check_Bool(pDX,IDC_ALLOW_END_ADJUST, m_Entry.m_EndAdjustment.m_AllowVertAdjustment );
   DDX_UnitValueAndTag(pDX, IDC_END_INCREMENT, IDC_END_INCREMENT_T, m_Entry.m_EndAdjustment.m_StrandIncrement, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_INCREMENT,m_Entry.m_EndAdjustment.m_StrandIncrement, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX, IDC_END_LSL, IDC_END_LSL_T, m_Entry.m_EndAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_LSL,m_Entry.m_EndAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_END_LSL, (int&)m_Entry.m_EndAdjustment.m_BottomFace);
   DDX_UnitValueAndTag(pDX, IDC_END_USL, IDC_END_USL_T, m_Entry.m_EndAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_USL,m_Entry.m_EndAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_END_USL, (int&)m_Entry.m_EndAdjustment.m_TopFace);

   DDX_Check_Bool(pDX,IDC_ODD_STRANDS, m_Entry.m_bOddNumberOfHarpedStrands );
   DDX_Check_Bool(pDX,IDC_USE_DIFF_GRID, m_Entry.m_bUseDifferentHarpedGridAtEnds);

   // get strand locations from grid and put them in library entry
   if (pDX->m_bSaveAndValidate)
   {
      // lots of points to create - might as well be efficient
      CComPtr<IGeomUtil> geom_util;
      geom_util.CoCreateInstance(CLSID_GeomUtil);
      CComPtr<IPoint2dFactory> pnt_factory;
      geom_util->get_Point2dFactory(&pnt_factory);

      Float64 heightStart = m_Entry.GetBeamHeight(pgsTypes::metStart);
      Float64 heightEnd   = m_Entry.GetBeamHeight(pgsTypes::metEnd);
    
      m_Entry.m_StraightStrands.clear();
      m_Entry.m_HarpedStrands.clear();
      m_Entry.m_GlobalStrandOrder.clear();

      // grab a reference to grid's internal data to save typing
      CGirderGlobalStrandGrid::EntryCollectionType& grid_collection =  m_GirderHarpedStrandPage.m_MainGrid.m_Entries;

      long g_cnt = grid_collection.size();
      Uint16 num_straight=0;
      Uint16 num_harped=0;
      for (long g_idx=0; g_idx<g_cnt; g_idx++)
      {
         const CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry = grid_collection[g_idx];

         if (entry.m_Type == GirderLibraryEntry::stStraight)
         {
            // straight strands are easy, just convert units and add point
            Float64 x = ::ConvertToSysUnits(entry.m_X, pDisplayUnits->ComponentDim.UnitOfMeasure);
            Float64 y = ::ConvertToSysUnits(entry.m_Y, pDisplayUnits->ComponentDim.UnitOfMeasure);

            m_Entry.m_StraightStrands.push_back(GirderLibraryEntry::StraightStrandLocation(x,y,entry.m_CanDebond));

            m_Entry.m_GlobalStrandOrder.push_back(GirderLibraryEntry::GlobalStrand(GirderLibraryEntry::stStraight, num_straight++));
         }
         else if (entry.m_Type == GirderLibraryEntry::stHarped)
         {
            // harped at HP
            Float64 x = ::ConvertToSysUnits(entry.m_X, pDisplayUnits->ComponentDim.UnitOfMeasure );
            Float64 y = ::ConvertToSysUnits(entry.m_Y, pDisplayUnits->ComponentDim.UnitOfMeasure );
            Float64 start_x = x;
            Float64 start_y = y;
            Float64 end_x   = x;  // assume same location at end
            Float64 end_y   = y;

            // harped at ends
            if (m_Entry.m_bUseDifferentHarpedGridAtEnds)
            {
               // Input is from top down, but it needs to be stored from bottom up
               start_x = ::ConvertToSysUnits(entry.m_Hend_X, pDisplayUnits->ComponentDim.UnitOfMeasure );
               start_y = heightStart - ::ConvertToSysUnits(entry.m_Hend_Y, pDisplayUnits->ComponentDim.UnitOfMeasure );

               end_x = ::ConvertToSysUnits(entry.m_Hend_X, pDisplayUnits->ComponentDim.UnitOfMeasure );
               end_y = heightEnd - ::ConvertToSysUnits(entry.m_Hend_Y, pDisplayUnits->ComponentDim.UnitOfMeasure );
            }
            
            m_Entry.m_HarpedStrands.push_back(GirderLibraryEntry::HarpedStrandLocation(start_x,start_y,x,y,end_x,end_y));

            m_Entry.m_GlobalStrandOrder.push_back(GirderLibraryEntry::GlobalStrand(GirderLibraryEntry::stHarped, num_harped++));
         }
         else
            ATLASSERT(0);
      }
   }

   return true;
}

void CGirderMainSheet::UploadStrandData()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // upload harped strand data into grid - convert units first
   Float64 heightStart = m_Entry.GetBeamHeight(pgsTypes::metStart);
   CGirderGlobalStrandGrid::EntryCollectionType grid_collection;

   StrandIndexType num_global = m_Entry.GetMaxGlobalStrands();
   for (StrandIndexType idx=0; idx<num_global; idx++)
   {
      StrandIndexType strand_idx;
      GirderLibraryEntry::psStrandType strand_type;
      m_Entry.GetGlobalStrandAtFill(idx, &strand_type, &strand_idx);

      Float64 start_x,start_y,x,y,end_x,end_y;
      bool can_debond(false);
      if (strand_type == GirderLibraryEntry::stStraight)
      {
         m_Entry.GetStraightStrandCoordinates(strand_idx, &start_x, &start_y, &end_x, &end_y, &can_debond);
         x = start_x;
         y = start_y;
      }
      else if (strand_type == GirderLibraryEntry::stHarped)
      {
         m_Entry.GetHarpedStrandCoordinates(strand_idx, &start_x, &start_y, &x, &y, &end_x, &end_y);
      }
      else
         ATLASSERT(0);

      CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;
      entry.m_X =  ::ConvertFromSysUnits(x, pDisplayUnits->ComponentDim.UnitOfMeasure );
      entry.m_Y =  ::ConvertFromSysUnits(y, pDisplayUnits->ComponentDim.UnitOfMeasure );
      entry.m_CanDebond = can_debond;
      entry.m_Type = strand_type;

      // adjust end strands to be measured from top
      entry.m_Hend_X = ::ConvertFromSysUnits(start_x, pDisplayUnits->ComponentDim.UnitOfMeasure );
      entry.m_Hend_Y = ::ConvertFromSysUnits(heightStart-start_y, pDisplayUnits->ComponentDim.UnitOfMeasure );

      grid_collection.push_back(entry);
   }

   // we have data structure built - now fill grid
   m_GirderHarpedStrandPage.m_MainGrid.FillGrid(grid_collection);
}

void CGirderMainSheet::UploadLongitudinalData()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   GirderLibraryEntry::LongSteelInfoVec vec = m_Entry.GetLongSteelInfo();

   // convert units
   for (GirderLibraryEntry::LongSteelInfoVec::iterator it = vec.begin(); it!=vec.end(); it++)
   {
      (*it).Cover = ::ConvertFromSysUnits((*it).Cover, pDisplayUnits->ComponentDim.UnitOfMeasure );
      (*it).BarSpacing = ::ConvertFromSysUnits((*it).BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure );
   }
   m_LongSteelPage.m_Grid.FillGrid(vec);
}

void CGirderMainSheet::ExchangeLongitudinalData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // longitudinal steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      GirderLibraryEntry::LongSteelInfoVec vec;
      GirderLibraryEntry::LongSteelInfo lsi;
      ROWCOL nrows = m_LongSteelPage.m_Grid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (m_LongSteelPage.m_Grid.GetRowData(i,&lsi))
         {
            // values are in display units - must convert to system
            lsi.Cover      = ::ConvertToSysUnits(lsi.Cover, pDisplayUnits->ComponentDim.UnitOfMeasure );
            lsi.BarSpacing = ::ConvertToSysUnits(lsi.BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure );
            vec.push_back(lsi);
         }
      }
      m_Entry.SetLongSteelInfo(vec);
   }
}


void CGirderMainSheet::ExchangeTransverseData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (!pDX->m_bSaveAndValidate)
   {
      // bar spacing in top flange
      m_TfBarSpacing= m_Entry.GetTopFlangeShearBarSpacing();
   }

   DDX_UnitValueAndTag(pDX, IDC_TF_SPACING, IDC_TF_SPACING_UNITS, m_TfBarSpacing, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_TF_SPACING,m_TfBarSpacing, pDisplayUnits->ComponentDim);

   // get shear steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      GirderLibraryEntry::ShearZoneInfoVec vec;
      GirderLibraryEntry::ShearZoneInfo lsi;
      ROWCOL nrows = m_ShearSteelPage.m_Grid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (m_ShearSteelPage.m_Grid.GetRowData(i,&lsi))
         {
            // values are in display units - must convert to system
            lsi.ZoneLength     = ::ConvertToSysUnits(lsi.ZoneLength, pDisplayUnits->SpanLength.UnitOfMeasure);
            lsi.StirrupSpacing = ::ConvertToSysUnits(lsi.StirrupSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
            vec.push_back(lsi);
         }
      }
      m_Entry.SetShearZoneInfo(vec);

      // shear bar material
      CString mn;
      m_ShearSteelPage.m_MaterialName.GetWindowText(mn);
      m_Entry.SetShearSteelMaterial((LPCTSTR)mn);

      // bar size in confinement zone.
      CString bs;
      m_ShearSteelPage.m_BarSize.GetWindowText(bs);
      bs.TrimLeft();
      if (bs=="none")
         m_Entry.SetShearSteelBarSize(0);
      else
      {
         int l = bs.GetLength();
         CString s2 = bs.Right(l-1);
         int i = atoi(s2);
         if (bs.IsEmpty() || (i==0&&bs[0]!='0'))
         {
            ASSERT(0);
            m_Entry.SetShearSteelBarSize(0);
         }
         else
            m_Entry.SetShearSteelBarSize(i);
      }

      // last confinement zone
      int siz = vec.size();
      int iz = m_ShearSteelPage.m_LastZone.GetCurSel();
      if (iz==CB_ERR)
         m_Entry.SetLastConfinementZone(iz);
      else
      {
         ASSERT(iz<=siz);
         m_Entry.SetLastConfinementZone(iz);
      }

      // bar size in top flange
      
      m_ShearSteelPage.m_TfBarSize.GetWindowText(bs);
      bs.TrimLeft();
      if (bs=="none")
         m_Entry.SetTopFlangeShearBarSize(0);
      else
      {
         int l = bs.GetLength();
         CString s2 = bs.Right(l-1);
         int i = atoi(s2);
         if (bs.IsEmpty() || (i==0&&bs[0]!='0'))
         {
            ASSERT(0);
            m_Entry.SetTopFlangeShearBarSize(0);
         }
         else
            m_Entry.SetTopFlangeShearBarSize(i);
      }
      // top flange bar spacing
      m_Entry.SetTopFlangeShearBarSpacing(m_TfBarSpacing);
   }

   DDX_Check_Bool(pDX,IDC_STIRRUPS_ENGAGE_DECK,m_Entry.m_bStirrupsEngageDeck);
}

void CGirderMainSheet::UploadTransverseData()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // fill grid
   GirderLibraryEntry::ShearZoneInfoVec vec = m_Entry.GetShearZoneInfo();
   // convert units
   for (GirderLibraryEntry::ShearZoneInfoVec::iterator it = vec.begin(); it!=vec.end(); it++)
   {
      (*it).ZoneLength = ::ConvertFromSysUnits((*it).ZoneLength, pDisplayUnits->SpanLength.UnitOfMeasure);
      (*it).StirrupSpacing = ::ConvertFromSysUnits((*it).StirrupSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure);
   }
   m_ShearSteelPage.m_Grid.FillGrid(vec);

   // shear material name
   m_ShearSteelPage.m_MaterialName.ResetContent();
   int sel = m_ShearSteelPage.m_MaterialName.AddString(m_Entry.GetShearSteelMaterial().c_str());
   CHECK(sel!=CB_ERR);
   m_ShearSteelPage.m_MaterialName.SetCurSel(sel);

   // bar size in confinement zone.
   Int32 siz = m_Entry.GetShearSteelBarSize();
   if (siz>0)
   {
      CString tmp;
      tmp.Format("#%d",siz);
      int idx = m_ShearSteelPage.m_BarSize.SelectString(0,tmp);
      if (idx==CB_ERR)
         m_ShearSteelPage.m_BarSize.SetCurSel(0);
   }
   else
      m_ShearSteelPage.m_BarSize.SetCurSel(0);

   // last confinement zone
   siz = vec.size();
   m_ShearSteelPage.FillLastZone(siz);
   sel = m_Entry.GetNumConfinementZones();
   if (sel <= siz)
      m_ShearSteelPage.m_LastZone.SetCurSel(sel);
   else
   {
      ASSERT(0); // Shear zone from data file out of range
      m_ShearSteelPage.m_LastZone.SetCurSel(0);
   }

   // bar size in top flange
   siz = m_Entry.GetTopFlangeShearBarSize();
   if (siz>0)
   {
      CString tmp;
      tmp.Format("#%d",siz);
      int idx = m_ShearSteelPage.m_TfBarSize.SelectString(0,tmp);
      if (idx==CB_ERR)
         m_ShearSteelPage.m_TfBarSize.SetCurSel(0);
   }
   else
      m_ShearSteelPage.m_TfBarSize.SetCurSel(0);

}

void CGirderMainSheet::ExchangeDiaphragmData(CDataExchange* pDX)
{
   if ( pDX->m_bSaveAndValidate )
      m_DiaphragmPage.m_Grid.GetRules(m_Entry.m_DiaphragmLayoutRules);
   else
      m_DiaphragmPage.m_Grid.SetRules(m_Entry.m_DiaphragmLayoutRules);
}

void CGirderMainSheet::ExchangeHarpPointData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   int hpRef = (int)m_Entry.m_HarpPointReference;
   DDX_CBIndex(pDX,IDC_HPREFERENCE,hpRef);
   if ( pDX->m_bSaveAndValidate )
      m_Entry.m_HarpPointReference = (GirderLibraryEntry::MeasurementLocation)(hpRef);

   int hpMeasure = (int)m_Entry.m_HarpPointMeasure;
   DDX_CBIndex(pDX,IDC_L,hpMeasure);
   if ( pDX->m_bSaveAndValidate )
      m_Entry.m_HarpPointMeasure = (GirderLibraryEntry::MeasurementType)(hpMeasure);

   if ( hpMeasure == m_Entry.mtFractionOfSpanLength || hpMeasure == m_Entry.mtFractionOfGirderLength )
   {
   	DDX_Text(pDX, IDC_HARP_LOCATION, m_Entry.m_HarpingPointLocation );
      DDV_Range( pDX, mfcDDV::LT, mfcDDV::GE, m_Entry.m_HarpingPointLocation, 0.00, 0.5 );
   }
   else
   {
      DDX_UnitValueAndTag(pDX, IDC_HARP_LOCATION, IDC_HARP_LOCATION_TAG, m_Entry.m_HarpingPointLocation, pDisplayUnits->SpanLength);
      DDV_UnitValueZeroOrMore(pDX, IDC_HARP_LOCATION,m_Entry.m_HarpingPointLocation, pDisplayUnits->SpanLength);
   }

   int bMin;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_Check(pDX,IDC_MINIMUM,bMin);
      m_Entry.m_bMinHarpingPointLocation = bMin==0?false:true;
   }
   else
   {
      bMin = m_Entry.m_bMinHarpingPointLocation;
      DDX_Check(pDX,IDC_MINIMUM,bMin);
   }


   DDX_UnitValueAndTag(pDX, IDC_MIN_HP, IDC_MIN_HP_UNIT, m_Entry.m_MinHarpingPointLocation, pDisplayUnits->SpanLength );
   DDV_UnitValueZeroOrMore(pDX, IDC_MIN_HP,m_Entry.m_MinHarpingPointLocation, pDisplayUnits->SpanLength );
}

void CGirderMainSheet::ExchangeDebondCriteriaData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Percentage(pDX,IDC_MAX_TOTAL_STRANDS, m_Entry.m_MaxDebondStrands);
   DDX_Percentage(pDX,IDC_MAX_STRANDS_PER_ROW, m_Entry.m_MaxDebondStrandsPerRow);

 	DDX_Text(pDX, IDC_MAX_NUM_PER_SECTION, m_Entry.m_MaxNumDebondedStrandsPerSection);
   DDX_Percentage(pDX,IDC_MAX_FRACTION_PER_SECTION, m_Entry.m_MaxDebondedStrandsPerSection);

   DDX_UnitValueAndTag(pDX, IDC_MIN_DISTANCE, IDC_MIN_DISTANCE_UNIT, m_Entry.m_MinDebondLength, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero( pDX, IDC_MIN_DISTANCE,m_Entry.m_MinDebondLength, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_DEFAULT_DISTANCE, IDC_DEFAULT_DISTANCE_UNIT, m_Entry.m_DefaultDebondLength, pDisplayUnits->ComponentDim);
   DDV_UnitValueGreaterThanZero( pDX, IDC_DEFAULT_DISTANCE,m_Entry.m_DefaultDebondLength, pDisplayUnits->ComponentDim);

   if (m_Entry.m_DefaultDebondLength < m_Entry.m_MinDebondLength)
   {
      ::AfxMessageBox("Error - The default debond length cannot be less than the minimum debond length");
      pDX->Fail();
   }

   // items with check enable boxes are tricky
   if ( !pDX->m_bSaveAndValidate )
   {
      BOOL bval = m_Entry.m_MaxDebondLengthBySpanFraction<0 ? FALSE:TRUE;
      DDX_Check(pDX, IDC_CHECK_MAX_LENGTH_FRACTION, bval);

      double dval = (bval!=FALSE) ? m_Entry.m_MaxDebondLengthBySpanFraction : 0.0;
      DDX_Percentage(pDX,IDC_MAX_LENGTH_FRACTION, dval);

      bval = m_Entry.m_MaxDebondLengthByHardDistance<0 ? FALSE:TRUE;
      DDX_Check(pDX, IDC_CHECK_MAX_LENGTH, bval);

      dval = (bval!=FALSE) ? m_Entry.m_MaxDebondLengthByHardDistance : 0.0;

      DDX_UnitValueAndTag(pDX, IDC_MAX_LENGTH, IDC_MAX_LENGTH_UNIT, dval, pDisplayUnits->SpanLength);
   }
   else
   {
      BOOL bval;
      DDX_Check(pDX, IDC_CHECK_MAX_LENGTH_FRACTION, bval);

      if(bval!=FALSE)
      {
         DDX_Percentage(pDX,IDC_MAX_LENGTH_FRACTION, m_Entry.m_MaxDebondLengthBySpanFraction);
         double dval = m_Entry.m_MaxDebondLengthBySpanFraction * 100;
         DDV_MinMaxDouble(pDX, dval, 0.0, 45.0);
      }
      else
      {
         m_Entry.m_MaxDebondLengthBySpanFraction = -1;
      }

      DDX_Check(pDX, IDC_CHECK_MAX_LENGTH, bval);
      if(bval!=FALSE)
      {
         DDX_UnitValueAndTag(pDX, IDC_MAX_LENGTH, IDC_MAX_LENGTH_UNIT, m_Entry.m_MaxDebondLengthByHardDistance, pDisplayUnits->SpanLength);
         DDV_UnitValueZeroOrMore(pDX, IDC_MAX_LENGTH,m_Entry.m_MaxDebondLengthByHardDistance, pDisplayUnits->SpanLength);
      }
      else
      {
         m_Entry.m_MaxDebondLengthByHardDistance = -1;
      }
   }
}


void CGirderMainSheet::OnApply( NMHDR * pNotifyStruct, LRESULT * result )
{
   GirderLibraryEntry::GirderEntryDataErrorVec vec;
   m_Entry.ValidateData(&vec);
   if (vec.size()>0)
   {
      // errors occurred - need to tell user
      // create error window or clear existing one.
      if (!m_GirderErrorDlg.GetSafeHwnd())
      {
         m_GirderErrorDlg.Create(IDD_GIRDER_ERROR, this);
      }
      else
      {
         m_GirderErrorDlg.m_ErrorEdit.SetSel(0,-1);
         m_GirderErrorDlg.m_ErrorEdit.Clear();
      }
      // fill error window with messages.
      CString errmsg;
      for(GirderLibraryEntry::GirderEntryDataErrorVec::const_iterator it = vec.begin();
          it != vec.end(); it++)
      {
         errmsg += "- ";
         errmsg +=  (*it).GetErrorMsg().c_str();
         errmsg += "\015\012\015\012";
      }
      m_GirderErrorDlg.m_ErrorEdit.SetWindowText(errmsg);

      m_GirderErrorDlg.ShowWindow(TRUE);

      *result = PSNRET_INVALID_NOCHANGEPAGE ;
   }
   else
   {
      // no errors
      // delete error window if it exists.
      if (!m_GirderErrorDlg.GetSafeHwnd())
         m_GirderErrorDlg.DestroyWindow();

      *result = PSNRET_NOERROR; 
   }
}

BOOL CGirderMainSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += " - ";
   head += m_Entry.GetName().c_str();
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += " (Read Only)";
   }
   SetWindowText(head);
	
	return bResult;
}

void CGirderMainSheet::MiscOnFractional()
{
   CWnd* pWnd = m_HarpPointPage.GetDlgItem(IDC_HARP_LOCATION_TAG);
   pWnd->SetWindowText("(0.0-0.5]");
}

void CGirderMainSheet::MiscOnAbsolute()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   CDataExchange dx(&m_HarpPointPage,FALSE);
   DDX_Tag(&dx, IDC_HARP_LOCATION_TAG, pDisplayUnits->SpanLength );
}
