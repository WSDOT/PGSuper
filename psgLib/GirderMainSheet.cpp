///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <EAF\EAFApp.h>
#include <EAF\EAFDisplayUnits.h>

#include <Lrfd\RebarPool.h>
#include <IFace\BeamFactory.h>

#include <Plugins\BeamFamilyCLSID.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline bool B2b(BOOL val) { return val!=0; }

/////////////////////////////////////////////////////////////////////////////
// CGirderMainSheet

IMPLEMENT_DYNAMIC(CGirderMainSheet, CPropertySheet)

CGirderMainSheet::CGirderMainSheet( GirderLibraryEntry& rentry,
                                   bool allowEditing,int refCount,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_RefCount(refCount),
   m_bAllowEditing(allowEditing)
{
   Init();
}

CGirderMainSheet::~CGirderMainSheet()
{
}

void CGirderMainSheet::SetBeamFactory(IBeamFactory* pFactory)
{
   m_Entry.SetBeamFactory(pFactory);
}

bool CGirderMainSheet::IsSplicedGirder()
{
   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_Entry.m_pBeamFactory);
   return (splicedBeamFactory == nullptr ? false : true);
}

bool CGirderMainSheet::HasDeck() const
{
   std::vector<pgsTypes::SupportedBeamSpacing> vSpacings = m_Entry.m_pBeamFactory->GetSupportedBeamSpacings();
   for (auto spacing : vSpacings)
   {
      auto vDeckTypes = m_Entry.m_pBeamFactory->GetSupportedDeckTypes(spacing);
      for (auto deckType : vDeckTypes)
      {
         if (deckType != pgsTypes::sdtNone)
         {
            return true;
         }
      }
   }

   return false;
}

LPCTSTR CGirderMainSheet::GetIntentionalRougheningPrompt() const
{
   return _T("Top flange is intentionally roughened for interface shear capacity");
}

void CGirderMainSheet::UpdatePropertyPages()
{
   // Certain pages don't apply to spliced girders
   if ( IsSplicedGirder() )
   {
      AddPage(&m_GirderDimensionsPage);
      AddPage(&m_FlexureDesignPage); // contains debond limits
      //AddPage(&m_ShearDesignPage); // no shear design for spliced girders
      //AddPage(&m_LongSteelPage); // no default long. reinforcement for spliced girders
      //AddPage(&m_ShearSteelPage); // no default trans. reinforcement for spliced girders
      AddPage(&m_GirderHaunchAndCamberPage);
      AddPage(&m_DiaphragmPage);
   }
   else
   {
      AddPage(&m_GirderDimensionsPage);
      AddPage(&m_GirderPermanentStrandPage);   // straight and harped strands
      AddPage(&m_GirderTemporaryStrandPage); // temporary strands
      AddPage(&m_FlexureDesignPage);
      AddPage(&m_ShearDesignPage);
      AddPage(&m_HarpPointPage);
      AddPage(&m_LongSteelPage);
      AddPage(&m_ShearSteelPage);
      AddPage(&m_GirderHaunchAndCamberPage);
      AddPage(&m_DiaphragmPage);
   }
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
   m_GirderPermanentStrandPage.m_psp.dwFlags|= PSP_HASHELP;
   m_GirderTemporaryStrandPage.m_psp.dwFlags|= PSP_HASHELP;
   m_LongSteelPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_ShearSteelPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_HarpPointPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_DiaphragmPage.m_psp.dwFlags            |= PSP_HASHELP;
   m_FlexureDesignPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_ShearDesignPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_GirderHaunchAndCamberPage.m_psp.dwFlags   |= PSP_HASHELP;

   UpdatePropertyPages();
}

