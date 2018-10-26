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

#include "StdAfx.h"
#include <Reporting\PrestressStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPrestressStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressStressTable::CPrestressStressTable()
{
}

CPrestressStressTable::CPrestressStressTable(const CPrestressStressTable& rOther)
{
   MakeCopy(rOther);
}

CPrestressStressTable::~CPrestressStressTable()
{
}

//======================== OPERATORS  =======================================
CPrestressStressTable& CPrestressStressTable::operator= (const CPrestressStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPrestressStressTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,
                                            bool bDesign,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, gdrpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, spanpoi, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   //gdrpoi.IncludeSpanAndGirder(span == ALL_SPANS);
   //spanpoi.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   //// Determine if there are temporary strands
   //bool bTempStrands = false;
   //GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   //GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
   //GroupIndexType lastGroupIdx  = (segmentKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   //for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   //{
   //   GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
   //   GirderIndexType firstGdrIdx = (segmentKey.girderIndex == ALL_GIRDERS ? 0 : segmentKey.girderIndex);
   //   GirderIndexType lastGdrIdx  = (segmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);
   //   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   //   {
   //      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
   //      SegmentIndexType firstSegIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
   //      SegmentIndexType lastSegIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegIdx );
   //      for ( SegmentIndexType segIdx = firstSegIdx; segIdx <= lastSegIdx; segIdx++ )
   //      {
   //         CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);
   //         if ( 0 < pStrandGeom->GetMaxStrands(thisSegmentKey,pgsTypes::Temporary) )
   //         {
   //            bTempStrands = true;
   //            break;
   //         }
   //      }
   //   }
   //}

   ColumnIndexType nColumns;
   if ( bDesign )
   {
      nColumns = 2 // two location columns
               + nIntervals-1 // one for each interval
               ;//+ (bTempStrands ? 1 : 0); // one for temporary strand removal
   }
   else
   {
      // Load Rating
      nColumns = 2; // location column and column for live load stage
   }
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Prestress Stresses"));

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      if ( bDesign )
      {
         p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   }

   // Set up table headings
   ColumnIndexType col = 0;
   if ( bDesign )
   {
      (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
      {
         (*p_table)(0,col++) << COLHDR(pIntervals->GetDescription(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      //if ( bTempStrands )
      //   (*p_table)(0,col++) << COLHDR(_T("Temporary") << rptNewLine << _T("Strand") << rptNewLine << _T("Removal"),      rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(pIntervals->GetDescription(liveLoadIntervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );
   pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPoi,POI_PIER);

   GET_IFACE2(pBroker,IPretensionStresses,pPrestress);

   // Fill up the table
   pgsPointOfInterest prev_poi(CSegmentKey(0,0,0),0.0);
   bool bSkipToNextRow = false;

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;
      col = 0;

      const pgsPointOfInterest& poi = *iter;
      const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

      if ( row != 1 && IsEqual(poi.GetDistFromStart(),prev_poi.GetDistFromStart()) )
      {
         bSkipToNextRow = true;
         row--;
      }

      Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);
      (*p_table)(row,col++) << gdrpoi.SetValue( POI_RELEASED_SEGMENT, poi );
      (*p_table)(row,col++) << spanpoi.SetValue( POI_GIRDER, poi, end_size  );

      if ( !bSkipToNextRow )
      {
         Float64 fTop, fBot;
         if ( bDesign )
         {
            for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
            {
               fTop = pPrestress->GetStress(intervalIdx,poi,pgsTypes::TopGirder);
               fBot = pPrestress->GetStress(intervalIdx,poi,pgsTypes::BottomGirder);
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
               col++;
            }

            //if ( bTempStrands )
            //{
            //   fTop = pPrestress->GetStress(tsRemovalIntervalIdx,poi,pgsTypes::TopGirder);
            //   fBot = pPrestress->GetStress(tsRemovalIntervalIdx,poi,pgsTypes::BottomGirder);
            //   (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            //   (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            //   col++;
            //}
         }
         else
         {
            // Rating
            fTop = pPrestress->GetStress(liveLoadIntervalIdx,poi,pgsTypes::TopGirder);
            fBot = pPrestress->GetStress(liveLoadIntervalIdx,poi,pgsTypes::BottomGirder);
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            col++;
         }
      }

      row++;
      prev_poi = poi;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPrestressStressTable::MakeCopy(const CPrestressStressTable& rOther)
{
   // Add copy code here...
}

void CPrestressStressTable::MakeAssignment(const CPrestressStressTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