void CGirderMainSheet::ExchangeDimensionData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if (pDX->m_bSaveAndValidate)
   {
	   DDX_Text(pDX, IDC_ENAME, m_Name);
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Girder Name cannot be blank"));
         pDX->Fail();
      }
      m_Entry.SetName(m_Name);
   }
   else
   {
      m_Name = m_Entry.GetName().c_str();
	   DDX_Text(pDX, IDC_ENAME, m_Name);
   }

   Float64 Cd = m_Entry.GetDragCoefficient();
   DDX_Text(pDX, IDC_DRAG_COEFFICIENT, Cd);
   DDV_GreaterThanZero(pDX,IDC_DRAG_COEFFICIENT,Cd);
   if ( pDX->m_bSaveAndValidate )
   {
      m_Entry.SetDragCoefficient(Cd);
   }

   bool bUnitsSI = (pApp->GetUnitsMode() == eafTypes::umSI);

   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(m_Entry.m_pBeamFactory);

   DDX_Check_Bool(pDX,IDC_VARIABLE_DEPTH_CHECK,m_Entry.m_bIsVariableDepthSectionEnabled);
   DDX_Check_Bool(pDX,IDC_BEARING_ELEVS,m_Entry.m_DoReportBearingElevationsAtGirderEdges);

   if ( pDX->m_bSaveAndValidate )
   {
      // Pull the values from the grid... Convert back to system units
      const auto& names = m_Entry.m_pBeamFactory->GetDimensionNames();
      const auto& units = m_Entry.m_pBeamFactory->GetDimensionUnits(bUnitsSI);

      ATLASSERT(names.size() == units.size());

      auto name_iter(names.begin());
      auto name_end(names.end());
      auto unit_iter(units.begin());
      auto unit_end(units.end());
      long nRow = 1;
      for ( ; name_iter != name_end || unit_iter != unit_end; name_iter++, unit_iter++ )
      {
         CString strValue = m_GirderDimensionsPage.m_Grid.GetCellValue(nRow,1);
         Float64 value = _tstof(strValue);

         const unitLength* pUnit = *unit_iter;

         if (pUnit == (const unitLength*)BFDIMUNITBOOLEAN)
         {
            value = value != 0 ? 1 : 0;
         }

         if (pUnit != (const unitLength*)BFDIMUNITSCALAR && pUnit != (const unitLength*)BFDIMUNITBOOLEAN)
         {
            value = ::ConvertToSysUnits(value, *pUnit);
         }

         m_Entry.SetDimension(*name_iter,value,true);
         nRow++;
      }

      std::_tstring strErrMsg;
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
      const auto& names = m_Entry.m_pBeamFactory->GetDimensionNames();
      const auto& units = m_Entry.m_pBeamFactory->GetDimensionUnits(bUnitsSI);

      ATLASSERT(names.size() == units.size());

      auto name_iter(names.begin());
      auto name_iter_end(names.end());
      auto unit_iter(units.begin());
      long nRow = 1;
      for ( ; name_iter != name_iter_end; name_iter++, unit_iter++ )
      {
         auto name = *name_iter;
         VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 0), name.c_str()));

         Float64 value = m_Entry.GetDimension(name);

         const unitLength* pUnit = *unit_iter;
         if (pUnit == (const unitLength*)BFDIMUNITBOOLEAN)
         {
            ATLASSERT((LONG)value == 0 || (LONG)value == 1);
            VERIFY(m_GirderDimensionsPage.m_Grid.SetStyleRange(CGXRange(nRow, 1), CGXStyle().SetControl(GX_IDS_CTRL_CHECKBOX3D).SetHorizontalAlignment(DT_CENTER)));
            VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 1), (LONG)value));
         }
         else
         {
            if (pUnit != (const unitLength*)BFDIMUNITSCALAR)
            {
               value = ::ConvertFromSysUnits(value, *pUnit);

               name += _T(" (");
               name += pUnit->UnitTag();
               name += _T(")");
            }

            VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 0), name.c_str()));
            VERIFY(m_GirderDimensionsPage.m_Grid.SetValueRange(CGXRange(nRow, 1), value));
         }

         nRow++;
      }

      m_GirderDimensionsPage.m_Grid.ResizeColWidthsToFit(CGXRange(0,0,m_GirderDimensionsPage.m_Grid.GetRowCount(),1));

      m_GirderDimensionsPage.GetDlgItem(IDC_NOTES)->SetWindowText(_T(""));

      if ( splicedBeamFactory && splicedBeamFactory->SupportsVariableDepthSection() )
      {
         CString strNotes;
         CString strNote1;
         strNote1.Format(_T("This is a variable depth girder. The overall section depth can be modified in the bridge model. The %s dimensions(s) establish the default depth of the section."),splicedBeamFactory->GetVariableDepthDimension());

         if (splicedBeamFactory->CanBottomFlangeDepthVary())
         {
            CString strNote2;
            strNote2.Format(_T("This girder supports a variable depth bottom flange. The %s dimension(s) can be modified in the bridge model."),splicedBeamFactory->GetBottomFlangeDepthDimension());

            strNotes.Format(_T("%s\n\n%s"),strNote1,strNote2);
         }
         else
         {
            strNotes = strNote1;
         }

         m_GirderDimensionsPage.GetDlgItem(IDC_NOTES)->SetWindowText(strNotes);
      }
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

      ROWCOL nrows = m_GirderTemporaryStrandPage.m_TemporaryGrid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         Float64 x,y;
         if (m_GirderTemporaryStrandPage.m_TemporaryGrid.GetRowData(i,&x,&y))
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
            msg.Format(_T("Error - Incomplete data in row %d"),i);
            AfxMessageBox(msg);
		      pDX->PrepareCtrl(m_GirderTemporaryStrandPage.m_TemporaryGrid.GridWnd()->GetDlgCtrlID());
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

   m_GirderTemporaryStrandPage.m_TemporaryGrid.FillGrid(new_temp_strands);
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

   DDX_Check_Bool(pDX,IDC_ALLOW_STR_ADJUST, m_Entry.m_StraightAdjustment.m_AllowVertAdjustment );
   DDX_UnitValueAndTag(pDX, IDC_STR_LSL, IDC_STR_LSL_T, m_Entry.m_StraightAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_STR_LSL,m_Entry.m_StraightAdjustment.m_BottomLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_STR_LSL, (int&)m_Entry.m_StraightAdjustment.m_BottomFace);
   DDX_UnitValueAndTag(pDX, IDC_STR_USL, IDC_STR_USL_T, m_Entry.m_StraightAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDV_UnitValueZeroOrMore(pDX, IDC_STR_USL,m_Entry.m_StraightAdjustment.m_TopLimit, pDisplayUnits->ComponentDim );
   DDX_CBIndex(pDX,IDC_COMBO_STR_USL, (int&)m_Entry.m_StraightAdjustment.m_TopFace);
   

   DDX_Check_Bool(pDX,IDC_ODD_STRANDS, m_Entry.m_bOddNumberOfHarpedStrands );
   DDX_Check_Bool(pDX,IDC_USE_DIFF_GRID, m_Entry.m_bUseDifferentHarpedGridAtEnds);

   int idx;
   if (!pDX->m_bSaveAndValidate)
   {
      idx = m_Entry.GetAdjustableStrandType();
   }

   DDX_CBIndex(pDX,IDC_WEB_STRAND_TYPE_COMBO, idx);

   if (pDX->m_bSaveAndValidate)
   {
      pgsTypes::AdjustableStrandType as_type = (pgsTypes::AdjustableStrandType)idx;
      m_Entry.SetAdjustableStrandType(as_type);

      // Different grid at ends option is only available for pure harped strands
      if(as_type != pgsTypes::asHarped)
      {
         m_Entry.m_bUseDifferentHarpedGridAtEnds = false;
      }
   }

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
      m_Entry.m_PermanentStrands.clear();

      // grab a reference to grid's internal data to save typing
      CGirderGlobalStrandGrid::EntryCollectionType& grid_collection =  m_GirderPermanentStrandPage.m_MainGrid.m_Entries;

      CollectionIndexType g_cnt = grid_collection.size();
      StrandIndexType num_straight=0;
      StrandIndexType num_harped=0;
      for (CollectionIndexType g_idx=0; g_idx<g_cnt; g_idx++)
      {
         const CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry = grid_collection[g_idx];

         if (entry.m_Type == GirderLibraryEntry::stStraight)
         {
            // straight strands are easy, just convert units and add point
            Float64 x = ::ConvertToSysUnits(entry.m_X, pDisplayUnits->ComponentDim.UnitOfMeasure);
            Float64 y = ::ConvertToSysUnits(entry.m_Y, pDisplayUnits->ComponentDim.UnitOfMeasure);

            m_Entry.m_StraightStrands.push_back(GirderLibraryEntry::StraightStrandLocation(x,y,entry.m_CanDebond));

            m_Entry.m_PermanentStrands.push_back(GirderLibraryEntry::PermanentStrand(GirderLibraryEntry::stStraight, num_straight++));
         }
         else if (entry.m_Type == GirderLibraryEntry::stAdjustable)
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

            m_Entry.m_PermanentStrands.push_back(GirderLibraryEntry::PermanentStrand(GirderLibraryEntry::stAdjustable, num_harped++));
         }
         else
            ATLASSERT(false);
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

   StrandIndexType num_global = m_Entry.GetPermanentStrandGridSize();
   for (StrandIndexType idx=0; idx<num_global; idx++)
   {
      StrandIndexType strand_idx;
      GirderLibraryEntry::psStrandType strand_type;
      m_Entry.GetGridPositionFromPermStrandGrid(idx, &strand_type, &strand_idx);

      Float64 start_x,start_y,x,y,end_x,end_y;
      bool can_debond(false);
      if (strand_type == GirderLibraryEntry::stStraight)
      {
         m_Entry.GetStraightStrandCoordinates(strand_idx, &start_x, &start_y, &end_x, &end_y, &can_debond);
         x = start_x;
         y = start_y;
      }
      else if (strand_type == GirderLibraryEntry::stAdjustable)
      {
         m_Entry.GetHarpedStrandCoordinates(strand_idx, &start_x, &start_y, &x, &y, &end_x, &end_y);
      }
      else
      {
         ATLASSERT(false);
      }

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
   m_GirderPermanentStrandPage.m_MainGrid.FillGrid(grid_collection);
}

void CGirderMainSheet::UploadLongitudinalData()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   GirderLibraryEntry::LongSteelInfoVec vec = m_Entry.GetLongSteelInfo();

   // Convert to display units before filling grid
   for (GirderLibraryEntry::LongSteelInfoVec::iterator it = vec.begin(); it!=vec.end(); it++)
   {
      (*it).BarLength = ::ConvertFromSysUnits((*it).BarLength, pDisplayUnits->SpanLength.UnitOfMeasure );
      (*it).DistFromEnd = ::ConvertFromSysUnits((*it).DistFromEnd, pDisplayUnits->SpanLength.UnitOfMeasure );
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
            lsi.BarLength    = ::ConvertToSysUnits(lsi.BarLength, pDisplayUnits->SpanLength.UnitOfMeasure );
            lsi.DistFromEnd  = ::ConvertToSysUnits(lsi.DistFromEnd, pDisplayUnits->SpanLength.UnitOfMeasure );
            lsi.Cover        = ::ConvertToSysUnits(lsi.Cover, pDisplayUnits->ComponentDim.UnitOfMeasure );
            lsi.BarSpacing   = ::ConvertToSysUnits(lsi.BarSpacing, pDisplayUnits->ComponentDim.UnitOfMeasure );
            vec.push_back(lsi);
         }
         else
         {
            pDX->Fail();
         }
      }
      m_Entry.SetLongSteelInfo(vec);
   }
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

void CGirderMainSheet::ExchangeHaunchData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX, IDC_MIN_FILLET, IDC_MIN_FILLET_UNIT, m_Entry.m_MinFilletValue, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_HARP_LOCATION,m_Entry.m_MinFilletValue, pDisplayUnits->ComponentDim);

   DDX_UnitValueAndTag(pDX, IDC_MIN_HAUNCH_BC, IDC_MIN_HAUNCH_BC_UNIT, m_Entry.m_MinHaunchAtBearingLines, pDisplayUnits->ComponentDim);

   int bMin;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_Check(pDX,IDC_CHECK_CL,bMin);
      m_Entry.m_DoCheckMinHaunchAtBearingLines = bMin==0?false:true;

      if (m_Entry.m_DoCheckMinHaunchAtBearingLines )
      {
         DDV_UnitValueZeroOrMore(pDX, IDC_MIN_HAUNCH_BC,m_Entry.m_MinHaunchAtBearingLines, pDisplayUnits->ComponentDim);
      }
   }
   else
   {
      bMin = m_Entry.m_DoCheckMinHaunchAtBearingLines;
      DDX_Check(pDX,IDC_CHECK_CL,bMin);
   }

   DDX_UnitValueAndTag(pDX, IDC_EXCESS_HAUNCH, IDC_EXCESS_HAUNCH_UNIT, m_Entry.m_ExcessiveSlabOffsetWarningTolerance, pDisplayUnits->ComponentDim);
   DDV_UnitValueZeroOrMore(pDX, IDC_EXCESS_HAUNCH,m_Entry.m_ExcessiveSlabOffsetWarningTolerance, pDisplayUnits->ComponentDim);

   // Don't exchange multiplier data for spliced projects
   if (!this->IsSplicedGirder()) 
   {
 	   DDX_Text(pDX, IDC_ERECTION,   m_Entry.m_CamberMultipliers.ErectionFactor);
 	   DDX_Text(pDX, IDC_CREEP,      m_Entry.m_CamberMultipliers.CreepFactor);
 	   DDX_Text(pDX, IDC_DIAPHRAGM,  m_Entry.m_CamberMultipliers.DiaphragmFactor);
 	   DDX_Text(pDX, IDC_DECK_PANEL, m_Entry.m_CamberMultipliers.DeckPanelFactor);
 	   DDX_Text(pDX, IDC_SLAB,       m_Entry.m_CamberMultipliers.SlabUser1Factor);
 	   DDX_Text(pDX, IDC_HAUNCH,     m_Entry.m_CamberMultipliers.SlabPadLoadFactor);
 	   DDX_Text(pDX, IDC_BARRIER,    m_Entry.m_CamberMultipliers.BarrierSwOverlayUser2Factor);

      DDX_Text(pDX, IDC_PRECAMBER_LIMIT, m_Entry.m_PrecamberLimit);
      DDV_LimitOrMore(pDX, IDC_PRECAMBER_LIMIT, m_Entry.m_PrecamberLimit, 0.0);
   }
}

void CGirderMainSheet::ExchangeFlexuralCriteriaData(CDataExchange* pDX)
{
   // debonding limits
   ExchangeDebondCriteriaData(pDX);

   // automated design strategies
   ExchangeFlexuralDesignStrategyCriteriaData(pDX);
}

void CGirderMainSheet::ExchangeDebondCriteriaData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Check_Bool(pDX, IDC_CHECK_MAX_TOTAL_STRANDS, m_Entry.m_bCheckMaxDebondStrands);
   DDX_Percentage(pDX,IDC_MAX_TOTAL_STRANDS, m_Entry.m_MaxDebondStrands);
   DDX_Percentage(pDX,IDC_MAX_STRANDS_PER_ROW, m_Entry.m_MaxDebondStrandsPerRow);

   DDX_Text(pDX, IDC_MAX_NUM_PER_SECTION_10_OR_LESS, m_Entry.m_MaxNumDebondedStrandsPerSection10orLess);
   DDX_Text(pDX, IDC_MAX_NUM_PER_SECTION, m_Entry.m_MaxNumDebondedStrandsPerSection);
   DDX_Check_Bool(pDX, IDC_CHECK_MAX_FRACTION_PER_SECTION, m_Entry.m_bCheckMaxNumDebondedStrandsPerSection);
   DDX_Percentage(pDX,IDC_MAX_FRACTION_PER_SECTION, m_Entry.m_MaxDebondedStrandsPerSection);

   DDX_Text(pDX, IDC_MIN_DISTANCE_DB, m_Entry.m_MinDebondLengthDB);
   DDV_GreaterThanZero(pDX, IDC_MIN_DISTANCE_DB, m_Entry.m_MinDebondLengthDB);
   DDX_Check_Bool(pDX, IDC_CHECK_MIN_DISTANCE, m_Entry.m_bCheckMinDebondLength);
   DDX_UnitValueAndTag(pDX, IDC_MIN_DISTANCE, IDC_MIN_DISTANCE_UNIT, m_Entry.m_MinDebondLength, pDisplayUnits->SpanLength);
   DDV_UnitValueGreaterThanZero( pDX, IDC_MIN_DISTANCE,m_Entry.m_MinDebondLength, pDisplayUnits->SpanLength);

   DDX_Check_Bool(pDX, IDC_CHECK_DEBONDING_SYMMETRY, m_Entry.m_bCheckDebondingSymmetry);
   DDX_Check_Bool(pDX, IDC_CHECK_ADJACENT_STRANDS, m_Entry.m_bCheckAdjacentDebonding);
   DDX_Check_Bool(pDX, IDC_CHECK_WEB_WIDTH_PROJECTIONS, m_Entry.m_bCheckDebondingInWebWidthProjections);

   // items with check enable boxes are tricky
   if ( !pDX->m_bSaveAndValidate )
   {
      BOOL bval = m_Entry.m_MaxDebondLengthBySpanFraction<0 ? FALSE:TRUE;
      DDX_Check(pDX, IDC_CHECK_MAX_LENGTH_FRACTION, bval);

      Float64 dval = (bval!=FALSE) ? m_Entry.m_MaxDebondLengthBySpanFraction : 0.0;
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
         Float64 dval = m_Entry.m_MaxDebondLengthBySpanFraction * 100;
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

void CGirderMainSheet::ExchangeFlexuralDesignStrategyCriteriaData(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_Tag(pDX, IDC_STRAIGHT_UNITS, pDisplayUnits->Stress);
   DDX_Tag(pDX, IDC_DEBOND_UNITS, pDisplayUnits->Stress);
   DDX_Tag(pDX, IDC_HARPED_UNITS, pDisplayUnits->Stress);

   if ( !pDX->m_bSaveAndValidate )
   {
      GirderLibraryEntry::PrestressDesignStrategyIterator it=m_Entry.m_PrestressDesignStrategies.begin();
      while (it!=m_Entry.m_PrestressDesignStrategies.end())
      {
         GirderLibraryEntry::PrestressDesignStrategy& rstrat = *it;

         if ( dtDesignFullyBonded       == it->m_FlexuralDesignType ||
                   dtDesignFullyBondedRaised == it->m_FlexuralDesignType)
         {
            BOOL bval = TRUE;
            DDX_Check(pDX, IDC_STRAIGHT_DESIGN_CHECK, bval);

            DDX_UnitValue(pDX, IDC_STRAIGHT_FCI, rstrat.m_MaxFci, pDisplayUnits->Stress );
            DDX_UnitValue(pDX, IDC_STRAIGHT_FC, rstrat.m_MaxFc, pDisplayUnits->Stress );

            bval = dtDesignFullyBondedRaised==it->m_FlexuralDesignType ? TRUE:FALSE;
            DDX_Check(pDX, IDC_STRAIGHT_RAISE_CHECK, bval);
         }
         else if ( dtDesignForDebonding       == it->m_FlexuralDesignType || 
                   dtDesignForDebondingRaised == it->m_FlexuralDesignType)
         {
            BOOL bval = TRUE;
            DDX_Check(pDX, IDC_DEBOND_DESIGN_CHECK, bval);

            DDX_UnitValue(pDX, IDC_DEBOND_FCI, rstrat.m_MaxFci, pDisplayUnits->Stress );
            DDX_UnitValue(pDX, IDC_DEBOND_FC, rstrat.m_MaxFc, pDisplayUnits->Stress );

            bval = dtDesignForDebondingRaised==it->m_FlexuralDesignType ? TRUE:FALSE;
            DDX_Check(pDX, IDC_DEBOND_RAISE_CHECK, bval);
         }
         else if (dtDesignForHarping == rstrat.m_FlexuralDesignType)
         {
            BOOL bval = TRUE;
            DDX_Check(pDX, IDC_HARPED_DESIGN_CHECK, bval);

            DDX_UnitValue(pDX, IDC_HARPED_FCI, rstrat.m_MaxFci, pDisplayUnits->Stress );
            DDX_UnitValue(pDX, IDC_HARPED_FC,  rstrat.m_MaxFc, pDisplayUnits->Stress );
         }
         else
         {
            ATLASSERT(0); // new strategy?
         }

         it++;
      }
   }
   else
   {
      // Hard coded min design values. 
      bool is_si = eafTypes::umSI == pApp->GetUnitsMode();

      Float64 minfci = is_si ? ::ConvertToSysUnits(28.0,unitMeasure::MPa) :
                               ::ConvertToSysUnits( 4.0,unitMeasure::KSI); // minimum per LRFD 5.4.2.1

      Float64 minfc =  is_si ? ::ConvertToSysUnits(34.5,unitMeasure::MPa) :
                               ::ConvertToSysUnits( 5.0,unitMeasure::KSI); // agreed by wsdot and txdot

      m_Entry.m_PrestressDesignStrategies.clear();

      bool can_straight = CanDoAllStraightDesign();
      bool can_harp = CanHarpStrands();
      bool can_debond = CanDebondStrands();

      BOOL bval;
      DDX_Check(pDX, IDC_STRAIGHT_DESIGN_CHECK, bval);

      // all straight bonded
      if(bval!=FALSE && can_straight)
      {
         GirderLibraryEntry::PrestressDesignStrategy strat;

         DDX_Check(pDX, IDC_STRAIGHT_RAISE_CHECK, bval);

         strat.m_FlexuralDesignType = (bval!=FALSE) ? dtDesignFullyBondedRaised : dtDesignFullyBonded;

         DDX_UnitValueAndTag(pDX, IDC_STRAIGHT_FCI, IDC_STRAIGHT_UNITS, strat.m_MaxFci, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_STRAIGHT_FCI, strat.m_MaxFci, minfci, pDisplayUnits->Stress);
         DDX_UnitValueAndTag(pDX, IDC_STRAIGHT_FC, IDC_STRAIGHT_UNITS, strat.m_MaxFc, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_STRAIGHT_FC, strat.m_MaxFc, minfc, pDisplayUnits->Stress);

         if (strat.m_MaxFc < strat.m_MaxFci)
         {
            ::AfxMessageBox(_T("Final Strength must be greater or equal to Release Strength"),MB_OK|MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         m_Entry.m_PrestressDesignStrategies.push_back(strat);
      }

      // debonded
      DDX_Check(pDX, IDC_DEBOND_DESIGN_CHECK, bval);

      if(bval!=FALSE && can_debond)
      {
         GirderLibraryEntry::PrestressDesignStrategy strat;

         DDX_Check(pDX, IDC_DEBOND_RAISE_CHECK, bval);

         strat.m_FlexuralDesignType = (bval!=FALSE) ? dtDesignForDebondingRaised : dtDesignForDebonding;

         DDX_UnitValueAndTag(pDX, IDC_DEBOND_FCI, IDC_DEBOND_UNITS, strat.m_MaxFci, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_DEBOND_FCI, strat.m_MaxFci, minfci, pDisplayUnits->Stress);
         DDX_UnitValueAndTag(pDX, IDC_DEBOND_FC, IDC_DEBOND_UNITS, strat.m_MaxFc, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_DEBOND_FC, strat.m_MaxFc, minfc, pDisplayUnits->Stress);

         if (strat.m_MaxFc < strat.m_MaxFci)
         {
            ::AfxMessageBox(_T("Final Strength must be greater or equal to Release Strength"),MB_OK|MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         m_Entry.m_PrestressDesignStrategies.push_back(strat);
      }

      // harped
      DDX_Check(pDX, IDC_HARPED_DESIGN_CHECK, bval);

      if(bval!=FALSE && can_harp)
      {
         GirderLibraryEntry::PrestressDesignStrategy strat;

         strat.m_FlexuralDesignType = dtDesignForHarping;

         DDX_UnitValueAndTag(pDX, IDC_HARPED_FCI, IDC_HARPED_UNITS, strat.m_MaxFci, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_HARPED_FCI, strat.m_MaxFci, minfci, pDisplayUnits->Stress);
         DDX_UnitValueAndTag(pDX, IDC_HARPED_FC, IDC_HARPED_UNITS, strat.m_MaxFc, pDisplayUnits->Stress );
         DDV_UnitValueLimitOrMore(pDX, IDC_HARPED_FC, strat.m_MaxFc, minfc, pDisplayUnits->Stress);

         if (strat.m_MaxFc < strat.m_MaxFci)
         {
            ::AfxMessageBox(_T("Final Strength must be greater or equal to Release Strength"),MB_OK|MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         m_Entry.m_PrestressDesignStrategies.push_back(strat);
      }
   }
}


void CGirderMainSheet::UploadShearDesignData(CDataExchange* pDX)
{
   m_ShearDesignPage.m_StirrupSizeBarComboColl.clear();

   IndexType siz = m_Entry.GetNumStirrupSizeBarCombos();
   for(IndexType is=0; is<siz; is++)
   {
      CShearDesignPage::StirrupSizeBarCombo cbo;

      m_Entry.GetStirrupSizeBarCombo(is, &cbo.Size, &cbo.NLegs);

      m_ShearDesignPage.m_StirrupSizeBarComboColl.push_back(cbo);
   }

   m_ShearDesignPage.m_BarSpacings.clear();

   siz = m_Entry.GetNumAvailableBarSpacings();
   for(IndexType is=0; is<siz; is++)
   {
      m_ShearDesignPage.m_BarSpacings.push_back( m_Entry.GetAvailableBarSpacing(is) );
   }

   m_ShearDesignPage.m_MaxStirrupSpacingChange = m_Entry.GetMaxSpacingChangeInZone();
   m_ShearDesignPage.m_MaxShearCapChange = m_Entry.GetMaxShearCapacityChangeInZone();

   Uint32 u32;
   Float64 f64;
   m_Entry.GetMinZoneLength(&u32, &f64);

   m_ShearDesignPage.m_MinZoneLengthBars = u32;
   m_ShearDesignPage.m_MinZoneLengthDist = f64;

   m_ShearDesignPage.m_bExtendDeckBars = m_Entry.GetExtendBarsIntoDeck();
   m_ShearDesignPage.m_bBarsProvideConfinement = m_Entry.GetBarsActAsConfinement();

   m_ShearDesignPage.m_LongReinfShearMethod = m_Entry.GetLongShearCapacityIncreaseMethod();
}

void CGirderMainSheet::DownloadShearDesignData(CDataExchange* pDX)
{
   m_Entry.ClearStirrupSizeBarCombos();
   for(CShearDesignPage::StirrupSizeBarComboIter it=m_ShearDesignPage.m_StirrupSizeBarComboColl.begin();
       it!=m_ShearDesignPage.m_StirrupSizeBarComboColl.end(); it++)
   {
      m_Entry.AddStirrupSizeBarCombo(it->Size, it->NLegs);
   }

   m_Entry.ClearAvailableBarSpacings();

   for(std::vector<Float64>::iterator its=m_ShearDesignPage.m_BarSpacings.begin(); its!=m_ShearDesignPage.m_BarSpacings.end(); its++)
   {
      // page has already sorted and removed duplicates from list
      Float64 val = *its;
      m_Entry.AddAvailableBarSpacing(val);
   }

   m_Entry.SetMaxSpacingChangeInZone( m_ShearDesignPage.m_MaxStirrupSpacingChange );
   m_Entry.SetMaxShearCapacityChangeInZone( m_ShearDesignPage.m_MaxShearCapChange );

   m_Entry.SetMinZoneLength(m_ShearDesignPage.m_MinZoneLengthBars, m_ShearDesignPage.m_MinZoneLengthDist);

   m_Entry.SetExtendBarsIntoDeck( B2b(m_ShearDesignPage.m_bExtendDeckBars) );
   m_Entry.SetBarsActAsConfinement( B2b(m_ShearDesignPage.m_bBarsProvideConfinement) );

   m_Entry.SetLongShearCapacityIncreaseMethod( (GirderLibraryEntry::LongShearCapacityIncreaseMethod)(m_ShearDesignPage.m_LongReinfShearMethod) );
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
         errmsg += _T("- ");
         errmsg +=  (*it).GetErrorMsg().c_str();
         errmsg += _T("\015\012\015\012");
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

void CGirderMainSheet::SetDebondTabName()
{
   if ( IsSplicedGirder() )
   {
      int index = GetPageIndex(&m_FlexureDesignPage);
      if ( index < 0 )
      {
         return; // not using the debond tab
      }

      CTabCtrl* pTab = GetTabControl();
      TC_ITEM ti;
      ti.mask = TCIF_TEXT;
      ti.pszText = _T("Debonding");
      pTab->SetItem(index,&ti);
   }
}

BOOL CGirderMainSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

   // Shear page takes care of its own data
   m_ShearSteelPage.m_ShearData = m_Entry.GetShearData();

   SetDebondTabName();

   // disable OK button if editing not allowed
   CString head;
   if ( IsSplicedGirder() )
   {
      head = _T("Spliced Girder Segment");
   }
   else
   {
      head = _T("Girders");
   }

   head += _T(" - ");
   head += m_Entry.GetName().c_str();
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
	return bResult;
}

void CGirderMainSheet::MiscOnFractional()
{
   CWnd* pWnd = m_HarpPointPage.GetDlgItem(IDC_HARP_LOCATION_TAG);
   pWnd->SetWindowText(_T("(0.0-0.5]"));
}

void CGirderMainSheet::MiscOnAbsolute()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   CDataExchange dx(&m_HarpPointPage,FALSE);
   DDX_Tag(&dx, IDC_HARP_LOCATION_TAG, pDisplayUnits->SpanLength );
}

bool CGirderMainSheet::CanHarpStrands() const
{
   if (pgsTypes::asStraight != m_Entry.GetAdjustableStrandType() && m_Entry.GetNumHarpedStrandCoordinates() > 0)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool CGirderMainSheet::CanDebondStrands() const
{
   if (m_Entry.CanDebondStraightStrands() && !(pgsTypes::asHarped == m_Entry.GetAdjustableStrandType() && m_Entry.GetNumHarpedStrandCoordinates() > 0) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool CGirderMainSheet::CanDoAllStraightDesign() const
{
   // Only time we can't do straight design is if non-parallel harped strands exist
   if (m_Entry.GetNumHarpedStrandCoordinates() == 0)
   {
      return true;
   }
   else if (pgsTypes::asHarped == m_Entry.GetAdjustableStrandType() && m_Entry.IsDifferentHarpedGridAtEndsUsed() )
   {
      return false;
   }
   else
   {
      return true;
   }
}